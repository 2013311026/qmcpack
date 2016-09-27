//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Ken Esler, kpesler@gmail.com, StoneRidge Inc.
//                    Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//                    Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//                    Christos Kartsaklis, kartsaklisc@ornl.gov, Oak Ridge National Laboratory
//                    Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//
// File created by: Ken Esler, kpesler@gmail.com, StoneRidge Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    
#include <assert.h>

/*                   !!!WARNING!!!
   Kernels in this file strongly depend on warp-synchronous behavior
   of current generations of GPUs. Any change to that behavior
   as well as any change to the warp size will break the code!
   In such case extra synchronizations are necessary.
*/

template<typename T, int BS> __global__
void phase_factor_kernel (T *kPoints, int *makeTwoCopies,
                          T *pos, T **phi_in, T **phi_out,
                          int num_splines, int num_walkers)
{
  __shared__ T in_shared[2*BS+1], kPoints_s[BS][3],
             pos_s[BS][3];
  volatile __shared__ T out_shared[2*BS+1];
  __shared__ T *phi_in_ptr[BS], *phi_out_ptr[BS];
  int tid = threadIdx.x;
  assert(warpSize == 32);
#pragma unroll
  for (int i=0; i<3; i++)
  {
    int off = (3*blockIdx.x+i)*BS + tid;
    if (off < 3*num_walkers)
      pos_s[0][i*BS + tid] =  pos[off];
  }
  if (blockIdx.x*BS+tid < num_walkers)
  {
    phi_in_ptr[tid]  = phi_in[blockIdx.x*BS+tid];
    phi_out_ptr[tid] = phi_out[blockIdx.x*BS+tid];
  }
  //__syncthreads();
  int nb = (num_splines + BS-1)/BS;
  int outIndex=0;
  int outBlock=0;
  int m2c;
  volatile __shared__ int m2c_ps[BS];
  int numWrite = min(BS, num_walkers-blockIdx.x*BS);
  for (int block=0; block<nb; block++)
  {
    // Load kpoints into shared memory
    for (int i=0; i<3; i++)
    {
      int off = (3*block+i)*BS + tid;
      if (off < 3*num_splines)
        kPoints_s[0][i*BS+tid] = kPoints[off];
    }
    // Load makeTwoCopies with coallesced reads
    if (block*BS+tid < num_splines)
    {
      if(makeTwoCopies[block*BS + tid])
        m2c = 1;
      else
        m2c = 0;
    }
    else
      m2c = 0;
    //prefix sum of m2c array
    m2c_ps[tid] = m2c+1;
    if(tid >= 1)
      m2c_ps[tid] += m2c_ps[tid-1];
    if(tid >= 2)
      m2c_ps[tid] += m2c_ps[tid-2];
    if(tid >= 4)
      m2c_ps[tid] += m2c_ps[tid-4];
    if(tid >= 8)
      m2c_ps[tid] += m2c_ps[tid-8];
    if(tid >= 16)
      m2c_ps[tid] += m2c_ps[tid-16];
    if(tid > 0)
      outIndex  = m2c_ps[tid-1];
    T s, c;
    int end = min (BS, num_splines-block*BS);
    if(tid < end)
      for (int i=0; i<numWrite; i++)
      {
        if ((2*block)*BS+tid < 2*num_splines)
          in_shared[tid   ] = phi_in_ptr[i][(2*block+0)*BS+tid];
        if ((2*block)*BS+tid + end < 2*num_splines)
          in_shared[tid+end] = phi_in_ptr[i][(2*block)*BS+tid + end];
        // Compute e^{-ikr}
        T phase = -(pos_s[i][0]*kPoints_s[tid][0] +
                    pos_s[i][1]*kPoints_s[tid][1] +
                    pos_s[i][2]*kPoints_s[tid][2]);
        sincos(phase, &s, &c);
        T phi_real = in_shared[2*tid]*c - in_shared[2*tid+1]*s;
        T phi_imag = in_shared[2*tid]*s + in_shared[2*tid+1]*c;
        out_shared[outIndex] = phi_real;
        if(m2c)
        {
          out_shared[outIndex + 1] = phi_imag;
        }
        phi_out_ptr[i][outBlock+tid]= out_shared[tid];
        if(tid + end < m2c_ps[end-1])
        {
          phi_out_ptr[i][outBlock + tid + end] = out_shared[tid+end];
        }
      }
    outBlock+= m2c_ps[end-1];
  }
}


template<typename T, int BS> __global__
void phase_factor_kernel_new (T *kPoints, int *makeTwoCopies,
                              T *pos, T **phi_in, T **phi_out,
                              int num_splines)
{
  __shared__ T in_shared[2*BS], out_shared[2*BS], kPoints_s[BS][3],
             pos_s[3];
  __shared__ int m2c[BS];
  __shared__ T *phi_in_ptr, *phi_out_ptr;
  int tid = threadIdx.x;
  if (tid < 3)
    pos_s[tid] = pos[3*blockIdx.x+tid];
  if (tid == 0)
  {
    phi_in_ptr = phi_in[blockIdx.x];
    phi_out_ptr = phi_out[blockIdx.x];
  }
  int NB = (num_splines+BS-1)/BS;
  int outIndex=0, outBlock=0;
  for (int ib=0; ib<NB; ib++)
  {
    for (int i=0; i<3; i++)
      kPoints_s[0][i*BS+tid] = kPoints[(3*ib+i)*BS+tid];
    T phase = -(kPoints_s[tid][0]*pos_s[0] +
                kPoints_s[tid][1]*pos_s[1] +
                kPoints_s[tid][2]*pos_s[2]);
    T s, c;
    sincosf (phase, &s, &c);
    int off = 2*ib*BS + tid;
    in_shared[tid]    = phi_in_ptr[off];
    in_shared[tid+BS] = phi_in_ptr[off+BS];
    T phi_real = in_shared[2*tid]*c - in_shared[2*tid+1]*s;
    T phi_imag = in_shared[2*tid]*s + in_shared[2*tid+1]*c;
    m2c[tid] = makeTwoCopies[ib*BS + tid];
    int iend = min (BS, num_splines - ib*BS);
    for (int i=0; i<iend; i++)
    {
      if (tid == i)
        out_shared[outIndex] = phi_real;
      outIndex++;
      __syncthreads();
      if (outIndex == BS)
      {
        phi_out_ptr[outBlock*BS+tid] = out_shared[tid];
        outIndex = 0;
        outBlock++;
      }
      __syncthreads();
      if (m2c[i])
      {
        if (tid == i)
          out_shared[outIndex] = phi_imag;
        outIndex++;
      }
      __syncthreads();
      if (outIndex == BS)
      {
        phi_out_ptr[outBlock*BS+tid] = out_shared[tid];
        outIndex = 0;
        outBlock++;
      }
      __syncthreads();
    }
  }
  if (tid < outIndex)
    phi_out_ptr[outBlock*BS+tid] = out_shared[tid];
}


// Original implementation

template<typename T, int BS> __global__
void phase_factor_kernel (T *kPoints, int *makeTwoCopies,
                          T *pos, T **phi_in, T **phi_out,
                          T **grad_lapl_in, T **grad_lapl_out,
                          int num_splines, int num_walkers,
                          int row_stride)
{
  volatile __shared__ T in_shared[5][2*BS+1], out_shared[5][BS+1], kPoints_s[BS][3];
  __shared__ T  pos_s[3];
  __shared__ T *my_phi_in, *my_phi_out, *my_GL_in, *my_GL_out;
  int tid = threadIdx.x;
  if (tid == 0)
  {
    my_phi_in  = phi_in[blockIdx.x];
    my_phi_out = phi_out[blockIdx.x];
    my_GL_in   = grad_lapl_in[blockIdx.x];
    my_GL_out  = grad_lapl_out[blockIdx.x];
  }
  if (tid < 3)
    pos_s[tid] = pos[3*blockIdx.x+tid];
  //__syncthreads();
  int nb = (num_splines + BS-1)/BS;
  int outIndex=0;
  int outBlock=0;
  __shared__ int m2c[BS];
  for (int block=0; block<nb; block++)
  {
    // Load kpoints into shared memory
    for (int i=0; i<3; i++)
    {
      int off = (3*block+i)*BS + tid;
      if (off < 3*num_splines)
        kPoints_s[0][i*BS+tid] = kPoints[off];
    }
    // Load phi_in with coallesced reads
    if ((2*block+0)*BS+tid < 2*num_splines)
    {
      in_shared[0][tid+ 0] = my_phi_in[(2*block+0)*BS+tid];
      for (int j=0; j<4; j++)
        in_shared[j+1][tid+ 0] = my_GL_in[2*j*num_splines+(2*block+0)*BS+tid];
    }
    if ((2*block+1)*BS+tid < 2*num_splines)
    {
      in_shared[0][tid+BS] = my_phi_in[(2*block+1)*BS+tid];
      for (int j=0; j<4; j++)
        in_shared[j+1][tid+BS] = my_GL_in[2*j*num_splines+(2*block+1)*BS+tid];
    }
    //__syncthreads();
    // Now add on phase factors
    T phase = -(pos_s[0]*kPoints_s[tid][0] +
                pos_s[1]*kPoints_s[tid][1] +
                pos_s[2]*kPoints_s[tid][2]);
    T s, c;
    sincos (phase, &s, &c);
    T u_re, u_im, gradu_re[3], gradu_im[3], laplu_re, laplu_im;
    u_re        = in_shared[0][2*tid+0];
    u_im        = in_shared[0][2*tid+1];
    gradu_re[0] = in_shared[1][2*tid+0];
    gradu_im[0] = in_shared[1][2*tid+1];
    gradu_re[1] = in_shared[2][2*tid+0];
    gradu_im[1] = in_shared[2][2*tid+1];
    gradu_re[2] = in_shared[3][2*tid+0];
    gradu_im[2] = in_shared[3][2*tid+1];
    laplu_re    = in_shared[4][2*tid+0];
    laplu_im    = in_shared[4][2*tid+1];
    in_shared[0][2*tid+0] = u_re*c - u_im*s;
    in_shared[0][2*tid+1] = u_re*s + u_im*c;
    // Gradient = e^(-ikr)*(-i*u*k + gradu)
    for (int dim=0; dim<3; dim++)
    {
      T gre, gim;
      gre = gradu_re[dim] + kPoints_s[tid][dim]*u_im;
      gim = gradu_im[dim] - kPoints_s[tid][dim]*u_re;
      in_shared[dim+1][2*tid+0] = gre*c - gim*s;
      in_shared[dim+1][2*tid+1] = gre*s + gim*c;
    }
    // Add phase contribution to laplacian
    T k2 = (kPoints_s[tid][0]*kPoints_s[tid][0] +
            kPoints_s[tid][1]*kPoints_s[tid][1] +
            kPoints_s[tid][2]*kPoints_s[tid][2]);
    T lre = laplu_re - k2*u_re + 2.0*(kPoints_s[tid][0]*gradu_im[0]+
                                      kPoints_s[tid][1]*gradu_im[1]+
                                      kPoints_s[tid][2]*gradu_im[2]);
    T lim = laplu_im - k2*u_im - 2.0*(kPoints_s[tid][0]*gradu_re[0]+
                                      kPoints_s[tid][1]*gradu_re[1]+
                                      kPoints_s[tid][2]*gradu_re[2]);
    in_shared[4][2*tid+0] = lre*c - lim*s;
    in_shared[4][2*tid+1] = lre*s + lim*c;
    // Load makeTwoCopies with coallesced reads
    if (block*BS+tid < num_splines)
      m2c[tid] = makeTwoCopies[block*BS + tid];
    //__syncthreads();
    // Now, serialize to output buffer
    int end = min (BS, num_splines - block*BS);
    for (int i=0; i<end; i++)
    {
      if (tid < 5)
        out_shared[tid][outIndex] = in_shared[tid][2*i+0];
      outIndex++;
      //__syncthreads();
      if (outIndex == BS)
      {
        // Write back to global memory
        my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
        my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
        my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
        my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
        my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];
        outIndex = 0;
        outBlock++;
      }
      if (m2c[i])
      {
        if (tid < 5)
          out_shared[tid][outIndex] = in_shared[tid][2*i+1];
        outIndex++;
        //__syncthreads();
        if (outIndex == BS)
        {
          // Write back to global memory
          my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
          my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
          my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
          my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
          my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];
          outIndex = 0;
          outBlock++;
          //__syncthreads();
        }
      }
    }
    //__syncthreads();
  }
  if (tid < outIndex)
  {
    my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
    my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
    my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
    my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
    my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];
  }
}

// Ye: optimized memory access.

template<typename T, int BS> __global__
void phase_factor_kernel (T *kPoints, int *makeTwoCopies,
                          int *TwoCopiesIndex,
                          T *pos, T **phi_in, T **phi_out,
                          T **grad_lapl_in, T **grad_lapl_out,
                          int num_splines, int num_walkers,
                          int row_stride)
{
  T in_shared[5][2], kPoints_s[3];
  __shared__ T  pos_s[3];
  __shared__ T *my_phi_in, *my_phi_out, *my_GL_in, *my_GL_out;
  int tid = threadIdx.x;
  if (tid == 0)
  {
    my_phi_in  = phi_in[blockIdx.x];
    my_phi_out = phi_out[blockIdx.x];
    my_GL_in   = grad_lapl_in[blockIdx.x];
    my_GL_out  = grad_lapl_out[blockIdx.x];
  }
  if (tid < 3)
    pos_s[tid] = pos[3*blockIdx.x+tid];
  __syncthreads();
  int nb = (num_splines + BS-1)/BS;
  __shared__ int m2c[BS], m2cIndex[BS];
  for (int block=0; block<nb; block++)
  {
    int off = block*BS + tid;
    if (off < num_splines)
    {
      // Load kpoints
      kPoints_s[0] = kPoints[off*3  ];
      kPoints_s[1] = kPoints[off*3+1];
      kPoints_s[2] = kPoints[off*3+2];
      // Load phi_in
      in_shared[0][0] = my_phi_in[off*2];
      in_shared[0][1] = my_phi_in[off*2+1];
      for (int j=0; j<4; j++)
      {
        in_shared[j+1][0] = my_GL_in[2*j*num_splines+2*off  ];
        in_shared[j+1][1] = my_GL_in[2*j*num_splines+2*off+1];
      }
      // Load makeTwoCopies
      m2c[tid] = makeTwoCopies[off];
      m2cIndex[tid] = TwoCopiesIndex[off];
    }
    // Now add on phase factors
    T phase = -(pos_s[0]*kPoints_s[0] +
                pos_s[1]*kPoints_s[1] +
                pos_s[2]*kPoints_s[2]);
    T s, c;
    sincos (phase, &s, &c);
    T u_re, u_im, gradu_re[3], gradu_im[3], laplu_re, laplu_im;
    u_re        = in_shared[0][0];
    u_im        = in_shared[0][1];
    gradu_re[0] = in_shared[1][0];
    gradu_im[0] = in_shared[1][1];
    gradu_re[1] = in_shared[2][0];
    gradu_im[1] = in_shared[2][1];
    gradu_re[2] = in_shared[3][0];
    gradu_im[2] = in_shared[3][1];
    laplu_re    = in_shared[4][0];
    laplu_im    = in_shared[4][1];
    in_shared[0][0] = u_re*c - u_im*s;
    in_shared[0][1] = u_re*s + u_im*c;
    // Gradient = e^(-ikr)*(-i*u*k + gradu)
    for (int dim=0; dim<3; dim++)
    {
      T gre, gim;
      gre = gradu_re[dim] + kPoints_s[dim]*u_im;
      gim = gradu_im[dim] - kPoints_s[dim]*u_re;
      in_shared[dim+1][0] = gre*c - gim*s;
      in_shared[dim+1][1] = gre*s + gim*c;
    }
    // Add phase contribution to laplacian
    T k2 = (kPoints_s[0]*kPoints_s[0] +
            kPoints_s[1]*kPoints_s[1] +
            kPoints_s[2]*kPoints_s[2]);
    T lre = laplu_re - k2*u_re + 2.0*(kPoints_s[0]*gradu_im[0]+
                                      kPoints_s[1]*gradu_im[1]+
                                      kPoints_s[2]*gradu_im[2]);
    T lim = laplu_im - k2*u_im - 2.0*(kPoints_s[0]*gradu_re[0]+
                                      kPoints_s[1]*gradu_re[1]+
                                      kPoints_s[2]*gradu_re[2]);
    in_shared[4][0] = lre*c - lim*s;
    in_shared[4][1] = lre*s + lim*c;
    // Now prepare the output buffer
    int end = min (BS, num_splines - block*BS);
    if ( tid < end )
    {
      my_phi_out[              m2cIndex[tid]] = in_shared[0][0];
      my_GL_out[0*row_stride + m2cIndex[tid]] = in_shared[1][0];
      my_GL_out[1*row_stride + m2cIndex[tid]] = in_shared[2][0];
      my_GL_out[2*row_stride + m2cIndex[tid]] = in_shared[3][0];
      my_GL_out[3*row_stride + m2cIndex[tid]] = in_shared[4][0];
      if (m2c[tid])
      {
        my_phi_out[              m2cIndex[tid]+1] = in_shared[0][1];
        my_GL_out[0*row_stride + m2cIndex[tid]+1] = in_shared[1][1];
        my_GL_out[1*row_stride + m2cIndex[tid]+1] = in_shared[2][1];
        my_GL_out[2*row_stride + m2cIndex[tid]+1] = in_shared[3][1];
        my_GL_out[3*row_stride + m2cIndex[tid]+1] = in_shared[4][1];
      }
    }
  }
}

#include <cstdio>
#include <complex>
#include <iostream>
#include "../CUDA/gpu_misc.h"

void apply_phase_factors(float kPoints[], int makeTwoCopies[],
                         float pos[], float *phi_in[], float *phi_out[],
                         int num_splines, int num_walkers)
{
//   float kPoints_h[3*num_splines];
//   int makeTwoCopies_h[num_splines];
//   float pos_h[3*num_walkers];
//   float *phi_in_ptr[num_walkers];
//   float *phi_out_ptr[num_walkers];
//   cudaMemcpy (kPoints_h, kPoints, 3*num_splines*sizeof(float), cudaMemcpyDeviceToHost);
//   cudaMemcpy (makeTwoCopies_h, makeTwoCopies, num_splines*sizeof(int), cudaMemcpyDeviceToHost);
//   cudaMemcpy (pos_h, pos, 3*num_walkers*sizeof(float), cudaMemcpyDeviceToHost);
//   cudaMemcpy (phi_in_ptr,  phi_in,  num_walkers*sizeof(float*), cudaMemcpyDeviceToHost);
//   cudaMemcpy (phi_out_ptr, phi_out, num_walkers*sizeof(float*), cudaMemcpyDeviceToHost);
//   for (int iw=0; iw<num_walkers; iw++) {
//     cudaMemcpy (kPoints_h, kPoints, 3*num_splines*sizeof(float), cudaMemcpyDeviceToHost);
//     std::complex<float> phi_in_h[num_splines];
//     float phi_out_h[num_splines*2];
//     cudaMemcpy (phi_in_h, phi_in_ptr[iw], num_splines*2*sizeof(float), cudaMemcpyDeviceToHost);
//     int iout = 0;
//     for (int isp=0; isp < num_splines; isp++) {
//       float phase = -(kPoints_h[3*isp+0] * pos_h[3*iw+0] +
// 		      kPoints_h[3*isp+1] * pos_h[3*iw+1] +
// 		      kPoints_h[3*isp+2] * pos_h[3*iw+2]);
//       float s,c;
//       sincosf(phase, &s, &c);
//       std::complex<float> z(c,s);
//       std::complex<float> out = z*phi_in_h[isp];
//       phi_out_h[iout++] = out.real();
//       if (makeTwoCopies_h[isp])
// 	phi_out_h[iout++] = out.imag();
//     }
//     cudaMemcpyAsync (phi_out_ptr[iw], phi_out_h, iout*sizeof(float), cudaMemcpyHostToDevice);
//   }
//   return;
  const int BS = 32;
  dim3 dimBlock(BS);
  dim3 dimGrid ((num_walkers+BS-1)/BS);
  phase_factor_kernel<float,BS><<<dimGrid,dimBlock>>>
  (kPoints, makeTwoCopies, pos, phi_in, phi_out, num_splines, num_walkers);
  // dim3 dimGrid (num_walkers);
  // phase_factor_kernel_new<float,BS><<<dimGrid,dimBlock>>>
  //   (kPoints, makeTwoCopies, pos, phi_in, phi_out, num_splines);
}


void apply_phase_factors(float kPoints[], int makeTwoCopies[],
                         float pos[], float *phi_in[], float *phi_out[],
                         float *GL_in[], float *GL_out[],
                         int num_splines, int num_walkers, int row_stride)
{
  const int BS = 32;
  dim3 dimBlock(BS);
  dim3 dimGrid (num_walkers);
  phase_factor_kernel<float,BS><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
  (kPoints, makeTwoCopies, pos, phi_in, phi_out,
   GL_in, GL_out, num_splines, num_walkers, row_stride);
}

void apply_phase_factors(float kPoints[], int makeTwoCopies[], int TwoCopiesIndex[],
                         float pos[], float *phi_in[], float *phi_out[],
                         float *GL_in[], float *GL_out[],
                         int num_splines, int num_walkers, int row_stride)
{
  const int BS = 128;
  dim3 dimBlock(BS);
  dim3 dimGrid (num_walkers);
  phase_factor_kernel<float,BS><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
  (kPoints, makeTwoCopies, TwoCopiesIndex, pos, phi_in, phi_out,
   GL_in, GL_out, num_splines, num_walkers, row_stride);
  /*
  const int BS = 32;
  dim3 dimBlock(BS);
  dim3 dimGrid (num_walkers);
  phase_factor_kernel<float,BS><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
  (kPoints, makeTwoCopies, pos, phi_in, phi_out,
   GL_in, GL_out, num_splines, num_walkers, row_stride);
  */
}

void apply_phase_factors(double kPoints[], int makeTwoCopies[],
                         double pos[], double *phi_in[], double *phi_out[],
                         int num_splines, int num_walkers)
{
  const int BS = 32;
  dim3 dimBlock(BS);
  dim3 dimGrid ((num_walkers+BS-1)/BS);
  phase_factor_kernel<double,BS><<<dimGrid,dimBlock>>>
  (kPoints, makeTwoCopies, pos, phi_in, phi_out, num_splines, num_walkers);
}


void apply_phase_factors(double kPoints[], int makeTwoCopies[],
                         double pos[], double *phi_in[], double *phi_out[],
                         double *GL_in[], double *GL_out[],
                         int num_splines, int num_walkers, int row_stride)
{
  const int BS = 32;
  dim3 dimBlock(BS);
  dim3 dimGrid (num_walkers);
  phase_factor_kernel<double,BS><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
  (kPoints, makeTwoCopies, pos, phi_in, phi_out,
   GL_in, GL_out, num_splines, num_walkers, row_stride);
}

void apply_phase_factors(double kPoints[], int makeTwoCopies[], int TwoCopiesIndex[],
                         double pos[], double *phi_in[], double *phi_out[],
                         double *GL_in[], double *GL_out[],
                         int num_splines, int num_walkers, int row_stride)
{
  const int BS = 128;
  dim3 dimBlock(BS);
  dim3 dimGrid (num_walkers);
  phase_factor_kernel<double,BS><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
  (kPoints, makeTwoCopies, TwoCopiesIndex, pos, phi_in, phi_out,
   GL_in, GL_out, num_splines, num_walkers, row_stride);
}

#ifdef ALGO_CHRISTOS
// Slower for some runs
#define WARP_SIZE 32

// NO_TWO_COPIES can be set to true if the makeTwoCopies is guaranteed to
//   only hold 0s.
template<typename T, int BS, bool NO_TWO_COPIES> __global__
void phase_factor_kernel (T *kPoints, int *makeTwoCopies,
                          T *pos, T **phi_in, T **phi_out,
                          T **grad_lapl_in, T **grad_lapl_out,
                          int num_splines, int num_walkers,
                          int row_stride)
{
  volatile __shared__ T in_shared[5][2*BS+1], out_shared[5][BS+1], kPoints_s[BS][3];
  __shared__ T  pos_s[3];
  __shared__ T *my_phi_in, *my_phi_out, *my_GL_in, *my_GL_out;
  int tid = threadIdx.x;
  if (tid == 0)
  {
    my_phi_in  = phi_in[blockIdx.x];
    my_phi_out = phi_out[blockIdx.x];
    my_GL_in   = grad_lapl_in[blockIdx.x];
    my_GL_out  = grad_lapl_out[blockIdx.x];
  }
  if (tid < 3)
    pos_s[tid] = pos[3*blockIdx.x+tid];

  if (BS > WARP_SIZE) __syncthreads();

  int nb = (num_splines + BS-1)/BS;
  int outIndex=0;
  int outBlock=0;

  // 0 if NO_TWO_COPIES, but won't compile
  __shared__ int m2c[NO_TWO_COPIES ? 1 : BS];

  for (int block=0; block<nb; block++)
  {
    // Load kpoints into shared memory
    for (int i=0; i<3; i++)
    {
      int off = (3*block+i)*BS + tid;
      if (off < 3*num_splines)
        kPoints_s[0][i*BS+tid] = kPoints[off];
    }
    // Load phi_in with coallesced reads
    if ((2*block+0)*BS+tid < 2*num_splines)
    {
      in_shared[0][tid+ 0] = my_phi_in[(2*block+0)*BS+tid];
      for (int j=0; j<4; j++)
        in_shared[j+1][tid+ 0] = my_GL_in[2*j*num_splines+(2*block+0)*BS+tid];
    }
    if ((2*block+1)*BS+tid < 2*num_splines)
    {
      in_shared[0][tid+BS] = my_phi_in[(2*block+1)*BS+tid];
      for (int j=0; j<4; j++)
        in_shared[j+1][tid+BS] = my_GL_in[2*j*num_splines+(2*block+1)*BS+tid];
    }

    if (BS > WARP_SIZE) __syncthreads();

    // Now add on phase factors
    T phase = -(pos_s[0]*kPoints_s[tid][0] +
                pos_s[1]*kPoints_s[tid][1] +
                pos_s[2]*kPoints_s[tid][2]);
    T s, c;
    sincosf (phase, &s, &c);
    T u_re, u_im, gradu_re[3], gradu_im[3], laplu_re, laplu_im;
    u_re        = in_shared[0][2*tid+0];
    u_im        = in_shared[0][2*tid+1];
    gradu_re[0] = in_shared[1][2*tid+0];
    gradu_im[0] = in_shared[1][2*tid+1];
    gradu_re[1] = in_shared[2][2*tid+0];
    gradu_im[1] = in_shared[2][2*tid+1];
    gradu_re[2] = in_shared[3][2*tid+0];
    gradu_im[2] = in_shared[3][2*tid+1];
    laplu_re    = in_shared[4][2*tid+0];
    laplu_im    = in_shared[4][2*tid+1];
    in_shared[0][2*tid+0] = u_re*c - u_im*s;
    in_shared[0][2*tid+1] = u_re*s + u_im*c;
    // Gradient = e^(-ikr)*(-i*u*k + gradu)
    for (int dim=0; dim<3; dim++)
    {
      T gre, gim;
      gre = gradu_re[dim] + kPoints_s[tid][dim]*u_im;
      gim = gradu_im[dim] - kPoints_s[tid][dim]*u_re;
      in_shared[dim+1][2*tid+0] = gre*c - gim*s;
      in_shared[dim+1][2*tid+1] = gre*s + gim*c;
    }
    // Add phase contribution to laplacian
    T k2 = (kPoints_s[tid][0]*kPoints_s[tid][0] +
            kPoints_s[tid][1]*kPoints_s[tid][1] +
            kPoints_s[tid][2]*kPoints_s[tid][2]);
    T lre = laplu_re - k2*u_re + 2.0*(kPoints_s[tid][0]*gradu_im[0]+
                                      kPoints_s[tid][1]*gradu_im[1]+
                                      kPoints_s[tid][2]*gradu_im[2]);
    T lim = laplu_im - k2*u_im - 2.0*(kPoints_s[tid][0]*gradu_re[0]+
                                      kPoints_s[tid][1]*gradu_re[1]+
                                      kPoints_s[tid][2]*gradu_re[2]);
    in_shared[4][2*tid+0] = lre*c - lim*s;
    in_shared[4][2*tid+1] = lre*s + lim*c;
    // Load makeTwoCopies with coallesced reads

    if (!NO_TWO_COPIES)
      if (block*BS+tid < num_splines)
        m2c[tid] = makeTwoCopies[block*BS + tid];

    if (BS > WARP_SIZE) __syncthreads();

    // Now, serialize to output buffer
    int end = min (BS, num_splines - block*BS);

    // christos: When NO_TWO_COPIES is on, the output buffer logic can be 
    // optimized further, cutting down on instruction level clutter,
    // synchronization & overall divergence. This is because outIndex's
    // updates follow the induction variable (i).

    if (NO_TWO_COPIES && end==BS) {
       // christos: note: it is possible to eliminate out_shared under
       // certain conditions
       for (int i=tid; i<end; i+=BS) {
          for (int j=0 ; j<5 ; j++)
          out_shared[j][outIndex+i] = in_shared[j][2*i+0];
       }

       if (BS > WARP_SIZE) __syncthreads();
       my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
       my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
       my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
       my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
       my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];

       outIndex = 0;
       outBlock++;
    } else {

    for (int i=0; i<end; i++)
    {
      if (tid < 5)
        out_shared[tid][outIndex] = in_shared[tid][2*i+0];
      outIndex++;
      if (BS > WARP_SIZE) __syncthreads();
      if (outIndex == BS)
      {
        // Write back to global memory
        my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
        my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
        my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
        my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
        my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];
        outIndex = 0;
        outBlock++;
      }
      if (!NO_TWO_COPIES && m2c[i])
      {
        if (tid < 5)
          out_shared[tid][outIndex] = in_shared[tid][2*i+1];
        outIndex++;
	if (BS > WARP_SIZE) __syncthreads();
        if (outIndex == BS)
        {
          // Write back to global memory
          my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
          my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
          my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
          my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
          my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];
          outIndex = 0;
          outBlock++;
          if (BS > WARP_SIZE) __syncthreads();
        }
      }
    }
    if (BS > WARP_SIZE) __syncthreads();
  }
  }
  if (tid < outIndex)
  {
    my_phi_out[             outBlock*BS+tid] = out_shared[0][tid];
    my_GL_out[0*row_stride +outBlock*BS+tid] = out_shared[1][tid];
    my_GL_out[1*row_stride +outBlock*BS+tid] = out_shared[2][tid];
    my_GL_out[2*row_stride +outBlock*BS+tid] = out_shared[3][tid];
    my_GL_out[3*row_stride +outBlock*BS+tid] = out_shared[4][tid];
  }
}

void apply_phase_factors(float kPoints[], int makeTwoCopies[],
                         float pos[], float *phi_in[], float *phi_out[],
                         float *GL_in[], float *GL_out[],
                         int num_splines, int num_walkers, int row_stride,
                         bool dontMakeTwoCopies)
{
  const int BS = 64;
  dim3 dimBlock(BS);
  dim3 dimGrid (num_walkers);

//  std::cout << "num_splines=" << num_splines << ", nb=" << ((num_splines + BS-1)/BS) << std::endl;

  if (dontMakeTwoCopies) {
     phase_factor_kernel<float,BS,true><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
       (kPoints, makeTwoCopies, pos, phi_in, phi_out,
       GL_in, GL_out, num_splines, num_walkers, row_stride);
  } else {
     phase_factor_kernel<float,BS,false><<<dimGrid,dimBlock, 0, gpu::kernelStream>>>
       (kPoints, makeTwoCopies, pos, phi_in, phi_out,
       GL_in, GL_out, num_splines, num_walkers, row_stride);
  }
}
#endif

