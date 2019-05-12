# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 020-vars.sh ###
# This file contains all the global variables (as in: configuration for the
# build pipeline) and gets sourced by all the other scripts.
# If you want to override settings, the suggested way is that you create a
# `0nn-custom.sh` file and put them there. All files named '0nn-*.sh' get
# sourced in numerical order.

[ -z $VARS_INCLUDED ] && VARS_INCLUDED=true || return   # include guard

### source .profile ############################################################

# This is necessary on the first run and after changing directory settings.
# So, we're better safe than sorry and source it.

[ -f $HOME/.profile ] && source $HOME/.profile

### name #######################################################################

SELF_NAME=$(basename $0)

### multithreading #############################################################

CORES=$(sysctl -n hw.ncpu)   # use all available cores
export MAKEFLAGS="-j $CORES"

### ramdisk ####################################################################

RAMDISK_ENABLE=true   # mount ramdisk to $WRK_DIR
RAMDISK_SIZE=10       # unit is GiB

### try to use pre-built toolset ###############################################

# In order to just download and extract a pre-built toolset
#   - it has to be enabled ('TOOLSET_CACHE_ENABLE=true')
#   - you have to use $DEFAULT_SYSTEM_WRK_DIR as your $WRK_DIR
#     (see commentary in the section below for explanation)
#
# It does not hurt to have it enabled by default, because if it cannot be
# used, it won't be used and doesn't cause errors.

TOOLSET_CACHE_ENABLE=true

### workspace/build environment paths ##########################################

# The current behavior of selecting a $WRK_DIR:
#   1. If there IS a pre-existing value (user configuration), use that.
#     a. If it IS NOT writable, exit with error.
#   2. If there IS NOT a pre-existing value, try to use $DEFAULT_SYSTEM_WRK_DIR.
#     a. If it DOES exist and IS writable, continue using that.
#     b. If it DOES exist and IS NOT writable, exit with error.
#     c. If it DOES NOT exist, use $DEFAULT_USER_WRK_DIR.
#
# The reasoning behind this:
# In regards to (1), if the user configured a directory, it has to be used. If
# it can't be used, exit with error (as a user would expect).
# In regards to (2), using an arbitrary system-level location like
# $DEFAULT_SYSTEM_WRK_DIR is considered optional as in "you have to opt-in"
# (and not mandatory at all). You can tell us that you "opted-in" by creating
# that directory beforehand and ensuring it's writable (2a). Than it's going to
# be used. (I don't like writing scripts that write to arbitrary locations on
# their own. Also, that would require 'sudo' permission.)
# If $DEFAULT_SYSTEM_WRK_DIR does not exist, it means that you did not
# "opt-in" (2c) and we're going to use the user-based default location,
# $DEFAULT_USER_WRK_DIR. If $DEFAULT_SYSTEM_WRK_DIR does exist but is not
# writable, the situation is unclear and we exit in error (2b).
#
# But why bother with a $DEFAULT_SYSTEM_WRK_DIR at all? Because that's the only
# way a pre-built build environment can be used (hard-coded library locations).

if [ -z $WRK_DIR ]; then     # no pre-existing value
  WRK_DIR=$DEFAULT_SYSTEM_WRK_DIR   # try default first

  if [ -d $WRK_DIR ]; then      # needs to exist and
    if [ ! -w $WRK_DIR ]; then  # must be writable
      echo "directory exists but not writable: $WRK_DIR"
      exit 1
    fi
  else   # use work directory below user's home (writable for sure)
    WRK_DIR=$DEFAULT_USER_WRK_DIR
  fi
fi

if [ $(mkdir -p $WRK_DIR 2>/dev/null; echo $?) -eq 0 ] &&
   [ -w $WRK_DIR ]; then
  echo "using build directory: $WRK_DIR"
else
  echo "directory not writable: $WRK_DIR"
  exit 1
fi

OPT_DIR=$WRK_DIR/opt
BIN_DIR=$OPT_DIR/bin
LIB_DIR=$OPT_DIR/lib
SRC_DIR=$OPT_DIR/src
TMP_DIR=$OPT_DIR/tmp

### artifact path ##############################################################

# This is the location where the final product - like application bundle or
# diskimage (no intermediate programs/libraries/...) - is created in.

ARTIFACT_DIR=$WRK_DIR/artifacts

### application bundle paths ###################################################

APP_DIR=$ARTIFACT_DIR/Inkscape.app
APP_CON_DIR=$APP_DIR/Contents
APP_RES_DIR=$APP_CON_DIR/Resources
APP_BIN_DIR=$APP_RES_DIR/bin
APP_ETC_DIR=$APP_RES_DIR/etc
APP_EXE_DIR=$APP_CON_DIR/MacOS
APP_LIB_DIR=$APP_RES_DIR/lib
APP_PLIST=$APP_CON_DIR/Info.plist

### downlad URLs ###############################################################

# These are the versioned URLs of Inkscape dependencies that are not part of
# any JHBuild moduleset. (They are candidates for a custom Inkscape moduleset.)

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
# Inkscape Git repository
URL_INKSCAPE=https://gitlab.com/inkscape/inkscape
# TODO icon in icns format
# This is the only binary file in the build pipeline and it's roughly 1 MiB in
# size. I do not want to merge that to Inkscape because it's a temporary
# workarond: a future version of the build pipeline will generate the icns from
# an already present png file on-the-fly. But until that's been implemented,
# download the icns from the original "macOS Inkscape build and package"
# repository.
URL_INKSCAPE_ICNS=https://github.com/dehesselle/mibap/raw/master/inkscape.icns
URL_LCMS2=https://netcologne.dl.sourceforge.net/project/lcms/lcms/2.9/lcms2-2.9.tar.gz
URL_LIBPSL=https://github.com/rockdaboot/libpsl/releases/download/libpsl-0.20.2/libpsl-0.20.2.tar.gz
URL_LIBSOUP=https://ftp.gnome.org/pub/GNOME/sources/libsoup/2.65/libsoup-2.65.92.tar.xz
URL_OPENJPEG=https://github.com/uclouvain/openjpeg/archive/v2.3.0.tar.gz
URL_OPENSSL=https://www.openssl.org/source/openssl-1.1.1b.tar.gz
URL_POPPLER=https://gitlab.freedesktop.org/poppler/poppler/-/archive/poppler-0.74.0/poppler-poppler-0.74.0.tar.gz
URL_POTRACE=http://potrace.sourceforge.net/download/1.15/potrace-1.15.tar.gz
# A pre-built version of the complete toolset.
URL_TOOLSET_CACHE=https://github.com/dehesselle/mibap/releases/download/v0.9/mibap_v0.9.tar.xz

