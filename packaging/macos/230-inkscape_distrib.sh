#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# This file is part of the build pipeline for Inkscape on macOS.

### description ################################################################

# Create disk image for distribution.

### includes ###################################################################

# shellcheck disable=SC1090 # can't point to a single source here
for script in "$(dirname "${BASH_SOURCE[0]}")"/0??-*.sh; do
  source "$script";
done

### settings ###################################################################

# shellcheck disable=SC2034 # this is from ansi_.sh
ANSI_TERM_ONLY=false   # use ANSI control characters even if not in terminal

error_trace_enable

### main #######################################################################

# Create background for development snapshots. This is not meant for
# official releases, those will be re-packaged manually (they also need
# to be signed and notarized).
convert -size 560x400 xc:transparent \
  -font Andale-Mono -pointsize 64 -fill black \
  -draw "text 20,60 'Inkscape'" \
  -draw "text 20,120 '$(ink_get_version)'" \
  -draw "text 20,180 'development'" \
  -draw "text 20,240 'snapshot'" \
  -draw "text 20,300 '$(ink_get_repo_shorthash)'" \
  "$SRC_DIR"/inkscape_dmg.png

# Create the disk image.
dmgbuild_run "$ARTIFACT_DIR"/Inkscape.dmg
