#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 150-jhbuild-inkdeps.sh ###
# Install additional dependencies into our jhbuild environment required for
# building Inkscape.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install GNU Scientific Library #############################################

get_source $URL_GSL
configure_make_makeinstall

### install additional GNOME libraries #########################################

# libsoup - GNOME http client/server library
# adwaita-icon-theme - icons used by Inkscape/GTK

jhbuild build adwaita-icon-theme libsoup

### install Garbage Collector for C/C++ ########################################

get_source $URL_GC
configure_make_makeinstall

### install GNOME Docking Library ##############################################

get_source $URL_GDL
jhbuild run ./autogen.sh
configure_make_makeinstall

### install boost ##############################################################

get_source $URL_BOOST
jhbuild run ./bootstrap.sh --prefix=$OPT_DIR
jhbuild run ./b2 -j$CORES install

### install little CMS colore engine v2 ########################################

get_source $URL_LCMS2
configure_make_makeinstall

### install OpenJPEG ###########################################################

get_source $URL_OPENJPEG
cmake_make_makeinstall

### install Poppler ############################################################

get_source $URL_POPPLER
cmake_make_makeinstall -DENABLE_UNSTABLE_API_ABI_HEADERS=ON

### install gtk-mac-bundler ####################################################

get_source $URL_GTK_MAC_BUNDLER
make install

### install double-conversion ##################################################

# Required by lib2geom.

get_source $URL_DOUBLE_CONVERSION
cmake_make_makeinstall

### install Potrace ############################################################

get_source $URL_POTRACE
configure_make_makeinstall --with-libpotrace

