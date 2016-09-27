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
    
    



#ifndef QMCPLUSPLUS_COMBO_POLYMERESTIMATOR_H
#define QMCPLUSPLUS_COMBO_POLYMERESTIMATOR_H

#include "Estimators/PolymerEstimator.h"
#include "Particle/MCWalkerConfiguration.h"
#include "ReptationEstimators/ReptileEstimator.h"


namespace qmcplusplus
{

struct ComboPolymerEstimator: public PolymerEstimator
{

  ComboPolymerEstimator(QMCHamiltonian& h, int hcopy=1, MultiChain* polymer=0);
  ComboPolymerEstimator(const ComboPolymerEstimator& mest);


  void put(xmlNodePtr cur, MCWalkerConfiguration& refWalker,int Rlength);

  /*@{*/
  void accumulate(const MCWalkerConfiguration& W
                  , WalkerIterator first, WalkerIterator last, RealType wgt);
  void add2Record(RecordNamedProperty<RealType>& record);
  void registerObservables(std::vector<observable_helper*>& h5dec, hid_t gid);
  ScalarEstimatorBase* clone();
  /*@}*/
  void evaluateDiff();

private:

  std::vector<std::string> scalars_name;
  std::vector<int> scalars_index;
  std::vector<ReptileEstimator*> RepEstimators;
  int FirstHamiltonian;
  int SizeOfHamiltonians;
};

}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1926 $   $Date: 2007-04-20 12:30:26 -0500 (Fri, 20 Apr 2007) $
 * $Id: MJPolymerEstimator.h 1926 2007-04-20 17:30:26Z jnkim $
 ***************************************************************************/
