#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 230-inkscape-distrib.sh ###
# Get application ready for distribution.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

run_annotated

### create disk image for distribution #########################################

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

# A few things to note:
#   - For this script to work, there must be a running desktop session.
#   - There have been reports of troubles with the Applescript portions
#     of this script, requiring additional privileges on newer macOS versions.
#   - The background image in the '.dmg' does not show in OS X El Capitan
#     (10.11), it has something to do with how the '.DS_Store' is generated.

cd $SRC_DIR/create-dmg*
./create-dmg \
  --volname Inkscape \
  --background $SRC_DIR/inkscape_dmg.png \
  --icon "Inkscape.app" 390 240 \
  --icon-size 64 \
  $TMP_DIR/Inkscape.dmg $ARTIFACT_DIR

rm -rf $APP_DIR
mv $TMP_DIR/Inkscape.dmg $ARTIFACT_DIR

# CI: move disk image to a location accessible for the runner

if [ ! -z $CI_JOB_ID ]; then
  [ -d $INK_DIR/artifacts ] && rm -rf $INK_DIR/artifacts
  mv $ARTIFACT_DIR $INK_DIR/artifacts
fi

