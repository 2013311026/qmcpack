//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//                    Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//                    Mark Dewing, markdewing@gmail.com, University of Illinois at Urbana Champain
//
// File created by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    


#ifndef OHMMS_SYSTEM_UTILITIES_H
#define OHMMS_SYSTEM_UTILITIES_H

/*!\file sysutil.h
 * Function declarations to get system information.
 */
#include <string>

//!< return the host name
std::string getHostName();

//!< return the date and time
std::string getDateAndTime();

/** get the time and date with a format
 */
std::string getDateAndTime(const char* format);

#ifdef __linux__
#include "sys/sysinfo.h"

inline size_t freemem()
{
  struct sysinfo si;
  sysinfo(&si);
  si.freeram+=si.bufferram;
  return si.freeram>>20;
  //return (si.freeram + si.bufferram);
}
#else

inline size_t freemem()
{
  return 0;
}

#endif

#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/
