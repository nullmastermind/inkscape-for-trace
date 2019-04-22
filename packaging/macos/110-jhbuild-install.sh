#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 110-jhbuild-install.sh ###
# Install the JHBuild tool.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### create our work directory ##################################################

[ ! -d $WRK_DIR ] && mkdir -p $WRK_DIR

# Use a ramdisk to speed up things.
if $RAMDISK_ENABLE; then
  create_ramdisk $WRK_DIR $RAMDISK_SIZE
fi

### setup path #################################################################

# WARNING: Operations like this are the reason that you're supposed to use
# a dedicated machine for building. This script does not care for your
# data.
echo "export PATH=$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin" > $HOME/.profile
source $HOME/.profile

### setup directories for jhbuild ##############################################

mkdir -p $TMP_DIR
mkdir -p $SRC_DIR/checkout     # extracted tarballs
mkdir -p $SRC_DIR/download     # downloaded tarballs

# WARNING: Operations like this are the reason that you're supposed to use
# a dedicated machine for building. This script does not care for your
# data.
rm -rf $HOME/.cache
rm -rf $HOME/.local
ln -sf $TMP_DIR $HOME/.cache   # link to our workspace
ln -sf $OPT_DIR $HOME/.local   # link to our workspace

### install and configure jhbuild ##############################################

# remove configuration files from a previous installation
rm $HOME/.jhbuild*

# install jhbuild
cd $WRK_DIR
export SRC_DIR   # used as '$SOURCE' inside jhbuild
bash <(curl -s $URL_GTK_OSX_BUILD_SETUP)   # run jhbuild setup script

# configure jhbuild
JHBUILDRC=$HOME/.jhbuildrc-custom
echo "checkoutroot = '$SRC_DIR/checkout'"   >> $JHBUILDRC
echo "prefix = '$OPT_DIR'"                  >> $JHBUILDRC
echo "tarballdir = '$SRC_DIR/download'"     >> $JHBUILDRC
echo "quiet_mode = True"                    >> $JHBUILDRC    # suppress output
echo "progress_bar = True"                  >> $JHBUILDRC
echo "moduleset = '$URL_GTK_OSX_MODULESET'" >> $JHBUILDRC    # custom moduleset

