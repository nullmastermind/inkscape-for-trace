# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### escape replacement string for sed ##########################################

# Escape slashes, backslashes and ampersands in strings to be used s as
# replacement strings when using 'sed'. Newlines are not taken into
# consideartion here.
# reference: https://stackoverflow.com/a/2705678

function escape_sed
{
  local string="$*"

  echo "$string" | sed -e 's/[\/&]/\\&/g'
}