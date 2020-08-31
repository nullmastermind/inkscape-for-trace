# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### get compression flag by filename extension #################################

function get_comp_flag
{
  local file=$1

  local extension=${file##*.}

  case $extension in
    bz2) echo "j" ;;
    gz) echo "z"  ;;
    tgz) echo "z" ;;
    xz) echo "J"  ;;
    *) echo "ERROR unknown extension $extension"
  esac
}