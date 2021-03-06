#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# This file is part of the build pipeline for Inkscape on macOS.

### description ################################################################

# This script performs system preparation tasks - basically the tings we need
# to take care of before even setting up JHBuild. Performing checks
# like running check_sys_ver are not part of this file (see bottom of
# 010-init.sh), as that would mean that those checks are only run exactly once,
# which is not what we want.

### includes ###################################################################

# shellcheck disable=SC1090 # can't point to a single source here
for script in "$(dirname "${BASH_SOURCE[0]}")"/0??-*.sh; do
  source "$script";
done

### settings ###################################################################

# Nothing here.

### main #######################################################################

#---------------------------------------------- print main directory and version

echo_i "WRK_DIR = $WRK_DIR"
echo_i "VER_DIR = $VER_DIR"

#------------------------------------------------------------ create directories

# We need these directories early on, so we need to create them here.

mkdir -p "$HOME"
mkdir -p "$PKG_DIR"
mkdir -p "$SRC_DIR"
mkdir -p "$TMP_DIR"

#---------------------------------------------------------------- install ccache

ccache_v4_install
ccache_configure

#------------------------------------------ log relevant versions to release.log

sys_ver_log
