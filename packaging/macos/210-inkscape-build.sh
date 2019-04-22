#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 210-inkscape-build.sh ###
# Build Inscape.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### build Inkscape #############################################################

if [ -z $CI_JOB_ID ]; then   # running standalone
  cd $SRC_DIR
  git clone --recurse-submodules --depth 10 $URL_INKSCAPE
  #git clone --recurse-submodules $URL_INKSCAPE   # this is a >1.6 GiB download
  mkdir inkscape_build
  cd inkscape_build
  cmake -DCMAKE_PREFIX_PATH=$OPT_DIR -DCMAKE_INSTALL_PREFIX=$OPT_DIR -DWITH_OPENMP=OFF ../inkscape
else   # running as CI job
  if [ -d $SELF_DIR/../../build ]; then   # cleanup previous run
    rm -rf $SELF_DIR/../../build
  fi
  mkdir $SELF_DIR/../../build
  cd $SELF_DIR/../../build
  cmake -DCMAKE_PREFIX_PATH=$OPT_DIR -DCMAKE_INSTALL_PREFIX=$OPT_DIR -DWITH_OPENMP=OFF ..
fi

make
make install

# patch library locations before packaging
install_name_tool -change @rpath/libpoppler.85.dylib $LIB_DIR/libpoppler.85.dylib $BIN_DIR/inkscape
install_name_tool -change @rpath/libpoppler-glib.8.dylib $LIB_DIR/libpoppler-glib.8.dylib $BIN_DIR/inkscape

