# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 010-vars.sh ###
# This file contains all the global variables (as in: configuration for the
# build pipeline) and gets sourced by all the other scripts.
# If you want to override settings, the suggested way is that you create a
# `0nn-custom.sh` file and put them there. All files named '0nn-*.sh' get
# sourced in order of appearance.

[ -z $VARS_INCLUDED ] && VARS_INCLUDED=true || return   # include guard

### source .profile ############################################################

# This is necessary on the first run and after changing directory settings.
# So, we're better safe than sorry and source it.

source $HOME/.profile

### general settings ###########################################################

CORES=$(sysctl -n hw.ncpu)   # use all available cores
export MAKEFLAGS="-j $CORES"

XZ_OPT=-T0   # use all available cores

### workspace/build environment paths ##########################################

WRK_DIR=$HOME/work
OPT_DIR=$WRK_DIR/opt
BIN_DIR=$OPT_DIR/bin
TMP_DIR=$OPT_DIR/tmp
SRC_DIR=$OPT_DIR/src
LIB_DIR=$OPT_DIR/lib

RAMDISK_ENABLE=true   # mount ramdisk to $WRK_DIR
RAMDISK_SIZE=10       # unit is GiB

### application bundle paths ###################################################

APP_DIR=$WRK_DIR/Inkscape.app   # keep in sync with 'inkscape.bundle'
APP_RES_DIR=$APP_DIR/Contents/Resources
APP_LIB_DIR=$APP_RES_DIR/lib
APP_BIN_DIR=$APP_RES_DIR/bin
APP_EXE_DIR=$APP_DIR/Contents/MacOS
APP_PLIST=$APP_DIR/Contents/Info.plist

### downlad URLs ###############################################################

URL_BOOST=https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2
URL_DOUBLE_CONVERSION=https://github.com/google/double-conversion/archive/v3.1.4.tar.gz
URL_FREETYPE=https://download.savannah.gnu.org/releases/freetype/freetype-2.10.0.tar.bz2
URL_GC=https://github.com/ivmai/bdwgc/releases/download/v8.0.4/gc-8.0.4.tar.gz
URL_GDL=https://github.com/GNOME/gdl/archive/GDL_3_28_0.tar.gz
URL_GSL=http://ftp.fau.de/gnu/gsl/gsl-2.5.tar.gz
URL_GTK_MAC_BUNDLER=https://gitlab.gnome.org/GNOME/gtk-mac-bundler/-/archive/727793cfae08dec0e1e2621078d53a02ec5f7fb3.tar.gz
URL_GTK_OSX=https://raw.githubusercontent.com/dehesselle/gtk-osx/inkscape
URL_GTK_OSX_BUILD_SETUP=$URL_GTK_OSX/gtk-osx-build-setup.sh
URL_GTK_OSX_MODULESET=$URL_GTK_OSX/modulesets-stable/gtk-osx.modules
URL_LIBPSL=https://github.com/rockdaboot/libpsl/releases/download/libpsl-0.20.2/libpsl-0.20.2.tar.gz
URL_LIBSOUP=https://ftp.gnome.org/pub/GNOME/sources/libsoup/2.65/libsoup-2.65.92.tar.xz
URL_OPENJPEG=https://github.com/uclouvain/openjpeg/archive/v2.3.0.tar.gz
URL_OPENSSL=https://www.openssl.org/source/openssl-1.1.1b.tar.gz
URL_POPPLER=https://gitlab.freedesktop.org/poppler/poppler/-/archive/poppler-0.74.0/poppler-poppler-0.74.0.tar.gz
# Inkscape Git
URL_INKSCAPE=https://gitlab.com/inkscape/inkscape
# TODO icon in icns format
# This is the only binary file in the build pipeline and it's roughly 1 MiB in
# size. I do not want to merge that to Inkscape because it's a temporary
# workarond: a future version of the build pipeline will generate the icns from
# an already present png file on-the-fly. But until that's been implemented,
# download the icns from the original "macOS Inkscape build and package"
# repository.
URL_INKSCAPE_ICNS=https://github.com/dehesselle/mibap/raw/master/inkscape.icns
