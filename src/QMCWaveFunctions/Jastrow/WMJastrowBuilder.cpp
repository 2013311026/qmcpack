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
    
    
#include "QMCWaveFunctions/Jastrow/WMJastrowBuilder.h"
#include "QMCWaveFunctions/Jastrow/TwoBodyJastrowOrbital.h"
#include "QMCWaveFunctions/Jastrow/OneBodyJastrowOrbital.h"
#include "QMCWaveFunctions/Jastrow/LinearCombinationFunctor.h"
#include "QMCWaveFunctions/Jastrow/CompositeFunctor.h"
#include "QMCWaveFunctions/DiffOrbitalBase.h"
#include "Utilities/IteratorUtility.h"
#include "Utilities/ProgressReportEngine.h"
#include "OhmmsData/AttributeSet.h"
//#include "QMCWaveFunctions/Jastrow/DiffTwoBodyJastrowOrbital.h"
//#include "QMCWaveFunctions/Jastrow/DiffOneBodyJastrowOrbital.h"

namespace qmcplusplus
{

WMJastrowBuilder::WMJastrowBuilder(ParticleSet& target, TrialWaveFunction& psi,
                                   ParticleSet* source):
  OrbitalBuilderBase(target,psi),sourcePtcl(source)
{
  ClassName="WMJastrowBuilder";
}

bool WMJastrowBuilder::put(xmlNodePtr cur)
{
  ReportEngine PRE(ClassName,"put()");
  if(sourcePtcl)
    return addOneBody(cur);
  else
    return addTwoBody(cur);
}

bool WMJastrowBuilder::addTwoBody(xmlNodePtr cur)
{
  cur=cur->children;
  while(cur != NULL)
  {
    std::string cname((const char*)(cur->name));
    if(cname == "basisset")
    {
      addSingleBasisPerSpecies(cur);
    }
    cur=cur->next;
  }
  if(BasisGroups.empty())
  {
    app_error() << "  WMJastrowBuilder::createTwoBody fails to create a TwoBodyJastrow "
                << " due to missing <basisset/> " << std::endl;
    return 0;
  }
  BasisGroupType* curGroup=0;
  BGContainerType::iterator it(BasisGroups.find(targetPtcl.getName()));
  if(it == BasisGroups.end())
  {
    return false;
  }
  else
  {
    curGroup=(*it).second;
  }
  InFuncType* infunc=curGroup->In_ ;
  OutFuncType* nfunc= new OutFuncType(infunc, curGroup->Rcut, curGroup->NumGridPoints);
  curGroup->Out_ = nfunc;
  if(OHMMS::Controller->rank()==0)
  {
    std::ofstream fout("J2.dat");
    fout.setf(std::ios::scientific, std::ios::floatfield);
    fout << "# Two-body Jastrow generated by WMJastrowBuilder::createTwoBody" << std::endl;
    nfunc->print(fout);
  }
  //create a Jastrow function
  typedef TwoBodyJastrowOrbital<OutFuncType> JeeType;
  JeeType *J2 = new JeeType(targetPtcl,targetPsi.is_manager());
  J2->addFunc(0,0,nfunc);
  targetPsi.addOrbital(J2,"wm_j2");
  //2008-04-07 derivatives not complete
  //typedef DiffTwoBodyJastrowOrbital<DerivFuncType> dJeeType;
  //dJeeType *dJ2 = new dJeeType(targetPtcl);
  //dJ2->addFunc("Jee",0,0,curGroup->Deriv_);
  //dJ2->initialize();
  ////add a derivative function
  //J2->setDiffOrbital(dJ2);
  return true;
}

bool WMJastrowBuilder::addOneBody(xmlNodePtr cur)
{
  std::map<std::string,InFuncType*> jnSet;
  cur=cur->children;
  while(cur != NULL)
  {
    std::string cname((const char*)(cur->name));
    if(cname == "basisset")
    {
      addSingleBasisPerSpecies(cur);
    }
    cur=cur->next;
  }
  int nSpecies = sourcePtcl->getSpeciesSet().getTotalNum();
  typedef OneBodyJastrowOrbital<OutFuncType> JneType;
  JneType* jne=new JneType(*sourcePtcl,targetPtcl);
  //typedef DiffOneBodyJastrowOrbital<DerivFuncType> dJneType;
  //dJneType *djne = new dJneType(source,targetPtcl);
  bool foundit=false;
  SpeciesSet& ion_species(sourcePtcl->getSpeciesSet());
  BGContainerType::iterator jit(BasisGroups.begin()), jit_end(BasisGroups.end());
  while(jit != jit_end)
  {
    int ig=ion_species.findSpecies((*jit).first);
    if(ig < nSpecies) //should not add any species here
    {
      foundit=true;
      BasisGroupType* curG((*jit).second);
      OutFuncType* nfunc= new OutFuncType(curG->In_,curG->Rcut, curG->NumGridPoints);
      jne->addFunc(ig,nfunc);
      curG->Out_ = nfunc;
      //add derivatives
      //djne->addFunc(ig,curG->Deriv_);
      if(OHMMS::Controller->rank()==0)
      {
        char fname[16];
        sprintf(fname,"J1.%s.dat",ion_species.speciesName[ig].c_str());
        std::ofstream fout(fname);
        fout.setf(std::ios::scientific, std::ios::floatfield);
        fout << "# One-body Jastrow " << ion_species.speciesName[ig]
             << " generated by WMJastrowBuilder::createOneBody" << std::endl;
        nfunc->print(fout);
      }
    }
    ++jit;
  }
  //if(foundit)
  //{
  //  djne->initialize();
  //  jne->setDiffOrbital(djne);
  //}
  //else
  //{
  //  delete jne; jne=0;
  //  delete djne;
  //}
  targetPsi.addOrbital(jne,"wm_j1");
  return true;
}

void WMJastrowBuilder::addSingleBasisPerSpecies(xmlNodePtr cur)
{
  RealType rcut=10.0;
  int npts=101;
  RealType step=-1.0;
  //if(myGrid) {
  //  rcut=myGrid->rmax();
  //  npts = myGrid->size();
  //}
  OhmmsAttributeSet gAttrib;
  gAttrib.add(rcut,"rf");
  gAttrib.add(npts,"npts");
  gAttrib.add(step,"step");
  std::string tpname(targetPtcl.getName());
  BasisGroupType* curBG=0;
  cur=cur->children;
  while(cur != NULL)
  {
    std::string cname((const char*)(cur->name));
    std::string elementType("e");
    OhmmsAttributeSet aAttrib;
    aAttrib.add(elementType,"elementType");
    aAttrib.put(cur);
    if(cname == "atomicBasisSet")
    {
      //replace elementType for clones
      if(tpname.find(elementType)<tpname.size())
        elementType=tpname;
      xmlNodePtr cur1=cur->children;
      while(cur1 != NULL)
      {
        std::string cname1((const char*)(cur1->name));
        if(cname1 == "basisGroup")
        {
          curBG=createBasisGroup(cur1,elementType);
          curBG->setGrid(rcut,npts);
          add2BasisGroup(curBG,cur1);
        }
        else
          if(cname1 == "grid")
            gAttrib.put(cur1);
        cur1=cur1->next;
      }
    }
    else
      if(cname == "basisGroup")
      {
        //replace elementType for clones
        if(tpname.find(elementType)<tpname.size())
          elementType=tpname;
        curBG=createBasisGroup(cur,elementType);
        curBG->setGrid(rcut,npts);
        add2BasisGroup(curBG,cur);
      }
      else
        if(cname == "grid")
          gAttrib.put(cur);
    cur=cur->next;
  }
}


WMJastrowBuilder::BasisGroupType*
WMJastrowBuilder::createBasisGroup(xmlNodePtr cur, const std::string& elementType)
{
  ReportEngine PRE(ClassName,"createBasisGroup(...)");
  std::string type("WM");
  RealType cusp=0.0;
  OhmmsAttributeSet aAttrib;
  aAttrib.add(type,"type");
  aAttrib.add(cusp,"cusp");
  aAttrib.put(cur);
  BGContainerType::iterator it(BasisGroups.find(elementType));
  BasisGroupType* curBG=0;
  if(it == BasisGroups.end())
  {
    curBG=new BasisGroupType;
    BasisGroups[elementType]=curBG;
  }
  else
  {
    curBG=(*it).second;
  }
  //overwrite a cusp
  curBG->Cusp=cusp;
  return curBG;
}

void WMJastrowBuilder::add2BasisGroup(BasisGroupType* curBG, xmlNodePtr cur)
{
  ReportEngine PRE(ClassName,"add2BasisGroup(...)");
  typedef LinearCombinationFunctor<RealType> ComboFuncType;
  ComboFuncType* acombo=new ComboFuncType;
  curBG->In_ = acombo;
  //DerivFuncType* aderiv=new DerivFuncType(curBG->Rcut);
  RealType cusp_exponent=2.0;
  std::string radID("0");
  cur=cur->children;
  while(cur != NULL)
  {
    std::string cname((const char*)(cur->name));
    if(cname == "radfunc")
    {
      OhmmsAttributeSet rAttrib;
      std::string rfuncName("WM");
      RealType exponent=1.0;
      RealType contraction=1.0;
      RealType rcut(curBG->Rcut);
      int rpower=0;
      rAttrib.add(radID,"id"); //rAttrib.add(a->B0,"b");
      rAttrib.add(exponent,"exponent");
      rAttrib.add(contraction,"contraction");
      //rAttrib.add(rpower,"node");
      rAttrib.add(rcut,"rcut");
      rAttrib.add(rfuncName,"type");
      rAttrib.put(cur);
      acombo->cutoff_radius=rcut;
      OptimizableFunctorBase *a=0;
      OptimizableFunctorBase *da=0;
      if(rfuncName == "cusp")
      {
        cusp_exponent=exponent;
        curBG->Cusp=contraction;
        rpower=0;//overwrite the power
      }
      else
      {
        a= new WMFunctor<RealType>(exponent,rcut);
        a->put(cur);
        std::string id_c=radID+"_C";
        acombo->addComponent(a,contraction,id_c);
        //add a component to the derivative
        //aderiv->addComponent(contraction,exponent,radID);
      }
      app_log()  << "<radfunc id=\"" << radID << "\" exponent=\""<< exponent
                 << "\" contraction=\"" << contraction
                 << "\" node=\"" << rpower << "\" rcut=\"" << rcut << "\"/>" << std::endl;
    }
    cur=cur->next;
  }
  //non-zero cusp
  if(std::abs(curBG->Cusp)>std::numeric_limits<RealType>::epsilon())
  {
    CuspCorrectionFunctor<RealType> *a  = new CuspCorrectionFunctor<RealType>(cusp_exponent,curBG->Rcut);
    app_log() << "  Adding a cusp term: " << curBG->Cusp << "* (-1/b exp(-br)), b=" << a->E << std::endl;
    std::string cusp_tag=radID+"_cusp"; //non optimizable
    acombo->addComponent(a,curBG->Cusp,cusp_tag,true);//this cannot be modified
  }
  //curBG->Deriv_ = aderiv;
  //add optimizable values now
  //aderiv->addOptimizables(targetPsi.VarList);
}

}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 2930 $   $Date: 2008-07-31 10:30:42 -0500 (Thu, 31 Jul 2008) $
 * $Id: PadeConstraints.cpp 2930 2008-07-31 15:30:42Z jnkim $
 ***************************************************************************/
