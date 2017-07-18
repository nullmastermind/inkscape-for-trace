#!/usr/bin/env bash

### functions
message() { echo -e "\e[1;32m\n${1}\n\e[0m"; }
warning() { echo -e "\e[1;33m\nWarning: ${1}\n\e[0m"; }
error()   { echo -e "\e[1;31m\nError: ${1}\n\e[0m";  exit 1; }



### setup

# do everything in /build
cd "$(cygpath ${APPVEYOR_BUILD_FOLDER})"
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
source ../buildtools/msys2installdeps.sh
pacman -S $MINGW_PACKAGE_PREFIX-{ccache,gtest,ntldd-git} --needed --noconfirm --noprogressbar
ccache --max-size=300M
ccache --set-config=sloppiness=include_file_ctime,include_file_mtime



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
appveyor SetVariable -Name APPVEYOR_SAVE_CACHE_ON_ERROR -Value true # build succeeded so it's safe to save the cache

# install
message "--- Installing the project"
ninja install || error "installation failed"
python  ../buildtools/msys2checkdeps.py check inkscape/ || error "missing libraries in installed project"

# test
message "--- Running tests"
# check if the installed executable works
inkscape/inkscape.exe -V || error "installed executable won't run"
PATH= inkscape/inkscape.exe -V >/dev/null || error "installed executable won't run with empty PATH (missing dependecies?)"
err=$(PATH= inkscape/inkscape.exe -V 2>&1 >/dev/null)
if [ -n "$err" ]; then warning "installed executable produces output on stderr:"; echo "$err"; fi
# check if the uninstalled executable works
INKSCAPE_DATADIR=../share bin/inkscape.exe -V >/dev/null || error "uninstalled executable won't run"
err=$(INKSCAPE_DATADIR=../share bin/inkscape.exe -V 2>&1 >/dev/null)
if [ -n "$err" ]; then warning "uninstalled executable produces output on stderr:"; echo "$err"; fi
# run tests (don't fail yet as most tests SEGFAULT on exit)
#ninja check || warning "tests failed" # disabled because of sporadic deadlocks :-(

message "##### BUILD SUCCESSFULL #####\n\n"


### package
BRANCH=$(git branch | tail -n 1 | tr -d ' ')
DATE=$(git log -n 1 --pretty=%cd --date=short)
HASH=$(git rev-parse --short HEAD)
7z a "inkscape-${BRANCH}-(${DATE}_${HASH})-${MSYSTEM_CARCH}.7z" inkscape
