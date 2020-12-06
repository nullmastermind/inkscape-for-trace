#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 110-sysprep.sh ###
# System preparation tasks.

### settings and functions #####################################################

for script in $(dirname ${BASH_SOURCE[0]})/0??-*.sh; do source $script; done

### initial information ########################################################

echo_i "WRK_DIR = $WRK_DIR"
echo_i "VER_DIR = $VER_DIR"

### check for presence of SDK ##################################################

if [ ! -d $SDKROOT ]; then
  echo_e "SDK not found: $SDKROOT"
  exit 1
fi

### create directories #########################################################

for dir in $VER_DIR $TMP_DIR $HOME; do
  mkdir -p $dir
done

### install ccache #############################################################

install_source $CCACHE_URL
configure_make_makeinstall

cd $BIN_DIR
ln -s ccache clang
ln -s ccache clang++
ln -s ccache gcc
ln -s ccache g++

configure_ccache $CCACHE_SIZE  # create directory and config file