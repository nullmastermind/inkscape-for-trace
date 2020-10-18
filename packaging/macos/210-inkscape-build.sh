#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 210-inkscape-build.sh ###
# Build Inscape.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

include_file ansi_.sh
include_file error_.sh
error_trace_enable

ANSI_TERM_ONLY=false   # use ANSI control characters even if not in terminal

### build Inkscape #############################################################

if [ -z $CI_JOB_ID ]; then   # running standalone
  git clone --recurse-submodules --depth 10 $URL_INKSCAPE $INK_DIR
fi

if [ -d $INK_BUILD_DIR ]; then   # cleanup previous run
  rm -rf $INK_BUILD_DIR
fi

mkdir -p $INK_BUILD_DIR
cd $INK_BUILD_DIR

cmake \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_PREFIX_PATH=$VER_DIR \
  -DCMAKE_INSTALL_PREFIX=$VER_DIR \
  $INK_DIR

make
make install
make tests

### patch Poppler library locations ############################################

lib_change_path \
  $LIB_DIR/libpoppler.94.dylib \
  $BIN_DIR/inkscape \
  $LIB_DIR/inkscape/libinkscape_base.dylib

lib_change_path \
  $LIB_DIR/libpoppler-glib.8.dylib \
  $BIN_DIR/inkscape \
  $LIB_DIR/inkscape/libinkscape_base.dylib

### patch OpenMP library locations #############################################

lib_change_path \
  $LIB_DIR/libomp.dylib \
  $BIN_DIR/inkscape \
  $LIB_DIR/inkscape/libinkscape_base.dylib
