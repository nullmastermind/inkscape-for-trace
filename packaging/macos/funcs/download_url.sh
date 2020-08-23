# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### download a file and save to disk ###########################################

function download_url
{
  local url=$1
  local target_dir=$2
  local file=$3   # optional

  if [ -z $target_dir ]; then
    echo_e "no target directory supplied"
    return 1
  fi

  mkdir -p $target_dir

  if [ -z $file ]; then
    file=$target_dir/$(basename $url)
  else
    file=$target_dir/$file
  fi

  if [ -f $file ]; then
    echo_w "file will be overwritten: $file"
  fi

  curl -o $file -L $url
}