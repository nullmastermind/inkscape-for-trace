# SPDX-License-Identifier: GPL-2.0-or-later

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced
# shellcheck disable=SC2164 # we trap errors to catch bad 'cd'

### description ################################################################

# Convert svg to icns.

### variables ##################################################################

# Nothing here.

### functions ##################################################################

function svg2icns_install
{
  cairosvg_install
  png2icns_install
}

function svg2icns
{
  local svg_file=$1
  local icns_file=$2

  local png_file
  png_file=$TMP_DIR/$(basename -s .svg "$svg_file").png

  # svg to png
  jhbuild run cairosvg -f png -s 1 -o "$png_file" "$svg_file"

  # png to icns
  cd "$TMP_DIR"   # png2icns.sh outputs to current directory
  png2icns.sh "$png_file"

  mv "$(basename -s .png "$png_file")".icns "$icns_file"
}
