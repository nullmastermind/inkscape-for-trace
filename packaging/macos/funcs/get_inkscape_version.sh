# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### get Inkscape version from CMakeLists.txt ###################################

function get_inkscape_version
{
  local file=$INK_DIR/CMakeLists.txt
  local ver_major=$(grep INKSCAPE_VERSION_MAJOR $file | head -n 1 | awk '{ print $2+0 }')
  local ver_minor=$(grep INKSCAPE_VERSION_MINOR $file | head -n 1 | awk '{ print $2+0 }')
  local ver_patch=$(grep INKSCAPE_VERSION_PATCH $file | head -n 1 | awk '{ print $2+0 }')
  local ver_suffix=$(grep INKSCAPE_VERSION_SUFFIX $file | head -n 1 | awk '{ print $2 }')

  ver_suffix=${ver_suffix%\"*}   # remove "double quote and everything after" from end
  ver_suffix=${ver_suffix#\"}   # remove "double quote" from beginning

  echo $ver_major.$ver_minor.$ver_patch$ver_suffix
}