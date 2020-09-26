#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 120-jhbuild-install.sh ###
# Install and configure JHBuild.

### load global settings and functions #########################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

include_file error_.sh
error_trace_enable

### install ccache #############################################################

install_source $URL_CCACHE
configure_make_makeinstall

cd $BIN_DIR
ln -s ccache clang
ln -s ccache clang++
ln -s ccache gcc
ln -s ccache g++

### install Python certifi package #############################################

# Without this, JHBuild won't be able to access https links later because
# Apple's Python won't be able to validate certificates.

pip3 install --ignore-installed --target=$LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages certifi

### install JHBuild ############################################################

install_source $URL_JHBUILD
JHBUILD_DIR=$(pwd)

# Create 'jhbuild' executable. This code has been adapted from
# https://gitlab.gnome.org/GNOME/gtk-osx/-/blob/master/gtk-osx-setup.sh

cat <<EOF > "$BIN_DIR/jhbuild"
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import builtins

sys.path.insert(0, '$JHBUILD_DIR')
pkgdatadir = None
datadir = None
import jhbuild
srcdir = os.path.abspath(os.path.join(os.path.dirname(jhbuild.__file__), '..'))

builtins.__dict__['PKGDATADIR'] = pkgdatadir
builtins.__dict__['DATADIR'] = datadir
builtins.__dict__['SRCDIR'] = srcdir

import jhbuild.main
jhbuild.main.main(sys.argv[1:])
EOF

chmod 755 $BIN_DIR/jhbuild

### configure JHBuild ##########################################################

mkdir -p $(dirname $JHBUILDRC)
cp $SELF_DIR/jhbuild/$(basename $JHBUILDRC)        $JHBUILDRC
cp $SELF_DIR/jhbuild/$(basename $JHBUILDRC_CUSTOM) $JHBUILDRC_CUSTOM

# set moduleset directory
echo "modulesets_dir = '$SELF_DIR/jhbuild'" >> $JHBUILDRC_CUSTOM

# basic directory layout
echo "buildroot = '$BLD_DIR'"    >> $JHBUILDRC_CUSTOM
echo "checkoutroot = '$SRC_DIR'" >> $JHBUILDRC_CUSTOM
echo "prefix = '$VER_DIR'"       >> $JHBUILDRC_CUSTOM
echo "tarballdir = '$PKG_DIR'"   >> $JHBUILDRC_CUSTOM

# set macOS SDK
echo "setup_sdk(target=\"$SDK_VERSION\")"   >> $JHBUILDRC_CUSTOM
echo "os.environ[\"SDKROOT\"]=\"$SDKROOT\"" >> $JHBUILDRC_CUSTOM

# Remove harmful settings in regards to the target platform:
#   - MACOSX_DEPLOYMENT_TARGET
#   - -mmacosx-version-min
#
#   otool -l <library> | grep -A 3 -B 1 LC_VERSION_MIN_MACOSX
#
#           cmd LC_VERSION_MIN_MACOSX
#       cmdsize 16
#       version 10.11
#           sdk n/a          < - - - notarized app won't load this library
echo "os.environ.pop(\"MACOSX_DEPLOYMENT_TARGET\")" \
    >> $JHBUILDRC_CUSTOM
echo "os.environ[\"CFLAGS\"] = \"-O2 -I$SDKROOT/usr/include -isysroot $SDKROOT\"" \
    >> $JHBUILDRC_CUSTOM
echo "os.environ[\"CPPFLAGS\"] = \"-I$INC_DIR -I$SDKROOT/usr/include -isysroot $SDKROOT\"" \
    >> $JHBUILDRC_CUSTOM
echo "os.environ[\"CXXFLAGS\"] = \"-O2 -I$SDKROOT/usr/include -isysroot $SDKROOT\"" \
    >> $JHBUILDRC_CUSTOM
echo "os.environ[\"LDFLAGS\"] = \"-L$LIB_DIR -L$SDKROOT/usr/lib -isysroot $SDKROOT -Wl,-headerpad_max_install_names\"" \
    >> $JHBUILDRC_CUSTOM
echo "os.environ[\"OBJCFLAGS\"] = \"-O2 -I$SDKROOT/usr/include -isysroot $SDKROOT\"" \
    >> $JHBUILDRC_CUSTOM

# enable ccache
echo "os.environ[\"CC\"] = \"$BIN_DIR/gcc\""   >> $JHBUILDRC_CUSTOM
echo "os.environ[\"OBJC\"] = \"$BIN_DIR/gcc\"" >> $JHBUILDRC_CUSTOM
echo "os.environ[\"CXX\"] = \"$BIN_DIR/g++\""  >> $JHBUILDRC_CUSTOM

# certificates for https
echo "os.environ[\"SSL_CERT_FILE\"] = \"$LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/certifi/cacert.pem\"" \
    >> $JHBUILDRC_CUSTOM
echo "os.environ[\"REQUESTS_CA_BUNDLE\"] = \"$LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/certifi/cacert.pem\"" \
    >> $JHBUILDRC_CUSTOM
