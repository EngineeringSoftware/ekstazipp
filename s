#!/bin/bash

### This script is used (on xps machines) to build the tool.

# Had to install:
# sudo apt install libpstreams-dev

export EKSTAZI_HOME=$HOME/projects/ekstazipp
export GTEST_HOME=$HOME/opt/googletest-release-1.8.0/googletest

rm -f CMakeCache.txt
rm -rf CMakeFiles/
rm -rf build
mkdir -p build

( cd build
  cmake ..
  make
  cd -
)
