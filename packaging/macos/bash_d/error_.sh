# SPDX-License-Identifier: GPL-2.0-or-later
# https://github.com/dehesselle/bash_d

include_guard

### includes ###################################################################

include_file ANSI_.sh
include_file echo_.sh

### variables ##################################################################

# Nothing here.

### functions ##################################################################

function error_catch
{
  local rc=$1

  local index=0
  local output

  while output=$(caller $index); do
    if [ $index -eq 0 ]; then
      echo_e "rc=$rc $ANSI_FG_YELLOW_BRIGHT$BASH_COMMAND$ANSI_FG_RESET"
    fi

    echo_e $output
    ((index+=1))
  done

  exit $rc
}

### aliases ####################################################################

alias error_trace_enable='set -o errtrace; trap '\''error_catch ${?}'\'' ERR'
alias error_trace_disable='trap - ERR'

### main #######################################################################

# Nothing here.