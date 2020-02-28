#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

testfile=$1
regex=$2

test -f "${testfile}" || { echo "match_regex.sh: testfile '${testfile}' not found."; exit 1; }
test -n "${regex}"    || { echo "match_regex.sh: no regex to match spoecified."; exit 1; }

if grep -E "${regex}" "${testfile}"; then
    echo "match_regex.sh: regex '${regex}' matches in testfile '${testfile}'."
    exit 1
fi
