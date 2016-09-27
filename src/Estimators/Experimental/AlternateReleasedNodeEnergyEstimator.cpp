//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    
    
    
    
    
    
    



#include "Estimators/AlternateReleasedNodeEnergyEstimator.h"

namespace qmcplusplus
{

AlternateReleasedNodeEnergyEstimator::AlternateReleasedNodeEnergyEstimator(QMCHamiltonian& h, int Sm)
  :refH(h), Smax(Sm)
{
  N_rn = Smax+1;
  SizeOfHamiltonians = h.sizeOfObservables();
  FirstHamiltonian = h.startIndex();
  scalars.resize(SizeOfHamiltonians+LE_MAX+3.0*N_rn);
  scalars_saved.resize(SizeOfHamiltonians+LE_MAX+3.0*N_rn);
}

ScalarEstimatorBase* AlternateReleasedNodeEnergyEstimator::clone()
{
  AlternateReleasedNodeEnergyEstimator* myClone = new AlternateReleasedNodeEnergyEstimator(*this);
  return myClone;
}

/**  add the local energy, variance and all the Hamiltonian components to the scalar record container
 * @param record storage of scalar records (name,value)
 */
void AlternateReleasedNodeEnergyEstimator::add2Record(RecordListType& record)
{
  FirstIndex = record.size();
  int dumy=record.add("LocalEnergy");
  dumy=record.add("LocalEnergy_sq");
  dumy=record.add("LocalPotential");
  dumy=record.add("FermionEnergy");
  for(int i=0; i<N_rn; ++i)
  {
    std::ostringstream o;
    o << "EW_RN_" << i;
    std::ostringstream p;
    p << "E2W_RN_" << i;
    std::ostringstream q;
    q << "W_RN_" << i;
    record.add(o.str());
    record.add(p.str());
    record.add(q.str());
  }
  for(int i=0; i<SizeOfHamiltonians; ++i)
    record.add(refH.getObservableName(i));
  LastIndex=record.size();
  clear();
}

}
/***************************************************************************
 * $RCSfile$   $Author: jmcminis $
 * $Revision: 4163 $   $Date: 2009-08-31 05:47:46 -0500 (Mon, 31 Aug 2009) $
 * $Id: AlternateReleasedNodeEnergyEstimator.cpp 4163 2009-08-31 10:47:46Z jmcminis $
 ***************************************************************************/
