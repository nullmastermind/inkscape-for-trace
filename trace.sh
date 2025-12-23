#!/usr/bin/env bash
set -euo pipefail

# Inkscape batch tracing script for pixel art images
# Usage: ./trace.sh [input.png] [output.svg]

INPUT="${1:-input.png}"
OUTPUT="${2:-output.svg}"

./build/bin/inkscape --actions="select-all;selection-trace:256,false,true,true,4,1.0,0.20;export-filename:${OUTPUT};export-do;" "${INPUT}" --batch-process

