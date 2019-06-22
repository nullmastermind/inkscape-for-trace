#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 120-jhbuild-bootstrap.sh ###
# Bootstrap the JHBuild environment.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install OpenSSL ############################################################

# We install OpenSSL first so this version gets used instead instead of the
# system one. (El Capitan uses 0.98)

get_source $URL_OPENSSL
./config --prefix=$OPT_DIR --openssldir=$OPT_DIR/etc/ssl $FLAG_MMACOSXVERSIONMIN
make_makeinstall

curl -o $OPT_DIR/etc/ssl/cert.pem $URL_CURL_CACERT

### bootstrap jhbuild environment ##############################################

jhbuild bootstrap
