#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### build_all.sh ###
# "from nothing to Inkscape.app"

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### main #######################################################################

$SELF_DIR/build_toolset.sh
$SELF_DIR/build_inkscape.sh

