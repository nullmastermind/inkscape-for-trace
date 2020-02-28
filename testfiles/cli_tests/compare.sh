#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

file1=$1
file2=$2

test -f "${file1}" || { echo "compare.sh: First file '${file1}' not found."; exit 1; }
test -f "${file2}" || { echo "compare.sh: Second file '${file2}' not found."; exit 1; }

if ! cmp "${file1}" "${file2}"; then
    echo "compare.sh: Files '${file1}' and '${file2}' are not identical'."
    exit 1
fi
