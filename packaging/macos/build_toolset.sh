#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### build_toolset.sh ###
# Create JHBuild toolset with all dependencies for Inkscape.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### download pre-built build environment or build from scratch #################

if [ -d $OPT_DIR ]; then
  echo "$SELF_NAME: $OPT_DIR exists - not doing anything"
else
  if $TOOLSET_CACHE_ENABLE &&
     [ "$WRK_DIR" = "$DEFAULT_SYSTEM_WRK_DIR" ]; then   # we're good to download
    $SELF_DIR/110-jhbuild-install.sh
    get_source $URL_TOOLSET_CACHE $WRK_DIR
  else  # we need to build from scratch
    for script in $SELF_DIR/1??-*.sh; do
      $script
    done
  fi
fi
