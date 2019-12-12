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

set -e

### install build system #######################################################

function install
{
  echo_info "VERSION_WANT = $VERSION_WANT"
  echo_info "VERSION_HAVE = $VERSION_HAVE"

  if [ "$VERSION_WANT" = "$VERSION_HAVE" ]; then
    echo_ok "version ok"
  else
    # Version requirement has not been met, we have to install the required
    # build system version.
    
    echo_err "version mismatch"

    while : ; do   # unmount everything
      local disk=$(mount | grep $WRK_DIR | tail -n1 | awk '{ print $1 }')

      if [ ${#disk} -eq 0 ]; then
        break                              # nothing to do here
      else
        diskutil eject $disk > /dev/null   # unmount
        echo_ok "ejected $disk"
      fi
    done

    local buildsys_dmg=$REPOSITORY_DIR/$(basename $URL_BUILDSYS)

    if [ -f $buildsys_dmg ]; then
      echo_info "no download required"
    else
      # File not present on disk, we need to download.
      echo_act "download required"
      [ ! -d $REPOSITORY_DIR ] && mkdir -p $REPOSITORY_DIR
      save_file $URL_BUILDSYS $REPOSITORY_DIR
      echo_ok "download successful"
    fi

    # mount build system read-only
    local device=$(create_dmg_device $buildsys_dmg)
    [ ! -d $WRK_DIR ] && mkdir -p $WRK_DIR
    mount -o nobrowse,ro -t hfs $device $WRK_DIR
    echo_ok "buildsystem mounted as $device"

    # Sadly, there are some limitations involved with union-mounting:
    #   - files are not visible to 'ls'
    #   - you cannot write in a location without having written to its
    #     parent location
    #
    # Shadow-mounting a dmg is not a feasible alternative due to its
    # bad write-performance.

    # prepare a script for mass-creating directories
    find $OPT_DIR -type d ! -path "$TMP_DIR/*" ! -path "$SRC_DIR/*" \
        -exec echo "mkdir {}" > $WRITABLE_DIR/create_dirs.sh \;
    chmod 755 $WRITABLE_DIR/create_dirs.sh

    # create writable (ramdisk-) overlay
    device=$(create_ram_device $RAMDISK_SIZE build)
    mount -o nobrowse,rw,union -t hfs $device $WRK_DIR
    echo_ok "writable ramdisk overlay mounted as $device"

    # create all directories inside overlay
    $WRITABLE_DIR/create_dirs.sh
    rm $WRITABLE_DIR/create_dirs.sh
  fi
}

### main #######################################################################

install

mkdir -p $HOME/.config
rm -f $HOME/.config/jhbuild*
ln -sf $DEVCONFIG/jhbuild* $HOME/.config