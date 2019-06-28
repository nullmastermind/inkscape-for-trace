#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 160-jhbuild-other.sh ###
# Install additional components that are not direct dependencies, like tools
# required for packaging.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install Node.js ############################################################

get_source $URL_NODEJS

(
  export PATH=$PATH:$(pwd)/bin
  npm install -g appdmg   # tool to easily create .dmg
)

