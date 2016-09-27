//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//                    Jeremy McMinnis, jmcminis@gmail.com, Navar Inc.
//
// File created by: Jeongnim Kim, jeongnim.kim@intel.com, Intel Inc.
//////////////////////////////////////////////////////////////////////////////////////
    
    
#ifndef QMCPLUSPLUS_NUMERICALRADIALGRIDFUNCTOR_H
#define QMCPLUSPLUS_NUMERICALRADIALGRIDFUNCTOR_H

#include "QMCWaveFunctions/MolecularOrbitals/RGFBuilderBase.h"

namespace qmcplusplus
{

/**Class to create a set of radial orbitals on a grid (e.g., AtomHF/Siesta)
 *
 * The grid and orbitals are stored in HDF5 format.
 */
struct NumericalRGFBuilder: public RGFBuilderBase
{
  ///constructor
  NumericalRGFBuilder(xmlNodePtr cur);
  bool putCommon(xmlNodePtr cur);
  bool addGrid(xmlNodePtr cur);
  bool addRadialOrbital(xmlNodePtr cur, const QuantumNumberType& nlms);

};

}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/
