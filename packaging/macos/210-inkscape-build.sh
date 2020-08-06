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

set -e -o errtrace
trap 'catch_error "$SELF_NAME" "$LINENO" "$FUNCNAME" "${BASH_COMMAND}" "${?}"' ERR

### build Inkscape #############################################################

if [ -z $CI_JOB_ID ]; then   # running standalone
  git clone --recurse-submodules --depth 10 $URL_INKSCAPE $INK_DIR
fi

[ -d $INK_DIR.build ] && rm -rf $INK_DIR.build || true  # cleanup previous run

mkdir -p $INK_DIR.build
cd $INK_DIR.build

# about the '-DOpenMP_*' arguments:
# All the settings for OpenMP are to trigger the detection during 'cmake'.
# Experimenting with a "Hello World"-style example shows that linking with
# '-lomp' would suffice, no '-fopenmp' required.
# TODO further investigation into required flags

cmake \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
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
make tests

### patch Poppler library locations ############################################

relocate_dependency $LIB_DIR/libpoppler.94.dylib $BIN_DIR/inkscape
relocate_dependency $LIB_DIR/libpoppler-glib.8.dylib $BIN_DIR/inkscape
relocate_dependency $LIB_DIR/libpoppler.94.dylib $LIB_DIR/inkscape/libinkscape_base.dylib
relocate_dependency $LIB_DIR/libpoppler-glib.8.dylib $LIB_DIR/inkscape/libinkscape_base.dylib

### patch OpenMP library locations #############################################

relocate_dependency $LIB_DIR/libomp.dylib $BIN_DIR/inkscape
relocate_dependency $LIB_DIR/libomp.dylib $LIB_DIR/inkscape/libinkscape_base.dylib
