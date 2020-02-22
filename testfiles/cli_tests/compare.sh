#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

command -v convert >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'convert' but it's not installed.  Aborting."; exit 1; }
command -v compare >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'compare' but it's not installed.  Aborting."; exit 1; }

OUTPUT_FILENAME=$1
REFERENCE_FILENAME=$2

convert ${OUTPUT_FILENAME} ${OUTPUT_FILENAME}.png
convert ${REFERENCE_FILENAME} ${OUTPUT_FILENAME}_reference.png

compare -metric AE ${OUTPUT_FILENAME}.png ${OUTPUT_FILENAME}_reference.png ${OUTPUT_FILENAME}-compare.png
exit $?
