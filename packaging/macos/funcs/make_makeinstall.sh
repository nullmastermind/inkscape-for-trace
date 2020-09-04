# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### make, make install in jhbuild environment ##################################

function make_makeinstall
{
  jhbuild run make
  jhbuild run make install
}