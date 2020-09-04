# SPDX-License-Identifier: GPL-2.0-or-later
# https://github.com/dehesselle/bash_d

include_guard

### includes ###################################################################

include_file ANSI_.sh

### variables ##################################################################

# Nothing here.

### functions ##################################################################

function _echo_message
{
  local name=$1    # optional
  local type=$2
  local color=$3
  local args=${@:4}

  [ -z $name ] && name=$ANSI_CURSOR_LEFT
  local color_reset=$ANSI_FG_RESET

  if [ ! -t 1 ]; then   # disable colors if not running in terminal
    color=""
    color_reset=""
  fi

  echo -e "$color[$type] $name$color_reset $args"
}

### aliases ####################################################################

alias echo_e='>&2 _echo_message "$FUNCNAME" "error" "$ANSI_FG_RED_BOLD"'
alias echo_i='>&2 _echo_message "$FUNCNAME" "info" "$ANSI_FG_BLUE_BOLD"'
alias echo_o='>&2 _echo_message "$FUNCNAME" "ok" "$ANSI_FG_GREEN_BOLD"'
alias echo_w='>&2 _echo_message "$FUNCNAME" "warning" "$ANSI_FG_YELLOW_BOLD"'

### main #######################################################################

# Nothing here.