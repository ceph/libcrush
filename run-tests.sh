#!/bin/bash

set -ex
apt-get update
apt-get install -y git cmake g++
git submodule sync
git submodule update --force --init --recursive
mkdir build
cd build
cmake ..
make VERBOSE=1
ctest -V
