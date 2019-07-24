#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 120-python3.sh ###
# Install a dedicated Python 3 for JHBuild because it fails to install
# a working Python by itself.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

run_annotated

### install Python 3 ###########################################################

get_source $URL_PYTHON3_SRC

sed -i "" '/^FRA_DIR/s/.*/FRA_DIR=$WRK_DIR/' 020-vars.sh

./110-build.sh
./310-package-fixed.sh

cd $SELF_DIR
rm -rf $OPT_DIR
mkdir -p $BIN_DIR
mkdir -p $TMP_DIR
ln -sf $WRK_DIR/Python.framework/Versions/Current/bin/python3 $BIN_DIR
ln -sf $BIN_DIR/python3 $BIN_DIR/python

