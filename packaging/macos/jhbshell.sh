#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### jhbshell.sh ###
# Start a JHBuild shell.
# Since we have a customized JHBuild environment with lots of settings, this
# takes care of that.

### settings and functions #####################################################

for script in $(dirname ${BASH_SOURCE[0]})/0??-*.sh; do source $script; done

### invoke JHBuild shell #######################################################

jhbuild shell
