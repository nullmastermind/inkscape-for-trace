# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### create and mount ramdisk ###################################################

# There is a more common approach to do this using
#    diskutil eraseVolume HFS+ VolName $(hdiutil attach -nomount ram://<size>)
# but that always attaches the ramdisk below '/Volumes'.
# To have full control, we need to do it as follows.

function create_ramdisk
{
  local dir=$1    # mountpoint
  local size=$2   # unit is GiB

  if [ $(mount | grep $dir | wc -l) -eq 0 ]; then
    local device=$(hdiutil attach -nomount ram://$(expr $size \* 1024 \* 2048))
    newfs_hfs -v "RAMDISK" $device
    mount -o noatime,nobrowse -t hfs $device $dir
  fi
}