# SPDX-License-Identifier: GPL-2.0-or-later

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced

### description ################################################################

# Convert svg to png with cairosvg.

### variables ##################################################################

# https://cairocffi.readthedocs.io/en/stable/
# https://github.com/Kozea/cairocffi
# https://cairosvg.org
# https://github.com/Kozea/CairoSVG
CAIROSVG_PIP="\
  cairocffi==1.1.0\
  cairosvg==2.4.2\
"

### functions ##################################################################

function cairosvg_install
{
  # shellcheck disable=SC2086 # we need word splitting here
  jhbuild run pip3 install $CAIROSVG_PIP
}
