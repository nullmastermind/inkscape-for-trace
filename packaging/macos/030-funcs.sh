# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 030-funcs.sh ###
# This file contains all the functions used by the other scripts. It helps
# modularizing functionalities and keeping the scripts that do the real work
# as clean as possible.
# This file does not include the "vars" files it requires itself (on purpose,
# for flexibility reasons), the script that wants to use these functions
# needs to do that. The suggest way is to always source all the "0nn-*.sh"
# files in order.

[ -z $FUNCS_INCLUDED ] && FUNCS_INCLUDED=true || return   # include guard

### get repository version string ##############################################

function get_repo_version
{
  local repo=$1
  #echo $(git -C $repo describe --tags --dirty)
  echo $(git -C $repo log --pretty=format:'%h' -n 1)
}

### get compression flag by filename extension #################################

function get_comp_flag
{
  local file=$1

  local extension=${file##*.}

  case $extension in
    gz) echo "z"  ;;
    bz2) echo "j" ;;
    xz) echo "J"  ;;
    *) echo "ERROR unknown extension $extension"
  esac
}

### download and extract source tarball ########################################

function get_source
{
  local url=$1
  local target_dir=$2   # optional argument, defaults to $SRC_DIR

  [ ! -d $TMP_DIR ] && mkdir -p $TMP_DIR
  local log=$(mktemp $TMP_DIR/$FUNCNAME.XXXX)
  [ -z $target_dir ] && target_dir=$SRC_DIR

  cd $target_dir

  # This downloads a file and pipes it directly into tar (file is not saved
  # to disk) to extract it. Output is saved temporarily to determine
  # the directory the files have been extracted to.
  curl -L $url | tar xv$(get_comp_flag $url) 2>$log
  cd $(head -1 $log | awk '{ print $2 }')
  [ $? -eq 0 ] && rm $log || echo "$FUNCNAME: check $log"
}

### make, make install in jhbuild environment ##################################

function make_makeinstall
{
  jhbuild run make
  jhbuild run make install
}

### configure, make, make install in jhbuild environment #######################

function configure_make_makeinstall
{
  local flags="$*"

  jhbuild run ./configure --prefix=$OPT_DIR $flags
  make_makeinstall
}

### cmake, make, make install in jhbuild environment ###########################

function cmake_make_makeinstall
{
  local flags="$*"

  mkdir builddir
  cd builddir
  jhbuild run cmake -DCMAKE_INSTALL_PREFIX=$OPT_DIR $flags ..
  make_makeinstall
}

### create and mount ramdisk ###################################################

function create_ramdisk
{
  local dir=$1    # mountpoint
  local size=$2   # unit is GiB

  if [ $(mount | grep $dir | wc -l) -eq 0 ]; then
    local device=$(hdiutil attach -nomount ram://$(expr $size \* 1024 \* 2048))
    newfs_hfs -v "RAMDISK" $device
    mount -o noatime,nobrowse -t hfs $device $dir
  fi
}

### insert line into a textfile ################################################

# insert_before [filename] '[insert before this pattern]' '[line to insert]'

function insert_before 
{
  local file=$1
  local pattern=$2
  local line=$3

  local file_tmp=$(mktemp)
  awk "/${pattern}/{print \"$line\"}1" $file > $file_tmp
  cat $file_tmp > $file   # we don't 'mv' to preserve permissions
  rm $file_tmp
}

