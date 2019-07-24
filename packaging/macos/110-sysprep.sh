#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 110-sysprep.sh ###
# System preparation tasks.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

run_annotated

### create our work directory ##################################################

[ ! -d $WRK_DIR ] && mkdir -p $WRK_DIR

# Use a ramdisk to speed up things.
if $RAMDISK_ENABLE; then
  create_ramdisk $WRK_DIR $RAMDISK_SIZE
fi

### housekeeping: redirect to locations below WRK_DIR ##########################

mkdir -p $TMP_DIR

rm -rf $HOME/.cache   # used by jhbuild
ln -sf $TMP_DIR $HOME/.cache

rm -rf $HOME/.local   # used by gtk-mac-bundler
ln -sf $OPT_DIR $HOME/.local

### setup path #################################################################

# WARNING: Operations like this are the reason why you're supposed to use
# a dedicated user for building.

echo "export PATH=$DEVPREFIX/bin:$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin" > $HOME/.profile
echo "export LANG=de_DE.UTF-8" >> $HOME/.profile   # jhbuild complains otherwise

