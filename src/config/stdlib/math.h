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


#ifndef QMCPLUSPLUS_STDLIB_PORT_H
#define QMCPLUSPLUS_STDLIB_PORT_H
#include <config.h>
#include <cmath>
#include <cstdlib>
#ifndef TWOPI
#ifndef M_PI
#define TWOPI 6.2831853071795862
#else
#define TWOPI (2*M_PI)
#endif /* M_PI */
#endif /* TWOPI */

#if !defined(HAVE_STD_ROUND)
template<typename T> inline T round(T x)
{
  T dmy;
  x=modf(x,&dmy);
  return x-static_cast<int>(x*2.0);
}
#endif

#if defined(HAVE_SINCOS)
inline void sincos(float a, float* s, float* c)
{
  sincosf(a,s,c);
}
#else
template<typename T>
inline void sincos(T a, T* restrict s, T*  restrict c)
{
  *s=std::sin(a);
  *c=std::cos(a);
}
inline void sincos(float a, float* restrict s, float*  restrict c)
{
  *s=sinf(a);
  *c=cosf(a);
}
#endif

namespace qmcplusplus
{
/** return i^n
 *
 * std::pow(int,int) is not standard
 */
inline int pow(int i, int n)
{
  return static_cast<int>(std::pow(static_cast<double>(i),n));
}
}

#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 3310 $   $Date: 2008-10-29 19:21:31 -0500 (Wed, 29 Oct 2008) $
 * $Id: stdfunc.h 3310 2008-10-30 00:21:31Z jnkim $
 ***************************************************************************/
