#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### build_inkscape.sh ###
# Compile and package Inkscape.

### settings and functions #####################################################

for script in $(dirname ${BASH_SOURCE[0]})/0??-*.sh; do source $script; done

set -e   # break if one of the called scripts ends in error

### run scripts to compile and build Inkscape ##################################

for script in $SELF_DIR/2??-*.sh; do
  $script
done
