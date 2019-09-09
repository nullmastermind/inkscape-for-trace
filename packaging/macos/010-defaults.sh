# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 010-defaults.sh ###
# This file contains a few default values that YOU ARE NOT SUPPOSED TO CHANGE
# and are for this reason only in '010-defaults.sh' and not in '020-vars.sh'.
# There are certain checks and dependencies that are out of your control
# and the other scripts blindly trust that the defaults set in this file
# are "the truth". (I you continue to read, you'll hopefully see that
# this has not been done out of malicious intent but for good reasons.)
# You have been warned.

[ -z $DEFAULTS_INCLUDED ] && DEFAULTS_INCLUDED=true || return   # include guard

### path for pre-built build environment #######################################

# This is the path the pre-built build environment has been built in. Therefore,
# all binaries and libraries have hard-coded absolute library locations to this
# path. If you decide to not build the build environment yourself and want to
# use the pre-built one, it is essential that the WRK_DIR you are using is the
# same as DEFAULT_WRK_DIR, otherwise it doesn't work.
# The scripts will make their decision if the pre-built build environment can be
# used based on comparing values between WRK_DIR and DEFAULT_WRK_DIR.

DEFAULT_WRK_DIR=/Users/Shared/work

