#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 160-jhbuild-other.sh ###
# Install additional components that are not direct dependencies, like tools
# required for packaging.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

include_file error_.sh
error_trace_enable

### install disk image creator #################################################

jhbuild run pip3 install $PYTHON_DMGBUILD

### download icon for disk image ###############################################

# dmgbuild offers to generate a badged version of the disk image icon, but
# it doesn't work and I have not investigated yet. We use a pre-made image
# for now.

download_url $URL_INKSCAPE_DMG_ICNS $SRC_DIR

### install gtk-mac-bundler ####################################################

(
  export GMB_BINDIR=$BIN_DIR

  install_source $URL_GTK_MAC_BUNDLER
  jhbuild run make install
)

### install svg to png convertor ###############################################

jhbuild run pip3 install $PYTHON_CAIROSVG
jhbuild run pip3 install $PYTHON_CAIROCFFI

### install png to icns converter ##############################################

install_source $URL_PNG2ICNS
ln -s $(pwd)/png2icns.sh $BIN_DIR

### downlaod a pre-built Python.framework ######################################

# This will be bundled with the application.

download_url $URL_PYTHON $PKG_DIR

################################################################################

# vim: expandtab:shiftwidth=2:tabstop=2:softtabstop=2 :
