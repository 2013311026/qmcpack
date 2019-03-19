#!/bin/bash

################################################################
## * This script builds available configurations of QMCPACK   ##
##   on CADES SHPC Condos , at Oak Ridge National Lab.        ##
##                                                            ##
## * Execute this script in trunk/                            ##
##   ./config/build_ornl_cades.sh                             ##
##                                                            ##
## Last verified: Mar 12, 2018                                ##
################################################################

# module files resulting from module imports below:
# Currently Loaded Modulefiles:
#   1) intel/18.0.0           4) gcc/6.3.0              7) cmake/3.11.0
#   2) openmpi/3.1.1          5) hdf5_parallel/1.10.3   8) boost/1.67.0-pe3
#   3) PE-intel/3.0           6) fftw/3.3.5-pe3         9) libxml2/2.9.9

source $MODULESHOME/init/bash
module purge
module load PE-intel/3.0
module load intel/18.0.0
module load gcc/6.3.0
module load hdf5_parallel/1.10.3
module load fftw/3.3.5-pe3
module load cmake
module load boost/1.67.0-pe3
module load libxml2/2.9.9
module list

# Set environment variables
export FFTW_HOME=$FFTW_DIR/..
export BOOST_ROOT=$BOOST_DIR

CMAKE_FLAGS="-DCMAKE_C_COMPILER=mpicc \
             -DCMAKE_CXX_COMPILER=mpicxx \
             -DCMAKE_C_FLAGS=-xCOMMON-AVX512 \
             -DCMAKE_CXX_FLAGS=-xCOMMON-AVX512 \
             -DHAVE_MKL=1"

# Configure and build cpu real AoS
echo ""
echo ""
echo "building QMCPACK for cpu AoS real for CADES SHPC Condo"
mkdir -p build_cades_cpu_real
cd build_cades_cpu_real
cmake $CMAKE_FLAGS ..
make -j 16
cd ..
ln -sf ./build_cades_cpu_real/bin/qmcpack ./qmcpack_cades_cpu_real

# Configure and build cpu complex AoS
echo ""
echo ""
echo "building QMCPACK for cpu AoS complex for CADES SHPC Condo"
mkdir -p build_cades_cpu_comp
cd build_cades_cpu_comp
cmake -DQMC_COMPLEX=1 $CMAKE_FLAGS ..
make -j 16
cd ..
ln -sf ./build_cades_cpu_comp/bin/qmcpack ./qmcpack_cades_cpu_comp

# Configure and build cpu real SoA
echo ""
echo ""
echo "building QMCPACK for cpu SoA real for CADES SHPC Condo"
mkdir -p build_cades_cpu_real_SoA
cd build_cades_cpu_real_SoA
cmake -DENABLE_SOA=1 $CMAKE_FLAGS ..
make -j 16
cd ..
ln -sf ./build_cades_cpu_real_SoA/bin/qmcpack ./qmcpack_cades_cpu_real_SoA

# Configure and build cpu complex SoA
echo ""
echo ""
echo "building QMCPACK for cpu SoA complex for CADES SHPC Condo"
mkdir -p build_cades_cpu_comp_SoA
cd build_cades_cpu_comp_SoA
cmake -DQMC_COMPLEX=1 -DENABLE_SOA=1 $CMAKE_FLAGS ..
make -j 16
cd ..
ln -sf ./build_cades_cpu_comp_SoA/bin/qmcpack ./qmcpack_cades_cpu_comp_SoA

