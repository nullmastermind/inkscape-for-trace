#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### install_toolset.sh ###
# Install a pre-compiled version of the JHBuild toolset and required
# dependencies for Inkscape.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); \
  cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

include_file ansi_.sh
include_file echo_.sh
include_file error_.sh
error_trace_enable

ANSI_TERM_ONLY=false   # use ANSI control characters even if not in terminal

### install toolset ############################################################

function install
{
  local toolset_dmg=$REPO_DIR/$(basename $URL_TOOLSET)

  if [ -f $toolset_dmg ]; then
    echo_i "no download required"
  else
    # File not present on disk, we need to download.
    echo_i "download required"
    download_url $URL_TOOLSET $REPO_DIR
  fi

  echo_i "Mounting compressed disk image, this may take some time..."

  # mount build system
  local device=$(create_dmg_device $toolset_dmg)
  if [ ! -d $VER_DIR ]; then
    mkdir -p $VER_DIR
  fi
  mount -o nobrowse,noowners,ro -t hfs $device $VER_DIR
  echo_i "toolset mounted as $device"

  # Sadly, there are some limitations involved with union-mounting:
  #   - Files are not visible to 'ls'.
  #   - You cannot write in a location without having written to its
  #     parent location. That's why we need to pre-create all directories
  #     below.
  #
  # Shadow-mounting a dmg is not a feasible alternative due to its
  # bad write-performance.

  # prepare a script for mass-creating directories
  find $VER_DIR -type d ! -path "$VAR_DIR/*" ! -path "$SRC_DIR/*" \
      -exec echo "mkdir {}" > $WRK_DIR/create_dirs.sh \;
  echo "mkdir $BLD_DIR" >> $WRK_DIR/create_dirs.sh
  sed -i "" "1d" $WRK_DIR/create_dirs.sh   # remove first line ("file exists")
  chmod 755 $WRK_DIR/create_dirs.sh

  # create writable (ramdisk-) overlay
  device=$(create_ram_device $OVERLAY_RAMDISK_SIZE build)
  mount -o nobrowse,rw,union -t hfs $device $VER_DIR
  echo_i "writable overlay mounted as $device"

  # create all directories inside overlay
  $WRK_DIR/create_dirs.sh
  rm $WRK_DIR/create_dirs.sh
}

### main #######################################################################

install
