//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//                    Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//                    Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    


#include "OhmmsData/ParameterSet.h"
#include "QMCDrivers/DMC/WalkerControlFactory.h"
#include "QMCDrivers/DMC/WalkerReconfiguration.h"
#include "QMCDrivers/DMC/WalkerPureDMC.h"
#if defined(HAVE_MPI)
#include "QMCDrivers/DMC/WalkerControlMPI.h"
#include "QMCDrivers/DMC/WalkerReconfigurationMPI.h"
#endif

namespace qmcplusplus
{

WalkerControlBase* createWalkerController(int nwtot, Communicate* comm, xmlNodePtr cur,
    bool reconfig)
{
  app_log() << "  Creating WalkerController: target  number of walkers = " << nwtot << std::endl;
  ///set of parameters
  int nmax=0;
  int nmin=0;
  std::string reconfigopt("no");
  ParameterSet m_param;
  m_param.add(nwtot,"targetWalkers","int");
  m_param.add(nwtot,"targetwalkers","int");
  m_param.add(nmax,"max_walkers","int");
  m_param.add(reconfigopt,"reconfiguration","string");
  m_param.put(cur);
  //if(nmax<0) nmax=2*nideal;
  //if(nmin<0) nmin=nideal/2;
  WalkerControlBase* wc=0;
  int ncontexts = comm->size();
  bool fixw= (reconfig || reconfigopt == "yes"|| reconfigopt == "pure");
  if(fixw)
  {
    int nwloc=std::max(omp_get_max_threads(),nwtot/ncontexts);
    nwtot=nwloc*ncontexts; 
    nmax=nwtot/ncontexts;
    nmin=nwtot/ncontexts;
  }
  else
  {
    int npernode=nwtot/ncontexts;
    nmin=npernode/5+1;
    if (nmax==0) nmax=2*npernode+1;
  }
#if defined(HAVE_MPI)
  if(ncontexts>1)
  {
    if(fixw)
    {
      app_log() << "  Using WalkerReconfigurationMPI for population control." << std::endl;
      wc = new WalkerReconfigurationMPI(comm);
    }
    else
    {
      app_log() << "  Using WalkerControlMPI for dynamic population control." << std::endl;
      wc = new WalkerControlMPI(comm);
    }
  }
  else
#endif
  {
    if(fixw)
    {
      app_log() << "  Using WalkerReconfiguration for population control." << std::endl;
      wc = new WalkerReconfiguration(comm);
    }
    else
    {
      app_log() << "  Using WalkerControlBase for dynamic population control." << std::endl;
      wc = new WalkerControlBase(comm);
    }
  }
  wc->Nmin=nmin;
  wc->Nmax=nmax;
  wc->MyMethod=fixw;
  return wc;
}

WalkerControlBase* CreateWalkerController(
  bool reconfig, int& swapmode, int nideal,
  int nmax, int nmin, WalkerControlBase* wc,
  Communicate* comm)
{
  int ncontexts = comm->size();
  //overwrite the SwapMode for a single-node run
  if(ncontexts == 1)
  {
    swapmode=0;
  }
  if(reconfig)
  {
    nmax=nideal/ncontexts;
    nmin=nideal/ncontexts;
  }
  else
  {
    if(swapmode)
    {
      if (nmax==0)
      {
        int npernode=nideal/ncontexts;
        nmax=2*npernode+1;
        nmin=npernode/5+1;
      }
      else
      {
        int nmaxpernode = nmax/ncontexts;
        int npernode=nideal/ncontexts;
        nmax=nmaxpernode;
        nmin=npernode/5+1;
      }
    }
    else
    {
      if (nmax==0)
      {
        nmax=2*nideal;
        nmin=nideal/2;
      }
      else
      {
        nmin=nideal/2;
      }
    }
  }
  if(wc)
  {
    if(swapmode != wc->SwapMode)
    {
      delete wc;
      wc=0;
    }
  }
  if(wc == 0)
  {
#if defined(HAVE_MPI)
    if(swapmode)
    {
      if(reconfig)
      {
        app_log() << "  Using WalkerReconfigurationMPI for population control." << std::endl;
        wc = new WalkerReconfigurationMPI(comm);
      }
      else
      {
        app_log() << "  Using WalkerControlMPI for dynamic population control." << std::endl;
        wc = new WalkerControlMPI(comm);
      }
    }
    else
#endif
    {
      if(reconfig)
      {
        app_log() << "  Using WalkerReconfiguration for population control." << std::endl;
        wc = new WalkerReconfiguration(comm);
      }
      else
      {
        app_log() << "  Using WalkerControlBase for dynamic population control." << std::endl;
        wc = new WalkerControlBase(comm);
      }
    }
  }
  wc->Nmin=nmin;
  wc->Nmax=nmax;
  return wc;
}
}
/***************************************************************************
 * $RCSfile: WalkerControlFactory.cpp,v $   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/

