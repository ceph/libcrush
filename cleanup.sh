#!/bin/bash

set -ex
if test "$CI_BUILD_REF_NAME" ; then
    rm -fr /var/www/html/builds/$CI_BUILD_REF_NAME/
fi
