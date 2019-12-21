#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 110-sysprep.sh ###
# System preparation tasks.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

run_annotated

### initial information ########################################################

echo_info "using TOOLSET_ROOT_DIR = $TOOLSET_ROOT_DIR" 

### create work directory ######################################################

[ ! -d $WRK_DIR ] && mkdir -p $WRK_DIR || true

### create temporary directory #################################################

[ ! -d $TMP_DIR ] && mkdir -p $TMP_DIR || true

### create toolset repository directory ########################################

[ ! -d $TOOLSET_REPO_DIR ] && mkdir -p $TOOLSET_REPO_DIR || true
