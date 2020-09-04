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

### install JHBuild ############################################################

install_source $URL_JHBUILD
JHBUILD_DIR=$(pwd)

mkdir $BIN_DIR

# Create 'jhbuild' executable. This code has been adapted from
# https://gitlab.gnome.org/GNOME/gtk-osx/-/blob/master/gtk-osx-setup.sh

cat <<EOF > "$BIN_DIR/jhbuild"
#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
# Python 3
#import builtins
import __builtin__

sys.path.insert(0, '$JHBUILD_DIR')
pkgdatadir = None
datadir = None
import jhbuild
srcdir = os.path.abspath(os.path.join(os.path.dirname(jhbuild.__file__), '..'))

# Python 3
#builtins.__dict__['PKGDATADIR'] = pkgdatadir
#builtins.__dict__['DATADIR'] = datadir
#builtins.__dict__['SRCDIR'] = srcdir
__builtin__.__dict__['PKGDATADIR'] = pkgdatadir
__builtin__.__dict__['DATADIR'] = datadir
__builtin__.__dict__['SRCDIR'] = srcdir

import jhbuild.main
jhbuild.main.main(sys.argv[1:])
EOF

chmod 755 $BIN_DIR/jhbuild

### configure JHBuild ##########################################################

download_url $URL_GTK_OSX/jhbuildrc-gtk-osx \
  $(dirname $JHBUILDRC) \
  $(basename $JHBUILDRC)

download_url $URL_GTK_OSX/jhbuildrc-gtk-osx-custom-example \
  $(dirname $JHBUILDRC_CUSTOM) \
  $(basename $JHBUILDRC_CUSTOM)

# JHBuild: paths
echo "buildroot = '$JHBUILD_BUILDROOT'" >> $JHBUILDRC_CUSTOM
echo "checkoutroot = '$SRC_DIR'"        >> $JHBUILDRC_CUSTOM
echo "prefix = '$VER_DIR'"              >> $JHBUILDRC_CUSTOM
echo "tarballdir = '$PKG_DIR'"          >> $JHBUILDRC_CUSTOM

# JHBuild: console output
echo "quiet_mode = True"   >> $JHBUILDRC_CUSTOM
echo "progress_bar = True" >> $JHBUILDRC_CUSTOM

# JHBuild: moduleset
echo "moduleset = '$URL_GTK_OSX_MODULESET'" >> $JHBUILDRC_CUSTOM

# JHBuild: macOS SDK
sed -i "" "s/^setup_sdk/#setup_sdk/"                      $JHBUILDRC_CUSTOM
echo "setup_sdk(target=\"$MACOSX_DEPLOYMENT_TARGET\")" >> $JHBUILDRC_CUSTOM
echo "os.environ[\"SDKROOT\"]=\"$SDKROOT\""            >> $JHBUILDRC_CUSTOM

# JHBuild: TODO: I have forgotten why this is here
echo "if \"openssl\" in skip:"    >> $JHBUILDRC_CUSTOM
echo "  skip.remove(\"openssl\")" >> $JHBUILDRC_CUSTOM

# optional: use ccache if available
# (But it's not as effective here as it is for building Inkscape.)
if [ -d $CCACHE_BIN_DIR ]; then
  echo_i "JHBuild will use ccache"
  echo "os.environ[\"CC\"] = \"$CCACHE_BIN_DIR/gcc\""  >> $JHBUILDRC_CUSTOM
  echo "os.environ[\"CXX\"] = \"$CCACHE_BIN_DIR/g++\"" >> $JHBUILDRC_CUSTOM
fi
