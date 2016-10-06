//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//                    Jeremy McMinnis, jmcminis@gmail.com, University of Illinois at Urbana-Champaign
//                    Ye Luo, yeluo@anl.gov, Argonne National Laboratory
//
// File created by: Jeongnim Kim, jeongnim.kim@gmail.com, University of Illinois at Urbana-Champaign
//////////////////////////////////////////////////////////////////////////////////////
    
    

#include "einspline/multi_bspline.h"
#include "einspline/multi_nubspline.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#ifdef _OPENMP
  #include <omp.h>
#endif

double drand48();

int main(int argc, char**argv)
{

  // First, create multispline 32 state on 108^3 
  int Nx=108; int Ny=108; int Nz = 108;
  int num_splines = 32;

  Ugrid x_grid, y_grid, z_grid;
  x_grid.start = 0.0e0; x_grid.end = 1.0e0; x_grid.num = Nx;
  y_grid.start = 0.0e0; y_grid.end = 1.0e0; y_grid.num = Ny;
  z_grid.start = 0.0e0; z_grid.end = 1.0e0; z_grid.num = Nz;

  BCtype_d xBC, yBC, zBC;
  xBC.lCode = xBC.rCode = PERIODIC;
  yBC.lCode = yBC.rCode = PERIODIC;
  zBC.lCode = zBC.rCode = PERIODIC;

  multi_UBspline_3d_d * multi_spline = create_multi_UBspline_3d_d (x_grid, y_grid, z_grid, xBC, yBC, zBC, num_splines);

  double data[Nx*Ny*Nz];
  // Now, create normal splines and set multispline data
  for (int i=0; i<num_splines; i++) {
    for (int j=0; j<Nx*Ny*Nz; j++) data[j] = (drand48()-0.5);// + (drand48()-0.5)*1.0i;
    set_multi_UBspline_3d_d (multi_spline, i, data);
  }

  //read the trace file generated by a run
  FILE *infile = fopen("bspline.0.dat","r");
  char line[256];
  int lcount=0;

  char mytag[3];
  int iat;
  double x,y,z;
  
  double v[num_splines],vg[num_splines*3],vh[num_splines*9];

  while( fgets(line, sizeof(line), infile) != NULL)
  {
    sscanf(line,"%s %d %lf %lf %lf\n",mytag,&iat,&x,&y,&z);
    if(!strcmp(mytag,"EVAL"))
    {
      eval_multi_UBspline_3d_d (multi_spline,x,y,z,v);
    }
    else
    {
      eval_multi_UBspline_3d_d_vgh(multi_spline,x,y,z,v,vg,vh);
    }
  }

  return 0;
}
