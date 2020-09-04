# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### install Python package with Python.framework ###############################

function pip_install
{
  local package=$1
  local options=$2   # optional

  local PATH_ORIGINAL=$PATH
  export PATH=$APP_FRA_DIR/Python.framework/Versions/Current/bin:$PATH

  pip$PY3_MAJOR install \
    --prefix=$APP_RES_DIR \
    --ignore-installed \
    $options \
    $package

  export PATH=$PATH_ORIGINAL
}