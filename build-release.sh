#!/bin/bash

set -ex
source run-tests.sh
$SUDO apt-get install -y rsync
releasedir=/var/www/html/releases
mkdir -p $releasedir
cp *.tar.gz $releasedir
