#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 120-jhbuild-python2.sh ###
# Install a current complete release of Python 2 w/SSL.

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install OpenSSL ############################################################

# Build OpenSSL as dedicated step because we need to link our system
# configuration and certs (/etc/ssl) to it. Otherwise https downloads
# will fail with certification validation issues.

jhbuild build openssl
mkdir -p $OPT_DIR/etc
ln -sf /etc/ssl $OPT_DIR/etc   # link system config to our OpenSSL

### install Python 2 ###########################################################

# Some packages complain about non-exiting development headers when you rely
# solely on the macOS-provided Python installation. This also enables 
# system-wide installations of packages without permission issues.

jhbuild build python

cd $SRC_DIR
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
jhbuild run python get-pip.py
jhbuild run pip install six   # required for a package in meta-gtk-osx-bootstrap
