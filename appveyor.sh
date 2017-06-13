#!/usr/bin/env bash

cd "$(dirname "$0")"
mkdir build
cd build



# Write an empty fonts.conf to speed up fc-cache
export FONTCONFIG_FILE=/dummy-fonts.conf
cat >"$FONTCONFIG_FILE" <<EOF
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig></fontconfig>
EOF



# install dependencies
pacman -S $MINGW_PACKAGE_PREFIX-ccache --needed --noconfirm --noprogressbar
source ../msys2installdeps.sh

# configure
ccache --max-size=200M
cmake .. -G Ninja -DCMAKE_C_COMPILER_LAUNCHER="ccache" -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" || exit 1

# build
ccache --zero-stats
ninja || exit 1
ccache --show-stats

# install
ninja install || exit 1

# test
inkscape/inkscape.exe -V || exit 1

# package
7z a inkscape.7z inkscape
