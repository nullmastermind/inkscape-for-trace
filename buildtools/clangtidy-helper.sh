#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
##
## usage: clangtidy-helper [list of cpp files]
##
## Without arguments, run for all source files.
##

set -e

mkdir -p build
cd build

export CC="ccache clang"
export CXX="ccache clang++"

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

#remove 3rdparty files from analysis, and only do those in inkscape_base target
jq -c '.[] | select( .file | contains("3rdparty") | not) | select(.command | contains("CMakeFiles/inkscape_base")) ' compile_commands.json | jq -s > compile_commands2.json
#treat 3rdparty code as system headers
sed -e 's/-I\([^ ]*\)\/src\/3rdparty/-isystem \1\/src\/3rdparty/g' compile_commands2.json > compile_commands.json && rm compile_commands2.json

run-clang-tidy -fix "$@" > /dev/null || true

# revert all fixes in protected directories
git checkout --recurse-submodules ../src/3rdparty

git diff | tee clang_tidy_diff

if [[ -s clang_tidy_diff ]]; then
    exit 1
fi
