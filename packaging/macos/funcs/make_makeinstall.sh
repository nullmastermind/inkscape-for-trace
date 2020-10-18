# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### make, make install in jhbuild environment ##################################

function make_makeinstall
{
  local jhbuild_run

  if [ -f $BIN_DIR/jhbuild ]; then
    jhbuild_run="jhbuild run"
  fi

  $jhbuild_run make
  $jhbuild_run make install
}