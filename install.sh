#!/bin/bash

set -ex
apt-get update
apt-get install -y rsync
cd /var/www/builds/$CI_BUILD_REF_NAME
#
# source
#
distdir=/var/www/html/dist/$CI_BUILD_REF_NAME
mkdir -p $distdir
cp *.tar.gz $distdir
#
# doc
#
rsync --delete -av doc/html/ /var/www/doc/$CI_BUILD_REF_NAME/
