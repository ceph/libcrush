#!/bin/bash

set -ex
apt-get update
apt-get install -y git cmake g++
git submodule sync
git submodule update --force --init --recursive
mkdir build
#
# building
#
cd build
cmake ..
make VERBOSE=1
#
# testing
#
ctest -V
#
# distributing
#
make VERBOSE=1 dist
distdir=/var/www/html/$CI_BUILD_REF_NAME
mkdir -p $distdir
cp *.tar.gz $distdir
