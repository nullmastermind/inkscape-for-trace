# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 030-funcs.sh ###
# This file once contained all the functions used by the other scripts to
# help with modularization. Since then, this file has been split up.
# This file does not include the "vars" files it requires itself (on purpose,
# for flexibility reasons), the script that wants to use these functions
# needs to do that. The suggested way is to always source all the "0nn-*.sh"
# files in order.

[ -z $FUNCS_INCLUDED ] && FUNCS_INCLUDED=true || return   # include guard

# Include common functions from https://github.com/dehesselle/bash_d

INCLUDE_DIR=$SELF_DIR/bash_d
source $INCLUDE_DIR/1_include_.sh
include_file echo_.sh
include_file lib_.sh

# Include custom functions.

for func in $SELF_DIR/funcs/*.sh; do
  source $func
done
