#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# This file is part of the build pipeline for Inkscape on macOS.

### description ################################################################

# Setup JHBuild.

### includes ###################################################################

# shellcheck disable=SC1090 # can't point to a single source here
for script in "$(dirname "${BASH_SOURCE[0]}")"/0??-*.sh; do
  source "$script";
done

### settings ###################################################################

error_trace_enable

### main #######################################################################

jhbuild_install
jhbuild_configure
