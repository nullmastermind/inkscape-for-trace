# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### create a ramdisk and return the device #####################################

function create_ram_device
{
  local size_gib=$1   # unit is GiB
  local name=$2       # volume name

  [ -z $name ] && name=$(uuidgen | md5 | head -c 8)   # generate random name

  local size_sectors=$(expr $size_gib \* 1024 \* 2048)
  local device=$(hdiutil attach -nomount ram://$size_sectors)
  newfs_hfs -v "$name" $device >/dev/null

  echo $device
}