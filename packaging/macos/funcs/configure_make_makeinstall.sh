# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### configure, make, make install in jhbuild environment #######################

function configure_make_makeinstall
{
  local flags="$*"
  local jhbuild_run

  if [ -f $BIN_DIR/jhbuild ]; then
    jhbuild_run="jhbuild run"
  fi

  $jhbuild_run ./configure --prefix=$VER_DIR $flags
  make_makeinstall
}