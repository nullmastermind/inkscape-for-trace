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

# You can build an macOS Mojave 10.14 using Xcode 10.3 using the SDK
# from OS X Mavericks 10.9 (part of Xcode 6.3).

export MACOSX_DEPLOYMENT_TARGET=10.9   # OS X Mavericks
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX${MACOSX_DEPLOYMENT_TARGET}.sdk

### ramdisk ####################################################################

# A ramdisk is used as writable overlay to a read-only disk image containing
# the build system.

RAMDISK_SIZE=4   # unit is GiB

### the main writable directory where all the action takes place ###############

[ -z $WRITABLE_DIR ] && WRITABLE_DIR=/Users/Shared/work

if  [ $(mkdir -p $WRITABLE_DIR 2>/dev/null; echo $?) -eq 0 ] &&
    [ -w $WRITABLE_DIR ] &&
    [ "$(stat -f '%Su' $WRITABLE_DIR)" = "$(whoami)" ] ; then
  echo "using build directory: $WRITABLE_DIR"
else
  echo "directory not usable: $WRITABLE_DIR"
  exit 1
fi

### build system paths  ########################################################

REPOSITORY_DIR=$WRITABLE_DIR/repo  # downloaded build systems (.dmg files)

WRK_DIR=$WRITABLE_DIR/1            # directory to mount build system to

OPT_DIR=$WRK_DIR/opt
BIN_DIR=$OPT_DIR/bin
LIB_DIR=$OPT_DIR/lib
SRC_DIR=$OPT_DIR/src
TMP_DIR=$OPT_DIR/tmp

### build system version #######################################################

VERSION_WANT=0.24
VERSION_HAVE=$([ -f $WRK_DIR/version.txt ] && cat $WRK_DIR/version.txt)

### use our TMP_DIR for everything temporary ###################################

export TMP=$TMP_DIR
export TEMP=$TMP_DIR
export TMPDIR=$TMP_DIR

### set jhbuild directories ####################################################

export DEVROOT=$WRK_DIR/gtk-osx
export DEVPREFIX=$DEVROOT/local
export DEV_SRC_ROOT=$DEVROOT/source
DEVCONFIG=$DEVROOT/config   # no export because this is an intermediate variable

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

URL_BOOST=https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2
URL_CPPUNIT=https://dev-www.libreoffice.org/src/cppunit-1.14.0.tar.gz
URL_DOUBLE_CONVERSION=https://github.com/google/double-conversion/archive/v3.1.4.tar.gz
URL_GC=https://github.com/ivmai/bdwgc/releases/download/v8.0.4/gc-8.0.4.tar.gz
URL_GDL=https://github.com/GNOME/gdl/archive/GDL_3_28_0.tar.gz
URL_GHOSTSCRIPT=https://github.com/ArtifexSoftware/ghostpdl-downloads/releases/download/gs927/ghostscript-9.27.tar.gz
URL_GSL=http://ftp.fau.de/gnu/gsl/gsl-2.5.tar.gz
URL_GTK_MAC_BUNDLER=https://gitlab.gnome.org/GNOME/gtk-mac-bundler/-/archive/93edee7e2d0ec8230aaf5acb21452202b10cd678.tar.gz
URL_GTK_OSX=https://raw.githubusercontent.com/dehesselle/gtk-osx/inkscape
URL_GTK_OSX_SETUP=$URL_GTK_OSX/gtk-osx-setup.sh
URL_GTK_OSX_MODULESET=$URL_GTK_OSX/modulesets-stable/gtk-osx.modules
URL_IMAGEMAGICK=https://github.com/ImageMagick/ImageMagick6/archive/6.9.7-10.tar.gz
URL_INKSCAPE=https://gitlab.com/inkscape/inkscape
URL_INKSCAPE_DMG_ICNS=https://github.com/dehesselle/mibap/raw/master/inkscape_dmg.icns
URL_LCMS2=https://netcologne.dl.sourceforge.net/project/lcms/lcms/2.9/lcms2-2.9.tar.gz
URL_LIBCDR=https://github.com/LibreOffice/libcdr/archive/libcdr-0.1.5.tar.gz
URL_LIBREVENGE=https://ayera.dl.sourceforge.net/project/libwpd/librevenge/librevenge-0.0.4/librevenge-0.0.4.tar.gz
URL_LIBVISIO=https://github.com/LibreOffice/libvisio/archive/libvisio-0.1.6.tar.gz
URL_LIBWPG=https://netcologne.dl.sourceforge.net/project/libwpg/libwpg/libwpg-0.3.3/libwpg-0.3.3.tar.xz
URL_OPENJPEG=https://github.com/uclouvain/openjpeg/archive/v2.3.0.tar.gz
URL_OPENMP=https://github.com/llvm/llvm-project/releases/download/llvmorg-7.1.0/openmp-7.1.0.src.tar.xz
URL_PNG2ICNS=https://github.com/bitboss-ca/png2icns/archive/v0.1.tar.gz
URL_POPPLER=https://gitlab.freedesktop.org/poppler/poppler/-/archive/poppler-0.74.0/poppler-poppler-0.74.0.tar.gz
URL_POTRACE=http://potrace.sourceforge.net/download/1.15/potrace-1.15.tar.gz
# This is the relocatable framework to be bundled with the app.
URL_PYTHON3_BIN=https://github.com/dehesselle/py3framework/releases/download/py374.3/py374_framework_3.tar.xz
# This is for JHBuild only.
URL_PYTHON36_SRC=https://github.com/dehesselle/py3framework/archive/py369.3.tar.gz
URL_PYTHON36_BIN=https://github.com/dehesselle/py3framework/releases/download/py369.3/py369_framework_3.tar.xz
URL_BUILDSYS=https://github.com/dehesselle/mibap/releases/download/v$VERSION_WANT/mibap_v$VERSION_WANT.dmg

### Python packages ############################################################

PYTHON_CAIROSVG=cairosvg==2.4.0
PYTHON_CAIROCFFI=cairocffi==1.0.2
PYTHON_DMGBUILD=dmgbuild==1.3.2
PYTHON_LXML=lxml==4.4.0
PYTHON_NUMPY=numpy==1.16.4   # 1.17.0 breaks (no investigation yet)
PYTHON_PYCAIRO=pycairo==1.18.1
PYTHON_PYGOBJECT=PyGObject==3.32.2
PYTHON_SCOUR=scour==0.37
PYTHON_PYSERIAL=pyserial==3.4

### profile ####################################################################

# Settings that would otherwise go into '.profile'.

export PATH=$DEVPREFIX/bin:$BIN_DIR:/usr/bin:/bin:/usr/sbin:/sbin
export LANG=de_DE.UTF-8   # jhbuild complains otherwise   FIXME hard-coded value

