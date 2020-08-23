#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 130-jhbuild-bootstrap.sh ###
# Bootstrap the JHBuild environment.

### load global settings and functions #########################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

### bootstrap JHBuild ##########################################################

mkdir -p $PKG_DIR $XDG_CACHE_HOME

# Basic bootstrapping.
jhbuild bootstrap-gtk-osx

# Install Meson build system.
jhbuild build python3
jhbuild run pip3 install $PYTHON_MESON

# Install Ninja build systems.
download_url $URL_NINJA $PKG_DIR
unzip -d $BIN_DIR $PKG_DIR/$(basename $URL_NINJA)