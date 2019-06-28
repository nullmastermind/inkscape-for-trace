#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 230-inkscape-distrib.sh ###
# Get application ready for distribution.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### create disk image for distribution #########################################

# Update the .json file with our paths.

cp $SELF_DIR/inkscape_dmg.json $SRC_DIR

(
  # escaping forward slashes: https://unix.stackexchange.com/a/379577
  # ESCAPED=${STRING//\//\\\/}

  FILE=$SRC_DIR/inkscape_dmg.png
  sed -i '' "s/PLACEHOLDERBACKGROUND/${FILE//\//\\\/}/" $SRC_DIR/inkscape_dmg.json
  sed -i '' "s/PLACEHOLDERPATH/${APP_DIR//\//\\\/}/" $SRC_DIR/inkscape_dmg.json
)

# Create background for development snapshots. This is not meant for
# official releases, those will be re-packaged manually (they also need
# to be signed and notarized).

convert -size 560x400 xc:transparent \
  -font Andale-Mono -pointsize 64 -fill black \
  -draw "text 20,60 'Inkscape'" \
  -draw "text 20,120 '$(get_inkscape_version)'" \
  -draw "text 20,180 'development'" \
  -draw "text 20,240 'snapshot'" \
  -draw "text 20,300 '$(get_repo_version $INK_DIR)'" \
  $SRC_DIR/inkscape_dmg.png

# create the disk image

(
  cd $SRC_DIR/node-*
  export PATH=$PATH:$(pwd)/bin
  appdmg $SRC_DIR/inkscape_dmg.json $ARTIFACT_DIR/Inkscape.dmg
)

# CI: move disk image to a location accessible for the runner

if [ ! -z $CI_JOB_ID ]; then
  [ -d $INK_DIR/artifacts ] && rm -rf $INK_DIR/artifacts
  mv $ARTIFACT_DIR $INK_DIR/artifacts
  rm -rf $INK_DIR/artifacts/*.app
fi
