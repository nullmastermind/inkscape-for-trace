#!/usr/bin/env bash

### functions
message() { echo -e "\e[1;32m\n${1}\n\e[0m"; }
error() { echo -e "\e[1;31m\nError: ${1}\n\e[0m"; exit 1; }



### setup

# do everything in /build
cd "$(dirname "$0")"
mkdir build
cd build

# write an empty fonts.conf to speed up fc-cache
export FONTCONFIG_FILE=/dummy-fonts.conf
cat >"$FONTCONFIG_FILE" <<EOF
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig></fontconfig>
EOF

# install dependencies
message "--- Installing dependencies"
source ../msys2installdeps.sh
pacman -S $MINGW_PACKAGE_PREFIX-ccache --needed --noconfirm --noprogressbar
ccache --max-size=200M



### build / test

message "\n\n##### STARTING BUILD #####"

# configure
message "--- Configuring the build"
cmake .. -G Ninja \
    -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
    -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
    -DCMAKE_INSTALL_MESSAGE="NEVER" \
    || error "cmake failed"

# build
message "--- Compiling Inkscape"
ccache --zero-stats
ninja || error "compilation failed"
ccache --show-stats

# install
message "--- Installing the project"
ninja install || error "installation failed"

# test
message "--- Running tests"
inkscape/inkscape.exe -V || error "tests failed"

message "##### BUILD SUCCESSFULL #####\n\n"


### package
7z a inkscape.7z inkscape
