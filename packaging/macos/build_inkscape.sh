#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### build_inkscape.sh ###
# Compile and package Inkscape.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### run scripts to compile and build Inkscape ##################################

for script in $SELF_DIR/2??-*.sh; do
  $script
done
