# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### configure, make, make install in jhbuild environment #######################

function configure_make_makeinstall
{
  local flags="$*"

  jhbuild run ./configure --prefix=$VER_DIR $flags
  make_makeinstall
}