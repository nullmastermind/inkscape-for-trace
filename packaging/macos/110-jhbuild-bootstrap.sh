#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 110-jhbuild-bootstrap.sh ###
# Bootstrap the jhbuild environment.

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
echo "export PATH=$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin" > ~/.profile
source ~/.profile

### setup directories for jhbuild ##############################################

mkdir -p $TMP_DIR
mkdir -p $SRC_DIR/checkout     # extracted tarballs
mkdir -p $SRC_DIR/download     # downloaded tarballs

# WARNING: Operations like this are the reason that you're supposed to use
# a dedicated machine for building. This script does not care for your
# data.
rm -rf ~/.cache ~/.local ~/Source   # remove remnants of previous run
ln -sf $TMP_DIR ~/.cache   # link to our workspace
ln -sf $OPT_DIR ~/.local   # link to our workspace
ln -sf $SRC_DIR ~/Source   # link to our workspace

### install and configure jhbuild ##############################################

cd $WRK_DIR
bash <(curl -s $URL_GTK_OSX_BUILD_SETUP)   # run jhbuild setup script

JHBUILDRC=$HOME/.jhbuildrc-custom
if [ -f $JHBUILDRC ]; then   # remove previous configuration from end of file
  LINE_NO=$(grep -n "# And more..." $JHBUILDRC | awk -F ":" '{ print $1 }')
  head -n +$LINE_NO $JHBUILDRC >$JHBUILDRC.clean
  mv $JHBUILDRC.clean $JHBUILDRC
  unset LINE_NO
fi

# configure jhbuild
echo "checkoutroot = '$SRC_DIR/checkout'" >> $JHBUILDRC
echo "prefix = '$OPT_DIR'" >> $JHBUILDRC
echo "tarballdir = '$SRC_DIR/download'" >> $JHBUILDRC
echo "quiet_mode = True" >> $JHBUILDRC    # suppress all build output
echo "progress_bar = True" >> $JHBUILDRC
echo "moduleset = '$URL_GTK_OSX_MODULESET'" >> $JHBUILDRC

### bootstrap jhbuild environment ##############################################

jhbuild bootstrap
