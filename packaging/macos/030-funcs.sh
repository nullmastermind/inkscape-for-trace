# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 030-funcs.sh ###
# This file contains all the functions used by the other scripts. It helps
# modularizing functionality and keeping the scripts that do the real work
# as readable as possible.
# This file does not include the "vars" files it requires itself (on purpose,
# for flexibility reasons), the script that wants to use these functions
# needs to do that. The suggested way is to always source all the "0nn-*.sh"
# files in order.

[ -z $FUNCS_INCLUDED ] && FUNCS_INCLUDED=true || return   # include guard

### get repository version string ##############################################

function get_repo_version
{
  local repo=$1
  #echo $(git -C $repo describe --tags --dirty)
  echo $(git -C $repo log --pretty=format:'%h' -n 1)
}

### get Inkscape version from CMakeLists.txt ###################################

function get_inkscape_version
{
  local file=$INK_DIR/CMakeLists.txt
  local ver_major=$(grep INKSCAPE_VERSION_MAJOR $file | head -n 1 | awk '{ print $2+0 }')
  local ver_minor=$(grep INKSCAPE_VERSION_MINOR $file | head -n 1 | awk '{ print $2+0 }')
  local ver_patch=$(grep INKSCAPE_VERSION_PATCH $file | head -n 1 | awk '{ print $2+0 }')
  local ver_suffix=$(grep INKSCAPE_VERSION_SUFFIX $file | head -n 1 | awk '{ print $2 }')
  
  ver_suffix=${ver_suffix%\"*}   # remove "double quote and everything after" from end
  ver_suffix=${ver_suffix#\"}   # remove "double quote" from beginning
 
  echo $ver_major.$ver_minor.$ver_patch$ver_suffix
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
  [ ! -d $SRC_DIR ] && mkdir -p $SRC_DIR

  cd $target_dir

  # This downloads a file and pipes it directly into tar (file is not saved
  # to disk) to extract it. Output is saved temporarily to determine
  # the directory the files have been extracted to.
  curl -L $url | tar xv$(get_comp_flag $url) 2>$log
  cd $(head -1 $log | awk '{ print $2 }')
  [ $? -eq 0 ] && rm $log || echo "$FUNCNAME: check $log"
}

### download a file and save to disk ###########################################

function save_file
{
  local url=$1
  local target_dir=$2   # optional argument, defaults to $SRC_DIR

  [ -z $target_dir ] && target_dir=$SRC_DIR

  local file=$SRC_DIR/$(basename $url)
  if [ -f $file ]; then
    echo "$FUNCNAME: file $file exists"
  else
    curl -o $file -L $url
  fi
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

### relocate a library dependency ##############################################

function relocate_dependency
{
  local target=$1    # fully qualified path and library name to new location
  local library=$2   # library where 'source' get changed to 'target'

  local source_lib=${target##*/}   # get library filename from target location
  local source=$(otool -L $library | grep $source_lib | awk '{ print $1 }')

  install_name_tool -change $source $target $library
}

### 'readlink -f' replacement ##################################################

# This is what the oneliner used to set SELF_DIR is based on.

function readlinkf
{
  # 'readlink -f' replacement: https://stackoverflow.com/a/1116890
  # 'do while' replacement: https://stackoverflow.com/a/16491478

  local file=$1

  # iterate down a (possible) chain of symlinks
  while
      [ ! -z $(readlink $file) ] && file=$(readlink $file)
      cd $(dirname $file)
      file=$(basename $file)
      [ -L "$file" ]
      do
    :
  done

  # Compute the canonicalized name by finding the physical path 
  # for the directory we're in and appending the target file.
  echo $(pwd -P)/$file
}

