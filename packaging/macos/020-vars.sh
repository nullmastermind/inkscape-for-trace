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

### build system: version ######################################################

TOOLSET_VERSION=0.45

### build system: target OS version ############################################

# The current build setup is
#   - Xcode 12.0.1
#   - OS X El Capitan 10.11 SDK (part of Xcode 7.3.1)
#   - macOS Catalina 10.15.7

SDK_VERSION=10.11
# Allow this to be ovefrrideable or use the default.
[ -z $SDKROOT_DIR ] && SDKROOT_DIR=/opt/sdks
export SDKROOT=$SDKROOT_DIR/MacOSX$SDK_VERSION.sdk

### main work directory ########################################################

# This is the main directory where all the action takes place below.

# Allow this to be overridable or use the default.
# Purpose of the default choice is to have guaranteed writable location
# that is present on every macOS installation.
[ -z $WRK_DIR ] && WRK_DIR=/Users/Shared/work || true

if  [ $(mkdir -p $WRK_DIR 2>/dev/null; echo $?) -eq 0 ] &&
    [ -w $WRK_DIR ] ; then
  :   # ok: WRK_DIR has been created or was already there and is writable
else
  echo "***ERROR*** WRK_DIR not usable: $WRK_DIR"
  exit 1
fi

### unversioned/persistent directories #########################################

# These directories are meant to be persistent between builds.

# Repository for downloaded toolset .dmg files.
REPO_DIR=$WRK_DIR/repo

# Location for ccache.
export CCACHE_DIR=$WRK_DIR/ccache

### versioned directories ######################################################

VER_DIR=$WRK_DIR/$TOOLSET_VERSION
BIN_DIR=$VER_DIR/bin
ETC_DIR=$VER_DIR/etc
INC_DIR=$VER_DIR/include
LIB_DIR=$VER_DIR/lib
VAR_DIR=$VER_DIR/var
BLD_DIR=$VAR_DIR/build
PKG_DIR=$VAR_DIR/cache/pkgs
SRC_DIR=$VER_DIR/usr/src
TMP_DIR=$VER_DIR/tmp

### versioned directories: temporary ###########################################

export TMP=$TMP_DIR
export TEMP=$TMP_DIR
export TMPDIR=$TMP_DIR   # TMPDIR is the common macOS default

### versioned directories: XDG #################################################

export XDG_CACHE_HOME=$VAR_DIR/cache  # instead ~/.cache
export XDG_CONFIG_HOME=$ETC_DIR       # instead ~/.config

### versioned directories: pip #################################################

export PIP_CACHE_DIR=$XDG_CACHE_HOME/pip       # instead ~/Library/Caches/pip
export PIPENV_CACHE_DIR=$XDG_CACHE_HOME/pipenv # instead ~/Library/Caches/pipenv

### versioned directories: artifact and application bundle paths ###############

# This is the location where the final product - like application bundle or
# diskimage (no intermediate programs/libraries/...) - is created in.
ARTIFACT_DIR=$VER_DIR/artifacts

APP_DIR=$ARTIFACT_DIR/Inkscape.app
APP_CON_DIR=$APP_DIR/Contents
APP_RES_DIR=$APP_CON_DIR/Resources
APP_FRA_DIR=$APP_CON_DIR/Frameworks
APP_BIN_DIR=$APP_RES_DIR/bin
APP_ETC_DIR=$APP_RES_DIR/etc
APP_EXE_DIR=$APP_CON_DIR/MacOS
APP_LIB_DIR=$APP_RES_DIR/lib

APP_PLIST=$APP_CON_DIR/Info.plist

### versioned directories: JHBuild configuration ###############################

export JHBUILDRC=$ETC_DIR/jhbuildrc
export JHBUILDRC_CUSTOM=$JHBUILDRC-custom

### Inkscape source and build directories ######################################

# Location differs between running standalone and GitLab CI job.

if [ -z $CI_JOB_ID ]; then
  INK_DIR=$SRC_DIR/inkscape
else
  INK_DIR=$SELF_DIR/../..   # SELF_DIR needs to be set by the sourcing script
  INK_DIR=$(cd $INK_DIR; pwd -P)   # make path canoncial
fi

INK_BUILD_DIR=$BLD_DIR/$(basename $INK_DIR)

### Python #####################################################################

# Inkscape comes bundled with its own Python runtime to make the core
# extensions work out-of-the-box.

PY3_MAJOR=3
PY3_MINOR=8
PY3_PATCH=5
PY3_BUILD=2   # custom build, see URL section below

# This is a relocatable Python.framework to be bundled with the app. Mind the
# lowercase 'i' at the end of the URL, hinting at a "customized for Inkscape"
# version of the framework.
# https://github.com/dehesselle/py3framework
URL_PYTHON=https://github.com/dehesselle/py3framework/releases/download/\
py$PY3_MAJOR$PY3_MINOR$PY3_PATCH.$PY3_BUILD/\
py$PY3_MAJOR$PY3_MINOR${PY3_PATCH}_framework_${PY3_BUILD}i.tar.xz

### Python: packages for Inkscape ##############################################

# The following Python packages are bundled with Inkscape.

# https://cairocffi.readthedocs.io/en/stable/
# https://github.com/Kozea/cairocffi
PYTHON_CAIROCFFI=cairocffi==1.1.0

# https://lxml.de
# https://github.com/lxml/lxml
# https://github.com/dehesselle/py3framework
PYTHON_LXML=$(dirname $URL_PYTHON)/lxml-4.5.2-cp$PY3_MAJOR$PY3_MINOR-cp$PY3_MAJOR$PY3_MINOR-macosx_10_9_x86_64.whl

# https://github.com/numpy/numpy
PYTHON_NUMPY=numpy==1.19.1

# https://pygobject.readthedocs.io/en/latest/
PYTHON_PYGOBJECT=PyGObject==3.36.1

# https://github.com/scour-project/scour
PYTHON_SCOUR=scour==0.37

# https://pyserial.readthedocs.io/en/latest/
# https://github.com/pyserial/pyserial
PYTHON_PYSERIAL=pyserial==3.4

### Python: auxiliary packages #################################################

# The following Python packages are required for the build system.

# convert SVG to PNG
# https://cairosvg.org
PYTHON_CAIROSVG=cairosvg==2.4.2

# Mozilla Root Certificates
# https://pypi.org/project/certifi
PYTHON_CERTIFI=certifi   # This is unversioned on purpose.

# create DMG
# https://dmgbuild.readthedocs.io/en/latest/
# https://github.com/al45tair/dmgbuild
PYTHON_DMGBUILD=dmgbuild==1.3.3

# Meson build system
# https://mesonbuild.com
PYTHON_MESON=meson==0.55.1

### download URLs ##############################################################

# Every required piece of software for building, packaging etc. that doesn't
# have its own section ends up here.

# https://ccache.dev
# https://github.com/ccache/ccache
URL_CCACHE=https://github.com/ccache/ccache/releases/download/v3.7.11/ccache-3.7.11.tar.xz

# create application bundle
# https://github.com/dehesselle/gtk-mac-bundler
# Forked from https://gitlab.gnome.org/GNOME/gtk-mac-bundler
URL_GTK_MAC_BUNDLER=https://github.com/dehesselle/gtk-mac-bundler/archive/f96a9daf2236814af7ace7a2fa91bbfb4f077779.tar.gz

# Inkscapge Git repo (for standalone/non-CI builds)
URL_INKSCAPE=https://gitlab.com/inkscape/inkscape
# disk image icon
URL_INKSCAPE_DMG_ICNS=https://github.com/dehesselle/mibap/raw/master/inkscape_dmg.icns

# JHBuild build system
# https://gitlab.gnome.org/GNOME/jhbuild
# https://wiki.gnome.org/Projects/Jhbuild/Introduction
URL_JHBUILD=https://github.com/dehesselle/jhbuild/archive/4b4723aa26950f1b32d80e848bffde63d3e5870f.tar.gz

# Ninja build system
# https://github.com/ninja-build/ninja
URL_NINJA=https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-mac.zip

# convert PNG image to iconset in ICNS format
# https://github.com/bitboss-ca/png2icns
URL_PNG2ICNS=https://github.com/bitboss-ca/png2icns/archive/v0.1.tar.gz

# A pre-compiled version of the whole toolset.
# https://github.com/dehesselle/mibap
URL_TOOLSET=https://github.com/dehesselle/mibap/releases/download/v$TOOLSET_VERSION/mibap_v$TOOLSET_VERSION.dmg

### multithreading #############################################################

CORES=$(sysctl -n hw.ncpu)   # use all available cores
export MAKEFLAGS="-j $CORES"

### name #######################################################################

SELF_NAME=$(basename $0)   # used by scripts that source this one

### path #######################################################################

export PATH=$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin

### ramdisk ####################################################################

# Using the toolset dmg, a small writable overlay is required.
OVERLAY_RAMDISK_SIZE=3   # unit is GiB
