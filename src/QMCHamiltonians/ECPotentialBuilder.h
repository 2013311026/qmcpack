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
    
    
#ifndef QMCPLUSPLUS_ECPPOTENTIAL_BUILDER_H
#define QMCPLUSPLUS_ECPPOTENTIAL_BUILDER_H
#include "Configuration.h"
#include "Particle/DistanceTableData.h"
#include "QMCHamiltonians/LocalECPotential.h"
#include "QMCHamiltonians/NonLocalECPotential.h"
namespace qmcplusplus
{

class QMCHamiltonian;
class ParticleSet;
class TrialWaveFunction;

struct ECPotentialBuilder: public MPIObjectBase, public QMCTraits
{

  typedef LocalECPotential::RadialPotentialType RadialPotentialType;
  typedef LocalECPotential::GridType GridType;
  bool hasLocalPot;
  bool hasNonLocalPot;

  QMCHamiltonian&  targetH;
  ParticleSet& IonConfig;
  ParticleSet& targetPtcl;
  TrialWaveFunction& targetPsi;

  std::vector<RealType>  localZeff;
  std::vector<RadialPotentialType*>  localPot;
  std::vector<NonLocalECPComponent*>  nonLocalPot;

  ECPotentialBuilder(QMCHamiltonian& h,
                     ParticleSet& ions, ParticleSet& els, TrialWaveFunction& psi,
                     Communicate* c);

  bool put(xmlNodePtr cur);

  void useSimpleTableFormat();
  void useXmlFormat(xmlNodePtr cur);
};
}
#endif

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/

