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

cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

jq -c '.[] | select( .file | contains("3rdparty") | not)' compile_commands.json | jq -s > compile_commands2.json
mv compile_commands2.json compile_commands.json

run-clang-tidy -quiet -fix -header-filter='.*' "$@" > /dev/null || true

# revert all fixes in protected directories
git checkout --recurse-submodules ../src/3rdparty

git diff | tee clang_tidy_diff

if [[ -s clang_tidy_diff ]]; then
    exit 1
fi
