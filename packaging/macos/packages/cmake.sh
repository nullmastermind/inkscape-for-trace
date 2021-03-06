# SPDX-License-Identifier: GPL-2.0-or-later

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced
# shellcheck disable=SC2034 # globally defined variables here w/o export

### description ################################################################

# This file contains everything related to setup cmake.

### variables ##################################################################

# https://pypi.org/project/cmake/
CMAKE_PIP=cmake==3.18.4.post1

### functions ##################################################################

function cmake_install
{
  pip3 install --ignore-installed --prefix "$VER_DIR" "$CMAKE_PIP"
}

function cmake_uninstall
{
  PYTHONPATH=$LIB_DIR/python$SYS_PYTHON_VER/site-packages \
    pip3 uninstall -y "${CMAKE_PIP%%=*}"
}

function cmake_run
{
  local args="$*"
  # shellcheck disable=SC2086 # we want splitting here
  PYTHONPATH=$LIB_DIR/python$SYS_PYTHON_VER/site-packages cmake $args
}