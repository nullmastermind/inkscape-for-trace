# SPDX-License-Identifier: GPL-2.0-or-later

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced
# shellcheck disable=SC2034 # globally defined variables here w/o export

### description ################################################################

# This file contains everything related to setup ccache.

### variables ##################################################################

if [ -z "$CCACHE_DIR" ]; then
  CCACHE_DIR=$WRK_DIR/ccache
fi
export CCACHE_DIR

# https://ccache.dev
# https://github.com/ccache/ccache
CCACHE_V3_VER=3.7.12
CCACHE_V3_URL=https://github.com/ccache/ccache/releases/download/\
v$CCACHE_V3_VER/ccache-$CCACHE_V3_VER.tar.xz
CCACHE_V4_VER=4.2
CCACHE_V4_URL=https://github.com/ccache/ccache/releases/download/\
v$CCACHE_V4_VER/ccache-$CCACHE_V4_VER.tar.xz

### functions ##################################################################

function ccache_configure
{
    mkdir -p "$CCACHE_DIR"

  cat <<EOF > "$CCACHE_DIR/ccache.conf"
base_dir = $VER_DIR
hash_dir = false
max_size = 3.0G
temporary_dir = $CCACHE_DIR/tmp
EOF
}

function ccache_v3_install
{
  local archive
  archive=$PKG_DIR/$(basename $CCACHE_V3_URL)
  curl -o "$archive" -L "$CCACHE_V3_URL"
  tar -C "$SRC_DIR" -xf "$archive"
  # shellcheck disable=SC2164 # we trap errors to catch bad 'cd'
  cd "$SRC_DIR"/ccache-$CCACHE_V3_VER

  ./configure --prefix="$VER_DIR"
  make
  make install

  for compiler in clang clang++ gcc g++; do
    ln -s ccache "$BIN_DIR"/$compiler
  done
}

function ccache_v4_install
{
  local archive
  archive=$PKG_DIR/$(basename $CCACHE_V4_URL)
  curl -o "$archive" -L "$CCACHE_V4_URL"
  tar -C "$SRC_DIR" -xf "$archive"

  mkdir -p "$BLD_DIR"/ccache-$CCACHE_V4_VER
  # shellcheck disable=SC2164 # we trap errors to catch bad 'cd'
  cd "$BLD_DIR"/ccache-$CCACHE_V4_VER

  cmake_install
  ninja_install
  cmake_run \
    -DCMAKE_BUILD_TYPE=Release \
    -DZSTD_FROM_INTERNET=ON \
    -DCMAKE_INSTALL_PREFIX="$VER_DIR" \
    -GNinja \
    "$SRC_DIR"/ccache-$CCACHE_V4_VER
  ninja
  ninja install
  cmake_uninstall   # cleaning up because cmake gets pulled in by JHBuild later

  for compiler in clang clang++ gcc g++; do
    ln -s ccache "$BIN_DIR"/$compiler
  done
}