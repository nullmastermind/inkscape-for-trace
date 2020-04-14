#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 130-jhbuild-bootstrap.sh ###
# Bootstrap the JHBuild environment.

### load global settings and functions #########################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

### local settings #############################################################

export PYTHONUSERBASE=$DEVPREFIX
export PIP_CONFIG_DIR=$DEVROOT/pip

### install and configure jhbuild ##############################################

bash <(curl -s $URL_GTK_OSX_SETUP)   # run jhbuild setup script

# JHBuild: paths
echo "checkoutroot = '$SRC_DIR/checkout'" >> $JHBUILDRC_CUSTOM
echo "prefix = '$OPT_DIR'"                >> $JHBUILDRC_CUSTOM
echo "tarballdir = '$SRC_DIR/download'"   >> $JHBUILDRC_CUSTOM

# JHBuild: console output
echo "quiet_mode = True"   >> $JHBUILDRC_CUSTOM
echo "progress_bar = True" >> $JHBUILDRC_CUSTOM

# JHBuild: moduleset
echo "moduleset = '$URL_GTK_OSX_MODULESET'" >> $JHBUILDRC_CUSTOM

# JHBuild: macOS SDK
sed -i "" "s/^setup_sdk/#setup_sdk/"                      $JHBUILDRC_CUSTOM
echo "setup_sdk(target=\"$MACOSX_DEPLOYMENT_TARGET\")" >> $JHBUILDRC_CUSTOM
echo "os.environ[\"SDKROOT\"]=\"$SDKROOT\""            >> $JHBUILDRC_CUSTOM

# JHBuild: TODO: I have forgotten why this is here
echo "if \"openssl\" in skip:"    >> $JHBUILDRC_CUSTOM
echo "  skip.remove(\"openssl\")" >> $JHBUILDRC_CUSTOM

### bootstrap JHBuild ##########################################################

jhbuild bootstrap-gtk-osx
