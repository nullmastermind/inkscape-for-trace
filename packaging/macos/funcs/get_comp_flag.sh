# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### get compression flag by filename extension #################################

function get_comp_flag
{
  local file=$1

  local extension=${file##*.}

  case $extension in
    gz) echo "z"  ;;
    bz2) echo "j" ;;
    xz) echo "J"  ;;
    *) echo "ERROR unknown extension $extension"
  esac
}