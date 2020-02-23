#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

command -v convert >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'convert' but it's not installed.  Aborting."; exit 1; }
command -v compare >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'compare' but it's not installed.  Aborting."; exit 1; }

OUTPUT_FILENAME=$1
REFERENCE_FILENAME=$2
EXPECTED_FILES=$3

# check if expected files exist
for file in ${EXPECTED_FILES}; do
    test -f "${file}" || { echo "Error: Expected file '${file}' not found."; exit 1; }
done

# if reference file is given check if input files exist and continue with comparison
test -n "${REFERENCE_FILENAME}" || exit 0
if [ ! -f "${OUTPUT_FILENAME}" ]; then
    echo "Error: Test file '${OUTPUT_FILENAME}' not found."
    exit 1
fi
if [ ! -f "${REFERENCE_FILENAME}" ]; then
    echo "Error: Reference file '${REFERENCE_FILENAME}' not found."
    exit 1
fi

# convert testfile and reference file to PNG format
if ! convert ${OUTPUT_FILENAME} ${OUTPUT_FILENAME}.png; then
    echo "Warning: Failed to convert test file '${OUTPUT_FILENAME}' to PNG format. Skipping comparison test."
    exit 1
fi
if ! convert ${REFERENCE_FILENAME} ${OUTPUT_FILENAME}_reference.png; then
    echo "Warning: Failed to convert reference file '${REFERENCE_FILENAME}' to PNG format. Skipping comparison test."
    exit 1
fi

# compare files
if ! compare -metric AE ${OUTPUT_FILENAME}.png ${OUTPUT_FILENAME}_reference.png ${OUTPUT_FILENAME}_compare.png; then
    echo && echo "Error: Comparison failed."
    exit 1
fi
