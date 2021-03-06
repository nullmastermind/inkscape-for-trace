# SPDX-License-Identifier: GPL-2.0-or-later

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced

### description ################################################################

# This file contains everything related to setup Meson build system.

### variables ##################################################################

# https://mesonbuild.com
MESON_PIP=meson==0.55.1

### functions ##################################################################

function meson_install
{
  pip3 install --prefix "$VER_DIR" "$MESON_PIP"
}
