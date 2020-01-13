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

get_source $URL_PYTHON36_BIN $WRK_DIR
get_source $URL_PYTHON36_SRC   # we only need 310-package-fixed.sh

sed -i "" "/^WRK_DIR/s/.*/WRK_DIR=$(escape_sed $WRK_DIR)/" 020-vars.sh
sed -i "" '/^FRA_DIR/s/.*/FRA_DIR=$WRK_DIR/' 020-vars.sh

# Using the relocatable version of the framework as starting point,
# we're going to turn the link paths into fixed (non-relocatable) ones.
grep -v "cp " 310-package-fixed.sh > 311-package-fixed-nocp.sh
chmod 755 311-package-fixed-nocp.sh
./311-package-fixed-nocp.sh

ln -sf $WRK_DIR/Python.framework/Versions/Current/bin/python3 $BIN_DIR
