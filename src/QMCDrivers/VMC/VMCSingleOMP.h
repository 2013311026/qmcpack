//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//                    Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//                    Mark A. Berrill, berrillma@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    


#ifndef QMCPLUSPLUS_VMCSINGLE_OMP_H
#define QMCPLUSPLUS_VMCSINGLE_OMP_H
#include "QMCDrivers/QMCDriver.h"
#include "QMCDrivers/CloneManager.h"
namespace qmcplusplus
{

/** @ingroup QMCDrivers  ParticleByParticle
 * @brief Implements a VMC using particle-by-particle move. Threaded execution.
 */
class VMCSingleOMP: public QMCDriver, public CloneManager
{
public:
  /// Constructor.
  VMCSingleOMP(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h,
               HamiltonianPool& hpool, WaveFunctionPool& ppool);
  bool run();
  bool put(xmlNodePtr cur);
  //inline std::vector<RandomGenerator_t*>& getRng() { return Rng;}
private:
  int prevSteps;
  int prevStepsBetweenSamples;

  ///Ways to set rn constant
  RealType logoffset,logepsilon;
  ///option to enable/disable drift equation or RN for VMC
  std::string UseDrift;
  ///check the run-time environments
  void resetRun();
  ///copy constructor
  VMCSingleOMP(const VMCSingleOMP& a): QMCDriver(a),CloneManager(a) { }
  /// Copy operator (disabled).
  VMCSingleOMP& operator=(const VMCSingleOMP&)
  {
    return *this;
  }
};
}

#endif
/***************************************************************************
 * $RCSfile: VMCSingleOMP.h,v $   $Author: jnkim $
 * $Revision: 1.5 $   $Date: 2006/07/17 14:29:40 $
 * $Id: VMCSingleOMP.h,v 1.5 2006/07/17 14:29:40 jnkim Exp $
 ***************************************************************************/
