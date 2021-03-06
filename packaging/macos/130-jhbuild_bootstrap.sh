#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# This file is part of the build pipeline for Inkscape on macOS.

### description ################################################################

# The first step to perform with JHBuild is to run its bootstrap command.
# After that it can be freely used to build whatever we want.
# JHBuild has two external dependencies - Meson and Ninja - that it does not
# fetch on its own, so we install them as well.

### includes ###################################################################

# shellcheck disable=SC1090 # can't point to a single source here
for script in "$(dirname "${BASH_SOURCE[0]}")"/0??-*.sh; do
  source "$script";
done

### settings ###################################################################

# Nothing here.

### main #######################################################################

#------------------------------------------------------------- bootstrap JHBuild

mkdir -p "$XDG_CACHE_HOME"

# Basic bootstrapping.
jhbuild bootstrap-gtk-osx
