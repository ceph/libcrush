#!/bin/bash

set -ex
[ $(id -u) = 0 ] || SUDO=sudo
if hash apt-get 2> /dev/null;
then
    $SUDO apt-get update
    $SUDO apt-get install -y git cmake g++ doxygen
else
    $SUDO yum update -y
    $SUDO yum install -y git cmake gcc-c++ doxygen
fi
git submodule sync
git submodule update --force --init --recursive
mkdir build
(
   #
   # building
   #
   cd build
   cmake ..
   make VERBOSE=1 all
   #
   # testing
   #
   ctest -V
   #
   # source distribution and documentation
   #
   make VERBOSE=1 dist doc
)
if test -n "$CI_BUILD_REF_NAME" ; then
    #
    # save the build
    #
    $SUDO apt-get install -y rsync
    builddir=/var/www/builds/$CI_BUILD_REF_NAME/
    mkdir -p $builddir
    rsync -av build/ $builddir
fi
