#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 130-jhbuild-bootstrap.sh ###
# Bootstrap the JHBuild environment.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); \
  cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install and configure jhbuild ##############################################

# remove configuration files from a previous installation
rm $HOME/.config/jhbuild* 2>/dev/null

bash <(curl -s $URL_GTK_OSX_SETUP)   # run jhbuild setup script

mv $HOME/.config/jhbuild* $DEVCONFIG
ln -sf $DEVCONFIG/jhbuild* $HOME/.config

( # configure jhbuild
  JHBUILDRC=$DEVCONFIG/jhbuildrc-custom
  echo "checkoutroot = '$SRC_DIR/checkout'"   >> $JHBUILDRC
  echo "prefix = '$OPT_DIR'"                  >> $JHBUILDRC
  echo "tarballdir = '$SRC_DIR/download'"     >> $JHBUILDRC
  echo "quiet_mode = True"                    >> $JHBUILDRC  # suppress output
  echo "progress_bar = True"                  >> $JHBUILDRC
  echo "moduleset = '$URL_GTK_OSX_MODULESET'" >> $JHBUILDRC  # custom moduleset

  # configure SDK
  sed -i "" "s/^setup_sdk/#setup_sdk/" $JHBUILDRC   # disable existing setting
  echo "setup_sdk(target=\"$MACOSX_DEPLOYMENT_TARGET\")" >> $JHBUILDRC
)

# bootstrapping
jhbuild bootstrap-gtk-osx
