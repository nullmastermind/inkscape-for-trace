# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### create disk image ##########################################################

function create_dmg
{
  local app=$1
  local dmg=$2
  local cfg=$3

  # set application
  sed -i '' "s/PLACEHOLDERAPPLICATION/$(escape_sed $app)/" $cfg

  # set disk image icon (if it exists)
  local icon=$SRC_DIR/$(basename -s .py $cfg).icns
  if [ -f $icon ]; then
    sed -i '' "s/PLACEHOLDERICON/$(escape_sed $icon)/" $cfg
  fi

  # set background image (if it exists)
  local background=$SRC_DIR/$(basename -s .py $cfg).png
  if [ -f $background ]; then
    sed -i '' "s/PLACEHOLDERBACKGROUND/$(escape_sed $background)/" $cfg
  fi

  # create disk image
  dmgbuild -s $cfg "$(basename -s .app $app)" $dmg
}