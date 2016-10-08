#!/usr/bin/python

from __future__ import print_function

import os
import sys

# check where to look for the inkscape files to bundle
#    - btool builds used [root]/inkscape
#    - cmake builds use [root]/build/inkscape unless "DESTDIR" is specified
def get_inkscape_dist_dir():
    # fisrt check the environment variable
    sourcedir = os.getenv('INKSCAPE_DIST_PATH')
    if sourcedir is None:
        sourcedir = ''
    if not os.path.isdir(sourcedir):
        sourcedir = '..\\..\\inkscape'
    if not os.path.isdir(sourcedir):
        sourcedir = '..\\..\\build\\inkscape'
    if not os.path.isdir(sourcedir):
        print("could not find inkscape distribution files, please set environment variable 'INKSCAPE_DIST_PATH'")
        sys.exit(1)
    return sourcedir