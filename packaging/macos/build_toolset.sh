#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### build_toolset.sh ###
# Create JHBuild toolset with all dependencies for Inkscape.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); \
  cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### download pre-built build environment or build from scratch #################

if [ -d $OPT_DIR ]; then
  echo "$SELF_NAME: $OPT_DIR exists - not doing anything"
else
  if $TOOLSET_CACHE_ENABLE &&
    [ "$WRK_DIR" = "$DEFAULT_WRK_DIR" ]; then   # we're good to download
    # Current configuration uses WRK_DIR for both the toolset (WRK_DIR/opt) 
    # and GitLab runner's build directory (WRK_DIR/build). Creating a ramdisk
    # in WRK_DIR during CI would shadow that.
    # Until a better solution is found, the ramdisk has to be manually created
    # on the runner and is disabled on CI.
    [ ! -z $CI_JOB_ID ] && RAMDISK_ENABLE=false
    $SELF_DIR/110-sysprep.sh
    get_source $URL_TOOLSET_CACHE $WRK_DIR "--exclude opt/src/boost* --exclude opt/src/checkout --exclude opt/tmp/jhbuild/build"
    mkdir -p $HOME/.config
    rm -f $HOME/.config/jhbuild*
    ln -sf $DEVCONFIG/jhbuild* $HOME/.config
  else  # we need to build from scratch
    for script in $SELF_DIR/1??-*.sh; do
      $script
    done
  fi
fi
