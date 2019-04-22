#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 230-inkscape-distrib.sh ###
# Get application ready for distribution.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### create disk image for distribution #########################################

# TODO: *.dmg, signing, notarization, ...

if [ ! -z $CI_JOB_ID ]; then   # create build artifcat for CI job
  cd $WRK_DIR
  mv $ARTIFACT_DIR $SELF_DIR/../../build
fi
