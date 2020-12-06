#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 130-jhbuild-bootstrap.sh ###
# Bootstrap the JHBuild environment.

### settings and functions #####################################################

for script in $(dirname ${BASH_SOURCE[0]})/0??-*.sh; do source $script; done

### bootstrap JHBuild ##########################################################

mkdir -p $PKG_DIR $XDG_CACHE_HOME

# Basic bootstrapping.
jhbuild bootstrap-gtk-osx

# Install Meson build system.
jhbuild build python3
jhbuild run pip3 install $PYTHON_MESON

# Install Ninja build systems.
download_url $NINJA_URL $PKG_DIR
unzip -d $BIN_DIR $PKG_DIR/$(basename $NINJA_URL)