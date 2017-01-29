#!/bin/bash

set -ex
env
apt-get update
apt-get install -y doxygen rsync
doxygen doc/Doxygen
rsync --delete -av doc/output/html/ /var/www/doc/$CI_BUILD_REF_NAME/
