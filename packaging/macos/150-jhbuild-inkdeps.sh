#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 150-jhbuild-inkdeps.sh ###
# Install additional dependencies into our jhbuild environment required for
# building Inkscape.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install Inkscape dependencies ##############################################

# Part of gtk-osx module sets.

jhbuild build \
  gtkspell3 \
  libsoup

# Part of inkscape module set.

jhbuild build \
  gsl \
  boehm_gc \
  gdl \
  openjpeg \
  libcdr \
  imagemagick \
  poppler \
  double_conversion \
  potrace \
  openmp \
  ghostscript \
  google_test
