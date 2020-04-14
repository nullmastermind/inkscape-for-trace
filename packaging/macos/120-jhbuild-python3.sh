#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 120-jhbuild-python3.sh ###
# Install a dedicated Python 3 for JHBuild because it fails to install
# a working Python by itself.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install Python 3 ###########################################################

install_source $URL_PYTHON_JHBUILD $WRK_DIR

ln -sf $WRK_DIR/Python.framework/Versions/Current/bin/python3 $BIN_DIR
