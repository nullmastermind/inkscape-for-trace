#!/usr/bin/env bash
set -euo pipefail

# Inkscape batch tracing script for pixel art images
# Usage: ./trace.sh [input.png] [output.svg]
#
# Uses --headless flag for truly headless operation (no GUI, suitable for automation)

INPUT="${1:-input.png}"
OUTPUT="${2:-output.svg}"

./build/bin/inkscape --headless --actions="select-all;selection-trace:256,false,true,true,2,1.0,0.20;export-filename:${OUTPUT};export-do;" "${INPUT}"
