//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//                    Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    


#include <Configuration.h>
#include "Utilities/OhmmsInfo.h"
#include "Utilities/RandomGenerator.h"
#include "Utilities/Timer.h"
#include "Message/Communicate.h"
//#define SINGLE_PREC
#include <fft/fft.h>


namespace qmcplusplus
{
template<typename T>
inline T abs_diff(T a, T b)
{
  return std::abs(a-b);
}

template<typename T>
inline T abs_diff(std::complex<T>& a, std::complex<T>& b)
{
  return std::abs(a.real()-b.real())+std::abs(a.imag()-b.imag());
}

template<typename T>
inline T abs_diff(std::complex<T>& a, T b)
{
  return std::abs(a.real()-b);
}
template<typename T1, typename T2, unsigned fft_eng>
struct fft1d_test
{
  typedef fft1d_engine<T1,T2,fft_eng> fft1d_engine_t;
  typedef typename fft1d_engine_t::space_type space_type;
  typedef typename fft1d_engine_t::spectral_type spectral_type;
  typedef typename fft1d_engine_t::real_type real_type;

  bool InPlaceFFT;
  fft1d_engine_t myfft;
  Matrix<space_type> in;
  Matrix<spectral_type> out;
  std::vector<space_type> sample;

  fft1d_test(int fft_dim, int howmany, bool inplace):InPlaceFFT(inplace)
  {
    reset(fft_dim,howmany,inplace);
  }

  void reset(int fft_dim, int howmany, bool inplace)
  {
    InPlaceFFT=inplace;
    myfft.set_defaults(fft_dim,howmany);
    in.resize(myfft(FFT_NUMBER_OF_TRANSFORMS),myfft(FFT_IN_DISTANCE));
    out.resize(myfft(FFT_NUMBER_OF_TRANSFORMS),myfft(FFT_OUT_DISTANCE));
    sample.resize(fft_dim);
    real_type phase=2*M_PI/static_cast<real_type>(fft_dim);
    for(int i=0; i<sample.size(); ++i)
      sample[i]=0.5*std::sin(phase*i)+0.1*std::sin(phase*2*i)+0.3*std::sin(phase*3*i)+0.5*std::sin(phase*4.1*i);;
    for(int i=0; i<howmany; ++i)
      copy(sample.begin(),sample.end(),in[i]);
    if(inplace)
      myfft.create(in.data());
    else
      myfft.create(in.data(),out.data());
  }

  void debug()
  {
    if(InPlaceFFT)
    {
      myfft.fft_forward(in.data());
      myfft.fft_backward(in.data());
    }
    else
    {
      myfft.fft_forward(in.data(),out.data());
      myfft.fft_backward(out.data(),in.data());
    }
    int fft_dim=myfft(FFT_LENGTH);
    int howmany=myfft(FFT_NUMBER_OF_TRANSFORMS);
    real_type norm=1.0/static_cast<real_type>(fft_dim);
    real_type eps=10*numeric_limits<real_type>::epsilon();
    in *= norm;
    for(int i=0; i<howmany; ++i)
    {
      std::cout << "State index = " << i << std::endl;
      for(int k=0; k<fft_dim; ++k)
      {
        //cout << target(i,k) << " " << sample[k] << std::endl;
        if(abs_diff(in(i,k),sample[k])>eps)
          std::cerr << "WRONG " << in(i,k) << " " << sample[k] <<  " " << std::endl;
      }
    }
  }
};

template<typename T, unsigned fft_id>
struct fft1d_debug
{
  static void doit(int fft_dim, int howmany)
  {
    {
      std::cout << "r2c, in-place " << std::endl;
      fft1d_test<T,complex<T>,fft_id> test(fft_dim,howmany,true);
      test.debug();
    }
    {
      std::cout << "r2c, out-place " << std::endl;
      fft1d_test<T,complex<T>,fft_id> test(fft_dim,howmany,false);
      test.debug();
    }
    {
      std::cout << "c2c, in-place " << std::endl;
      fft1d_test<std::complex<T>,complex<T>,fft_id> test(fft_dim,howmany,true);
      test.debug();
    }
    {
      std::cout << "c2c, out-place " << std::endl;
      fft1d_test<std::complex<T>,complex<T>,fft_id> test(fft_dim,howmany,false);
      test.debug();
    }
  }
};

}


inline void print_help(const std::string& msg)
{
  printf("%s -d fft_dim -m numer_of_fft -i iterations -p [d|s] -t [r2c|c2c] -e [fftw|mkl|essl] \n",msg.c_str());
}

int main(int argc, char** argv)
{
  using namespace qmcplusplus;
  OHMMS::Controller->initialize(argc,argv);
  Communicate* mycomm=OHMMS::Controller;
  OhmmsInfo Welcome(argc,argv,mycomm->rank());
  qmcplusplus::Random.init(0,1,11);
  int howmany=1;
  int niters=10;
  int fft_dim=16;
  //accepted: r2c  or c2c
  std::string fft_type("r2c");
  std::string fft_eng("fftw");
  bool inplace=true;
  bool single_precision=false;
  int opt;
  while((opt = getopt(argc, argv, "hp:d:m:i:e:t:")) != -1)
  {
    switch(opt)
    {
    case 'h':
      print_help("Help Message");
      return 1;
    case 'd':
      fft_dim=atoi(optarg);
      break;
    case 'm':
      howmany=atoi(optarg);
      break;
    case 'i':
      niters=atoi(optarg);
      break;
    case 'p':
      if(optarg[0]=='s')
        single_precision=true;
      break;
    case 'e':
      fft_eng=optarg;
      break;
    case 't':
      fft_type=optarg;
      break;
    default:
      print_help("Unknown options");
      return 1;
    }
  }
  if(fft_eng =="fftw")
  {
    std::cout << "Testing FFT1D with FFTW " << std::endl;
#if defined(SINGLE_PREC)
    std::cout << "Single precision " << std::endl;
    fft1d_debug<float,FFTW_ENG>::doit(fft_dim,howmany);
#else
    std::cout << "Double precision " << std::endl;
    fft1d_debug<double,FFTW_ENG>::doit(fft_dim,howmany);
#endif
  }
#if defined(HAVE_MKL)
  else
    if(fft_eng=="mkl")
    {
      std::cout << "Testing FFT1D with MKL " << std::endl;
      std::cout << "Double precision " << std::endl;
      fft1d_debug<double,FFTMKL_ENG>::doit(fft_dim,howmany);
      std::cout << "Single precision " << std::endl;
      fft1d_debug<float,FFTMKL_ENG>::doit(fft_dim,howmany);
    }
#endif
#if defined(HAVE_ESSL)
    else
      if(fft_eng =="essl")
      {
        std::cout << "Testing FFT1D with ESSL " << std::endl;
        std::cout << "Double precision " << std::endl;
        fft1d_debug<double,FFTESSL_ENG>::doit(fft_dim,howmany);
        std::cout << "Single precision " << std::endl;
        fft1d_debug<float,FFTESSL_ENG>::doit(fft_dim,howmany);
      }
#endif
      else
      {
        std::cout << fft_eng << " is not available. Choose fftw, mkl or essl" << std::endl;
      }
  OHMMS::Controller->finalize();
  return 0;
}

/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1770 $   $Date: 2007-02-17 17:45:38 -0600 (Sat, 17 Feb 2007) $
 * $Id: OrbitalBase.h 1770 2007-02-17 23:45:38Z jnkim $
 ***************************************************************************/
