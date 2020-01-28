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

### name and directory #########################################################

SELF_NAME=$(basename $0)

### multithreading #############################################################

CORES=$(sysctl -n hw.ncpu)   # use all available cores
export MAKEFLAGS="-j $CORES"

### target OS version ##########################################################

# Gtk support policy is to support operating system releases up to 5 years
# back. See https://gitlab.gnome.org/GNOME/gtk-osx/blob/master/README.md
#
# gtk 3.24.13 does not build with 10.10 SDK
# (undeclared identifier NSWindowCollectionBehaviorFullScreenDisallowsTiling)

# The current setup is
#   - Xcode 11.3.1
#   - OS X El Capitan 10.11 SDK (part of Xcode 7.3.1)
#   - macOS Mojave 10.14.6
export MACOSX_DEPLOYMENT_TARGET=10.11
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX${MACOSX_DEPLOYMENT_TARGET}.sdk

### build system/toolset version ###############################################

TOOLSET_VERSION=0.30

### ramdisk ####################################################################

# Using the toolset dmg, a small writable overlay required.
OVERLAY_RAMDISK_SIZE=2   # unit is GiB

### toolset root directory #####################################################

# This is the main directory where all the action takes place below. It is
# one level above WRK_DIR (which in previous releases has been called the
# main directory) so we can manage and switch between multiple different WRK_DIR
# versions as required.

# Allow this to be overridable or use the default.
[ -z $TOOLSET_ROOT_DIR ] && TOOLSET_ROOT_DIR=/Users/Shared/work || true

if  [ $(mkdir -p $TOOLSET_ROOT_DIR 2>/dev/null; echo $?) -eq 0 ] &&
    [ -w $TOOLSET_ROOT_DIR ] &&
    [ "$(stat -f '%Su' $TOOLSET_ROOT_DIR)" = "$(whoami)" ] ; then
  :   # nothing to do, everything ok
else
  echo "❌ directory not usable (TOOLSET_ROOT_DIR): $TOOLSET_ROOT_DIR"
  exit 1
fi

### toolset repository directory ###############################################

# This is where .dmg files with pre-compiled toolsets are downloaded to.

TOOLSET_REPO_DIR=$TOOLSET_ROOT_DIR/repo

### work directory and subdirectories ##########################################

# Allow this to be overrideable or use version number as default.
[ -z $WRK_DIR ] && WRK_DIR=$TOOLSET_ROOT_DIR/$TOOLSET_VERSION || true

OPT_DIR=$WRK_DIR/opt
BIN_DIR=$OPT_DIR/bin
LIB_DIR=$OPT_DIR/lib
SRC_DIR=$OPT_DIR/src
TMP_DIR=$OPT_DIR/tmp

### use TMP_DIR for everything temporary #######################################

export TMP=$TMP_DIR
export TEMP=$TMP_DIR
export TMPDIR=$TMP_DIR
export XDG_CACHE_HOME=$TMP_DIR    # avoids creation of ~/.cache
export XDG_CONFIG_HOME=$TMP_DIR   # avoids creation of ~/.config

### JHBuild subdirectories and configuration ###################################

export DEVROOT=$WRK_DIR/gtk-osx
export DEVPREFIX=$DEVROOT/local
export DEV_SRC_ROOT=$DEVROOT/source

export JHBUILDRC=$DEVROOT/jhbuildrc   # requires modified gtk-osx-setup.sh
export JHBUILDRC_CUSTOM=$JHBUILDRC-custom

### Inkscape Git repository directory ##########################################

# Location is different when running as GitLab CI job.

if [ -z $CI_JOB_ID ]; then
  INK_DIR=$SRC_DIR/inkscape
else
  INK_DIR=$SELF_DIR/../..   # SELF_DIR needs to be set by the sourcing script
  INK_DIR=$(cd $INK_DIR; pwd -P)   # make path canoncial
fi

### artifact path ##############################################################

# This is the location where the final product - like application bundle or
# diskimage (no intermediate programs/libraries/...) - is created in.

ARTIFACT_DIR=$WRK_DIR/artifacts

### application bundle paths ###################################################

APP_DIR=$ARTIFACT_DIR/Inkscape.app
APP_CON_DIR=$APP_DIR/Contents
APP_RES_DIR=$APP_CON_DIR/Resources
APP_FRA_DIR=$APP_CON_DIR/Frameworks
APP_BIN_DIR=$APP_RES_DIR/bin
APP_ETC_DIR=$APP_RES_DIR/etc
APP_EXE_DIR=$APP_CON_DIR/MacOS
APP_LIB_DIR=$APP_RES_DIR/lib

APP_PLIST=$APP_CON_DIR/Info.plist

### download URLs ##############################################################

# These are the versioned URLs of Inkscape dependencies that are not part of
# any JHBuild moduleset. (They are candidates for a custom Inkscape moduleset.)

URL_BOOST=https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2
URL_CPPUNIT=https://dev-www.libreoffice.org/src/cppunit-1.14.0.tar.gz
URL_DOUBLE_CONVERSION=https://github.com/google/double-conversion/archive/v3.1.5.tar.gz
URL_GC=https://github.com/ivmai/bdwgc/releases/download/v8.0.4/gc-8.0.4.tar.gz

# This is one commit ahead of GDL_3_34_0.
# Fixes https://gitlab.gnome.org/GNOME/gdl/issues/2
URL_GDL=https://gitlab.gnome.org/GNOME/gdl/-/archive/9f11ad3ca8cef85b075419b30036d73648498dfe/gdl-9f11ad3ca8cef85b075419b30036d73648498dfe.tar.gz

URL_GHOSTSCRIPT=https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/gs950/ghostscript-9.50.tar.gz
URL_GSL=http://ftp.fau.de/gnu/gsl/gsl-2.6.tar.gz
URL_GTK_MAC_BUNDLER=https://github.com/dehesselle/gtk-mac-bundler/archive/f96a9daf2236814af7ace7a2fa91bbfb4f077779.tar.gz
URL_GTK_OSX=https://raw.githubusercontent.com/dehesselle/gtk-osx/inkscape
URL_GTK_OSX_SETUP=$URL_GTK_OSX/gtk-osx-setup.sh
URL_GTK_OSX_MODULESET=$URL_GTK_OSX/modulesets-stable/gtk-osx.modules
URL_IMAGEMAGICK=https://github.com/ImageMagick/ImageMagick6/archive/6.9.10-89.tar.gz
URL_INKSCAPE=https://gitlab.com/inkscape/inkscape
URL_INKSCAPE_DMG_ICNS=https://github.com/dehesselle/mibap/raw/master/inkscape_dmg.icns
URL_LIBCDR=https://github.com/LibreOffice/libcdr/archive/libcdr-0.1.5.tar.gz
URL_LIBREVENGE=https://ayera.dl.sourceforge.net/project/libwpd/librevenge/librevenge-0.0.4/librevenge-0.0.4.tar.gz
URL_LIBVISIO=https://github.com/LibreOffice/libvisio/archive/libvisio-0.1.7.tar.gz
URL_LIBWPG=https://netcologne.dl.sourceforge.net/project/libwpg/libwpg/libwpg-0.3.3/libwpg-0.3.3.tar.xz
URL_OPENJPEG=https://github.com/uclouvain/openjpeg/archive/v2.3.1.tar.gz
URL_OPENMP=https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/openmp-9.0.1.src.tar.xz
URL_PNG2ICNS=https://github.com/bitboss-ca/png2icns/archive/v0.1.tar.gz
URL_POPPLER=https://gitlab.freedesktop.org/poppler/poppler/-/archive/poppler-0.84.0/poppler-poppler-0.84.0.tar.gz
URL_POTRACE=http://potrace.sourceforge.net/download/1.16/potrace-1.16.tar.gz

# This is the relocatable framework to be bundled with the app.
URL_PYTHON3_BIN=https://github.com/dehesselle/py3framework/releases/download/py376.1/py376_framework_1.tar.xz

# These two are for JHBuild only (it fails to download and install its own Python).
URL_PYTHON36_SRC=https://github.com/dehesselle/py3framework/archive/py369.3.tar.gz
URL_PYTHON36_BIN=https://github.com/dehesselle/py3framework/releases/download/py369.3/py369_framework_3.tar.xz

# Pre-compiled version of the whole toolset.
URL_TOOLSET=https://github.com/dehesselle/mibap/releases/download/v$TOOLSET_VERSION/mibap_v$TOOLSET_VERSION.dmg

### Python packages ############################################################

PYTHON_CAIROSVG=cairosvg==2.4.2
PYTHON_CAIROCFFI=cairocffi==1.1.0
PYTHON_DMGBUILD=dmgbuild==1.3.3
PYTHON_LXML=lxml==4.4.2
PYTHON_NUMPY=numpy==1.18.1
PYTHON_PYCAIRO=pycairo==1.19.0
PYTHON_PYGOBJECT=PyGObject==3.34.0
PYTHON_SCOUR=scour==0.37
PYTHON_PYSERIAL=pyserial==3.4

### profile ####################################################################

# Settings that would otherwise go into '.profile'.

export PATH=$DEVPREFIX/bin:$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin
export LANG=de_DE.UTF-8   # jhbuild complains otherwise   FIXME: hard-coded value
