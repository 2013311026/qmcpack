//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.   
//
// File created by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc. 
//////////////////////////////////////////////////////////////////////////////////////


#ifndef BSPLINE_EVAL_CUDA_H
#define BSPLINE_EVAL_CUDA_H

#include "bspline_structs_cuda.h"

extern "C" void
eval_multi_UBspline_3d_s_cuda (UBspline_3d_s_cuda *spline,
                               float *pos_d, float *vals_d[], int num);

extern "C" void
eval_multi_UBspline_3d_s_sign_cuda (UBspline_3d_s_cuda *spline,
                                    float *pos_d, float *sign_d,
                                    float *vals_d[], int num);

extern "C" void
eval_multi_UBspline_3d_s_vgh_cuda (UBspline_3d_s_cuda *spline,
                                   float *pos_d, float *vals_d[], float *grads_d[],
                                   float *hess_d[], int num);

extern "C" void
eval_multi_UBspline_3d_s_vgl_cuda
(UBspline_3d_s_cuda *spline, float *pos_d, float *Linv_d,
 float *vals_d[], float *grad_lapl_d[], int num, int row_stride);

extern "C" void
eval_multi_UBspline_3d_s_vgl_sign_cuda
(UBspline_3d_s_cuda *spline, float *pos_d, float *sign_d, float *Linv_d,
 float *vals_d[], float *grad_lapl_d[], int num, int row_stride);



#endif
