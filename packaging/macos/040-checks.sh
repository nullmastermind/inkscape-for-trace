# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 040-checks.sh ###
# Check basic prerequisites and break if they're not met.

[ -z $CHECKS_INCLUDED ] && CHECKS_INCLUDED=true || return   # include guard

### check if WRK_DIR is usable #################################################

if  [ $(mkdir -p $WRK_DIR 2>/dev/null; echo $?) -eq 0 ] &&
    [ -w $WRK_DIR ] ; then
  : # WRK_DIR has been created or was already there and is writable
else
  echo_e "WRK_DIR not usable: $WRK_DIR"
  exit 1
fi

### add more checks here #######################################################

