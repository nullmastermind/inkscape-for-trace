# SPDX-License-Identifier: GPL-2.0-or-later

### description ################################################################

# This file contains everything related to Inkscape.

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced
# shellcheck disable=SC2034 # no exports desired

### variables ##################################################################

#----------------------------------------------- source directory and git branch

# If we're running inside Inkscape's official CI, the repository is already
# there and we adjust INK_DIR accordingly.
# If not, check if a custom repository location and/or branch has been
# specified in the environment.

if $CI_GITLAB; then   # running GitLab CI
  INK_DIR=$SELF_DIR/../..
else                  # not running GitLab CI
  INK_DIR=$SRC_DIR/inkscape

  # Allow using a custom Inkscape repository and branch.
  if [ -z "$INK_URL" ]; then
    INK_URL=https://gitlab.com/inkscape/inkscape
  fi

  # Allow using a custom branch.
  if [ -z "$INK_BRANCH" ]; then
    INK_BRANCH=master
  fi
fi

INK_BLD_DIR=$BLD_DIR/$(basename "$INK_DIR")  # we build out-of-tree

#----------------------------------- Python runtime to be  bundled with Inkscape

# Inkscape will be bundled with its own (customized) Python 3 runtime to make
# the core extensions work out-of-the-box. This is independent from whatever
# Python is running JHBuild or getting built as a dependency.

INK_PYTHON_VER_MAJOR=3
INK_PYTHON_VER_MINOR=8
INK_PYTHON_VER_PATCH=5
INK_PYTHON_VER_BUILD=2  # custom build version

INK_PYTHON_VER=$INK_PYTHON_VER_MAJOR.$INK_PYTHON_VER_MINOR  # convenience handle

# https://github.com/dehesselle/py3framework
INK_PYTHON_URL=https://github.com/dehesselle/py3framework/releases/download/\
py${INK_PYTHON_VER/./}$INK_PYTHON_VER_PATCH.$INK_PYTHON_VER_BUILD/\
py${INK_PYTHON_VER/./}${INK_PYTHON_VER_PATCH}_framework_${INK_PYTHON_VER_BUILD}i.tar.xz

#----------------------------------- Python packages to be bundled with Inkscape

# https://lxml.de
# https://github.com/lxml/lxml
# https://github.com/dehesselle/py3framework
# TODO: check and document why we're using our own build here
INK_PYTHON_LXML=$(dirname $INK_PYTHON_URL)/lxml-4.5.2-\
cp${INK_PYTHON_VER/./}-cp${INK_PYTHON_VER/./}-macosx_10_9_x86_64.whl

# https://github.com/numpy/numpy
INK_PYTHON_NUMPY=numpy==1.19.1

# https://pygobject.readthedocs.io/en/latest/
INK_PYTHON_PYGOBJECT=PyGObject==3.36.1

# https://github.com/scour-project/scour
INK_PYTHON_SCOUR=scour==0.37

# https://pyserial.readthedocs.io/en/latest/
# https://github.com/pyserial/pyserial
INK_PYTHON_PYSERIAL=pyserial==3.5

#------------------------------------------- application bundle directory layout

INK_APP_DIR=$ARTIFACT_DIR/Inkscape.app

INK_APP_CON_DIR=$INK_APP_DIR/Contents
INK_APP_RES_DIR=$INK_APP_CON_DIR/Resources
INK_APP_FRA_DIR=$INK_APP_CON_DIR/Frameworks
INK_APP_BIN_DIR=$INK_APP_RES_DIR/bin
INK_APP_ETC_DIR=$INK_APP_RES_DIR/etc
INK_APP_EXE_DIR=$INK_APP_CON_DIR/MacOS
INK_APP_LIB_DIR=$INK_APP_RES_DIR/lib

INK_APP_SITEPKG_DIR=$INK_APP_LIB_DIR/python$INK_PYTHON_VER/site-packages

### functions ##################################################################

function ink_get_version
{
  local file=$INK_DIR/CMakeLists.txt
  local ver_major
  ver_major=$(grep INKSCAPE_VERSION_MAJOR "$file" | head -n 1 | awk '{ print $2+0 }')
  local ver_minor
  ver_minor=$(grep INKSCAPE_VERSION_MINOR "$file" | head -n 1 | awk '{ print $2+0 }')
  local ver_patch
  ver_patch=$(grep INKSCAPE_VERSION_PATCH "$file" | head -n 1 | awk '{ print $2+0 }')
  local ver_suffix
  ver_suffix=$(grep INKSCAPE_VERSION_SUFFIX "$file" | head -n 1 | awk '{ print $2 }')

  ver_suffix=${ver_suffix%\"*}   # remove "double quote and everything after" from end
  ver_suffix=${ver_suffix#\"}   # remove "double quote" from beginning

  # shellcheck disable=SC2086 # they are integers
  echo $ver_major.$ver_minor.$ver_patch"$ver_suffix"
}

function ink_get_repo_shorthash
{
  # do it the same way as in CMakeScripts/inkscape-version.cmake
  git -C "$INK_DIR" rev-parse --short HEAD
}

function ink_pipinstall
{
  local package=$1
  local options=$2   # optional

  local PATH_ORIGINAL=$PATH
  export PATH=$INK_APP_FRA_DIR/Python.framework/Versions/Current/bin:$PATH

  # shellcheck disable=SC2086 # we need word splitting here
  pip$INK_PYTHON_VER_MAJOR install \
    --prefix "$INK_APP_RES_DIR" \
    --ignore-installed \
    $options \
    $package

  export PATH=$PATH_ORIGINAL
}

function ink_python_download
{
  curl -o "$PKG_DIR"/"$(basename "$INK_PYTHON_URL")" -L "$INK_PYTHON_URL"
}