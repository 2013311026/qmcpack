//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
// jnkim@ornl.gov
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "qmc_common.h"
#include <QMCApp/ParticleSetPool.h>

namespace qmcplusplus 
{
  ///initialize static data
  ParticleSetPool* qmc_common::ptcl_pool=0;
  string qmc_common::master_eshd_name="none";
  bool qmc_common::use_density=false;
  bool qmc_common::dryrun=false;
  bool qmc_common::save_wfs=false;

  void qmc_common::initialize(int argc, char **argv)
  {
    //going to use better option library
    int i=1;
    while(i<argc)
    {
      string c(argv[i]);
      if(c.find("dryrun") < c.size())
      {
        dryrun=true;
      }
      else if(c.find("save_wfs") < c.size())
      {
        //--save_wfs=yes|no
        if(!c.find("no")) save_wfs=true;
      }
      ++i;
    }
  }

  ParticleSetPool* qmc_common::getParticleSetPool(Communicate* mycomm)
  {
    if(ptcl_pool==0)
      ptcl_pool=new ParticleSetPool(mycomm);
    return ptcl_pool;
  }
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 5388 $   $Date: 2011-12-02 08:45:44 -0500 (Fri, 02 Dec 2011) $
 * $Id: Configuration.h 5388 2011-12-02 13:45:44Z jnkim $ 
 ***************************************************************************/
