# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### cmake, make, make install in jhbuild environment ###########################

function cmake_make_makeinstall
{
  local flags="$*"

  mkdir builddir
  cd builddir
  jhbuild run cmake -DCMAKE_INSTALL_PREFIX=$VER_DIR $flags ..
  make_makeinstall
}