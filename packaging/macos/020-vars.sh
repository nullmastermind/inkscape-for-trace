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

### nesting level when trapping errors #########################################

ERRTRACE_COUNT=0

### multithreading #############################################################

CORES=$(sysctl -n hw.ncpu)   # use all available cores
export MAKEFLAGS="-j $CORES"

### target OS version ##########################################################

# The current build setup is
#   - Xcode 11.3.1
#   - OS X El Capitan 10.11 SDK (part of Xcode 7.3.1)
#   - macOS Mojave 10.14.6

export MACOSX_DEPLOYMENT_TARGET=10.11
[ -z $SDKROOT_DIR ] && SDKROOT_DIR=/Library/Developer/CommandLineTools/SDKs
export SDKROOT=$SDKROOT_DIR/MacOSX${MACOSX_DEPLOYMENT_TARGET}.sdk

### build system/toolset version ###############################################

TOOLSET_VERSION=0.33

### ramdisk ####################################################################

# Using the toolset dmg, a small writable overlay is required.
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

### toolset directories ########################################################

# This is where .dmg files with pre-compiled toolsets are downloaded to.
TOOLSET_REPO_DIR=$TOOLSET_ROOT_DIR/repo
# Persistent location for ccache.
export CCACHE_DIR=$TOOLSET_ROOT_DIR/ccache

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
export XDG_CACHE_HOME=$TMP_DIR/xdgcache     # instead ~/.cache
export XDG_CONFIG_HOME=$TMP_DIR/xdgconfig   # instead ~/.config
export PIP_CACHE_DIR=$TMP_DIR/pip           # instead ~/Library/Caches/pip
export PIPENV_CACHE_DIR=$TMP_DIR/pipenv     # instead ~/Library/Caches/pipenv

# TODO: ~/Library/Caches/pip-tools

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

### bundled Python version #####################################################

PY3_MAJOR=3
PY3_MINOR=7
PY3_PATCH=6
PY3_BUILD=1  # custom framework build number

### download URLs for dependencies #############################################

# These are the versioned URLs of Inkscape dependencies that are not part of
# the JHBuild moduleset. (They are candidates for a custom Inkscape moduleset.)

# https://www.boost.org
URL_BOOST=https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.bz2
# https://www.freedesktop.org/wiki/Software/cppunit/
URL_CPPUNIT=https://dev-www.libreoffice.org/src/cppunit-1.14.0.tar.gz
# https://github.com/google/double-conversion
URL_DOUBLE_CONVERSION=https://github.com/google/double-conversion/archive/v3.1.5.tar.gz
# https://github.com/ivmai/bdwgc
URL_GC=https://github.com/ivmai/bdwgc/releases/download/v8.0.4/gc-8.0.4.tar.gz
# https://gitlab.gnome.org/GNOME/gdl
# This is one commit ahead of GDL_3_34_0.
# Fixes https://gitlab.gnome.org/GNOME/gdl/issues/2
URL_GDL=https://gitlab.gnome.org/GNOME/gdl/-/archive/9f11ad3ca8cef85b075419b30036d73648498dfe/gdl-9f11ad3ca8cef85b075419b30036d73648498dfe.tar.gz
# https://www.ghostscript.com
URL_GHOSTSCRIPT=https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/gs950/ghostscript-9.50.tar.gz
# https://www.gnu.org/software/gsl/
URL_GSL=http://ftp.fau.de/gnu/gsl/gsl-2.6.tar.gz
# Google Testing and Mocking Framework
# https://github.com/google/googletest
URL_GTEST=https://github.com/google/googletest/archive/release-1.10.0.tar.gz
# https://github.com/dehesselle/gtk-osx
# Forked from https://gitlab.gnome.org/GNOME/gtk-osx
URL_GTK_OSX=https://raw.githubusercontent.com/dehesselle/gtk-osx/inkscape-1.0.x-2
URL_GTK_OSX_SETUP=$URL_GTK_OSX/gtk-osx-setup.sh
URL_GTK_OSX_MODULESET=$URL_GTK_OSX/modulesets-stable/gtk-osx.modules
# https://legacy.imagemagick.org
URL_IMAGEMAGICK=https://github.com/ImageMagick/ImageMagick6/archive/6.9.10-89.tar.gz
# https://github.com/LibreOffice/libcdr
URL_LIBCDR=https://github.com/LibreOffice/libcdr/archive/libcdr-0.1.5.tar.gz
# https://sourceforge.net/p/libwpd/wiki/librevenge/
URL_LIBREVENGE=https://ayera.dl.sourceforge.net/project/libwpd/librevenge/librevenge-0.0.4/librevenge-0.0.4.tar.gz
# https://github.com/LibreOffice/libvisio
URL_LIBVISIO=https://github.com/LibreOffice/libvisio/archive/libvisio-0.1.7.tar.gz
# http://libwpg.sourceforge.net/libwpg.htm
URL_LIBWPG=https://netcologne.dl.sourceforge.net/project/libwpg/libwpg/libwpg-0.3.3/libwpg-0.3.3.tar.xz
# https://github.com/uclouvain/openjpeg
URL_OPENJPEG=https://github.com/uclouvain/openjpeg/archive/v2.3.1.tar.gz
# https://github.com/llvm/llvm-project
URL_OPENMP=https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/openmp-9.0.1.src.tar.xz
# https://poppler.freedesktop.org
URL_POPPLER=https://gitlab.freedesktop.org/poppler/poppler/-/archive/poppler-0.84.0/poppler-poppler-0.84.0.tar.gz
# http://potrace.sourceforge.net
URL_POTRACE=http://potrace.sourceforge.net/download/1.16/potrace-1.16.tar.gz

### download URLs for auxiliary software #######################################

# These are versioned URLs of software that is not a direct dependency but
# required for building, packaging or similar.

# compiler cache
URL_CCACHE=https://github.com/ccache/ccache/releases/download/v3.7.9/ccache-3.7.9.tar.xz
# create application bundle
# https://github.com/dehesselle/gtk-mac-bundler
# Forked from https://gitlab.gnome.org/GNOME/gtk-mac-bundler
URL_GTK_MAC_BUNDLER=https://github.com/dehesselle/gtk-mac-bundler/archive/f96a9daf2236814af7ace7a2fa91bbfb4f077779.tar.gz
# Inkscapge Git repo (for standalone/non-CI builds)
URL_INKSCAPE=https://gitlab.com/inkscape/inkscape
# disk image icon
URL_INKSCAPE_DMG_ICNS=https://github.com/dehesselle/mibap/raw/master/inkscape_dmg.icns
# convert PNG image to iconset in ICNS format
# https://github.com/bitboss-ca/png2icns
URL_PNG2ICNS=https://github.com/bitboss-ca/png2icns/archive/v0.1.tar.gz
# This is a relocatable Python.framework to be bundled with the app.
# https://github.com/dehesselle/py3framework
URL_PYTHON=https://github.com/dehesselle/py3framework/releases/download/py$PY3_MAJOR$PY3_MINOR$PY3_PATCH.$PY3_BUILD/py$PY3_MAJOR$PY3_MINOR${PY3_PATCH}_framework_$PY3_BUILD.tar.xz
# This Python 3.6 is only to setup JHBuild as it fails to download/install
# its own Python.
URL_PYTHON_JHBUILD=https://github.com/dehesselle/py3framework/releases/download/py3610.1/py3610_framework_1.tar.xz
# A pre-compiled version of the whole toolset.
# https://github.com/dehesselle/mibap
URL_TOOLSET=https://github.com/dehesselle/mibap/releases/download/v$TOOLSET_VERSION/mibap_v$TOOLSET_VERSION.dmg

### Python: packages for Inkscape ##############################################

# The following Python packages are bundled with Inkscape.

# https://cairocffi.readthedocs.io/en/stable/
# https://github.com/Kozea/cairocffi
PYTHON_CAIROCFFI=cairocffi==1.1.0
# https://lxml.de
# https://github.com/lxml/lxml
PYTHON_LXML=lxml==4.4.2
# https://github.com/lxml/lxml
PYTHON_NUMPY=numpy==1.18.1
# https://www.cairographics.org/pycairo/
# https://github.com/pygobject/pycairo
PYTHON_PYCAIRO=pycairo==1.19.0
# https://pygobject.readthedocs.io/en/latest/
PYTHON_PYGOBJECT=PyGObject==3.34.0
# https://github.com/scour-project/scour
PYTHON_SCOUR=scour==0.37
# https://pyserial.readthedocs.io/en/latest/
# https://github.com/pyserial/pyserial
PYTHON_PYSERIAL=pyserial==3.4

### Python: auxiliary packages #################################################

# convert SVG to PNG
# https://cairosvg.org
PYTHON_CAIROSVG=cairosvg==2.4.2
# create DMG
# https://dmgbuild.readthedocs.io/en/latest/
# https://github.com/al45tair/dmgbuild
PYTHON_DMGBUILD=dmgbuild==1.3.3

### path #######################################################################

export PATH=$DEVPREFIX/bin:$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin
