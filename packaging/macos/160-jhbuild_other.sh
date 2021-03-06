#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# This file is part of the build pipeline for Inkscape on macOS.

### description ################################################################

# Install additional tools that are no direct dependencies of Inkscape but that
# we need for e.g. packaging the application.

### includes ###################################################################

# shellcheck disable=SC1090 # can't point to a single source here
for script in "$(dirname "${BASH_SOURCE[0]}")"/0??-*.sh; do
  source "$script";
done

### settings ###################################################################

error_trace_enable

### main #######################################################################

#---------------------------------------------------- install GNU Find Utilities

# We need this because the 'find' provided by macOS does not see the files
# in the lower (read-only) layer when we union-mount a ramdisk ontop of it.

jhbuild build findutils

#---------------------------------------------------- install disk image creator

dmgbuild_install

#-------------------------------------------- install application bundle creator

jhbuild build gtkmacbundler

#------------------------------------------------- install svg to icns convertor

svg2icns_install

#---------------------------- downlaod Python runtime to be bundled with the app

ink_python_download
