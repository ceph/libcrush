#!/bin/bash

set -ex
cd /var/www/builds/$CI_BUILD_REF_NAME
releasedir=/var/www/html/releases
mkdir -p $releasedir
cp *.tar.gz $releasedir
