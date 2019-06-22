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
  git clone --recurse-submodules --depth 10 $URL_INKSCAPE $INK_DIR
  #git clone --recurse-submodules $URL_INKSCAPE $INK_DIR   # >1.6 GiB download
fi

[ -d $INK_DIR.build ] && rm -rf $INK_DIR.build   # cleanup previous run

mkdir -p $INK_DIR.build
cd $INK_DIR.build

# about the '-DOpenMP_*' arguments:
# All the settings for OpenMP are to trigger the detection during 'cmake'.
# Experimenting with a "Hello World"-style example shows that linking with
# '-lomp' would suffice, no '-fopenmp' required.
# TODO further investigation into required flags

cmake \
  -DCMAKE_PREFIX_PATH=$OPT_DIR \
  -DCMAKE_INSTALL_PREFIX=$OPT_DIR \
  -DOpenMP_CXX_FLAGS="-Xclang -fopenmp" \
  -DOpenMP_C_FLAGS="-Xclang -fopenmp" \
  -DOpenMP_CXX_LIB_NAMES="omp" \
  -DOpenMP_C_LIB_NAMES="omp" \
  -DOpenMP_omp_LIBRARY=$LIB_DIR/libomp.dylib \
  $INK_DIR

make
make install

# patch Poppler library locations
install_name_tool -change @rpath/libpoppler.85.dylib $LIB_DIR/libpoppler.85.dylib $BIN_DIR/inkscape
install_name_tool -change @rpath/libpoppler-glib.8.dylib $LIB_DIR/libpoppler-glib.8.dylib $BIN_DIR/inkscape
install_name_tool -change @rpath/libpoppler.85.dylib $LIB_DIR/libpoppler.85.dylib $LIB_DIR/inkscape/libinkscape_base.dylib
install_name_tool -change @rpath/libpoppler-glib.8.dylib $LIB_DIR/libpoppler-glib.8.dylib $LIB_DIR/inkscape/libinkscape_base.dylib

# patch OpenMP library locations
install_name_tool -change @rpath/libomp.dylib $LIB_DIR/libomp.dylib $BIN_DIR/inkscape
install_name_tool -change @rpath/libomp.dylib $LIB_DIR/libomp.dylib $LIB_DIR/inkscape/libinkscape_base.dylib

