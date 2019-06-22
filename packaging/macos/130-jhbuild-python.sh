#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 130-jhbuild-python.sh ###
# Install Python 2 and 3.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install Python 2 ###########################################################

# Some packages complain about non-exiting development headers when you rely
# solely on the macOS-provided Python installation. This also enables
# system-wide installations of packages without permission issues.

jhbuild build python

cd $SRC_DIR
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
jhbuild run python get-pip.py
jhbuild run pip install six   # required for a package in meta-gtk-osx-bootstrap

