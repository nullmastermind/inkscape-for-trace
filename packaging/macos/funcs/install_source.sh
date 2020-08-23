# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.

### download and extract source tarball ########################################

function install_source
{
  local url=$1
  local target_dir=$2   # optional: target directory, defaults to $SRC_DIR
  local options=$3      # optional: additional options for 'tar'

  [ ! -d $TMP_DIR ] && mkdir -p $TMP_DIR
  local log=$(mktemp $TMP_DIR/$FUNCNAME.XXXX)
  [ -z $target_dir ] && target_dir=$SRC_DIR
  [ ! -d $target_dir ] && mkdir -p $target_dir

  cd $target_dir

  # This downloads a file and pipes it directly into tar (file is not saved
  # to disk) to extract it. Output from stderr is saved temporarily to
  # determine the directory the files have been extracted to.
  curl -L $url | tar xv$(get_comp_flag $url) $options 2>$log
  cd $(head -1 $log | awk '{ print $2 }')
  [ $? -eq 0 ] && rm $log || echo "$FUNCNAME: check $log"
}