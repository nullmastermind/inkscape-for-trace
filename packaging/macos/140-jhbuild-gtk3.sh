#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 140-jhbuild-gtk3.sh ###
# Install GTK3 libraries.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

#run_annotated   disabled for now, breaks jhbuild interactive mode

### install GTK3 libraries #####################################################

jhbuild build \
  meta-gtk-osx-bootstrap \
  meta-gtk-osx-freetype \
  meta-gtk-osx-gtk3 \
  meta-gtk-osx-gtkmm3

### fix GDK pixbuf loaders #####################################################

for file in $LIB_DIR/gdk-pixbuf-2.0/2.10.0/loaders/*.dylib; do
  mv $file $(dirname $file)/$(basename -s .dylib $file).so
done

gdk-pixbuf-query-loaders > $LIB_DIR/gdk-pixbuf-2.0/2.10.0/loaders.cache

