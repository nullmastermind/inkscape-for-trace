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
  # do it the same way as in CMakeScripts/inkscape-verson.cmake
  echo $(git -C $repo rev-parse --short HEAD)
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
  local target_dir=$2   # optional: target directory, defaults to $SRC_DIR
  local options=$3      # optional: additional options for 'tar'

  [ ! -d $TMP_DIR ] && mkdir -p $TMP_DIR
  local log=$(mktemp $TMP_DIR/$FUNCNAME.XXXX)
  [ -z $target_dir ] && target_dir=$SRC_DIR
  [ ! -d $SRC_DIR ] && mkdir -p $SRC_DIR

  cd $target_dir

  # This downloads a file and pipes it directly into tar (file is not saved
  # to disk) to extract it. Output from stderr is saved temporarily to 
  # determine the directory the files have been extracted to.
  curl -L $url | tar xv$(get_comp_flag $url) $options 2>$log
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

# There is a more common approach to do this using
#    diskutil eraseVolume HFS+ VolName $(hdiutil attach -nomount ram://<size>)
# but that always attaches the ramdisk below '/Volumes'.
# To have full control, we need to do it as follows.

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

# usage:
# insert_before <filename> <insert before this pattern> <line to insert>

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

### replace line that matches pattern ##########################################

function replace_line
{
  local file=$1
  local pattern=$2
  local replacement=$3

  sed -i '' "s/.*${pattern}.*/$(escape_sed $replacement)/" $file
}

### relocate a library dependency ##############################################

function relocate_dependency
{
  local target=$1    # fully qualified path and library name to new location
  local library=$2   # library to be modified (change 'source' to 'target'I

  local source_lib=${target##*/}   # get library filename from target location
  local source=$(otool -L $library | grep $source_lib | awk '{ print $1 }')

  install_name_tool -change $source $target $library
}

### relocate all neighbouring libraries in a directory #########################

function relocate_neighbouring_libs
{
  local dir=$1

  for lib in $dir/*.dylib; do
    for linked_lib in $(otool -L $lib | tail -n +3 | awk '{ print $1 }'); do
      if [ -f $APP_LIB_DIR/$(basename $linked_lib) ]; then
        relocate_dependency @loader_path/$(basename $linked_lib) $lib
      fi
    done
  done
}

### 'readlink -f' replacement ##################################################

# This is what the oneliner setting SELF_DIR (see top of file) is based on.

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

### run script and echo comments enclosed by three hashes ######################

# This little magic trick
#   - reads the current file
#   - turns every line that matches "### text here ###" into an echo statement
#     (whatever follows after the last three hashes is ignored)
#   - adds script name and line number as prefix
#   - sets background color for that 'echo' to blue
#   - removes the call to 'run_annotated' to avoid recursion
#   - sets SELF_DIR to the correct value (piping into bash breaks it)
#
# Known side effects:
#   - SELF_NAME no longer works (is now "bash")
#   - interactive JHBuild (e.g. on error) breaks

function run_annotated
{
  # The newlines in the last 'sed' statement are significant!

  sed 's/\(^### .* ###\).*/echo \-e "\\033[1;44m\\033[1;37m['$SELF_NAME':$(printf '%03d' $LINENO)] \1\\033[0m"/g' $SELF_DIR/$SELF_NAME | sed 's/^run_annotated/#run_annotated/' | sed '/SELF_DIR=/a\
SELF_DIR='$SELF_DIR'\
' | bash

  exit $?
}

### create disk image ##########################################################

function create_dmg
{
  local app=$1
  local dmg=$2
  local cfg=$3

  # set application
  sed -i '' "s/PLACEHOLDERAPPLICATION/$(escape_sed $app)/" $cfg
  
  # set disk image icon (if it exists)
  local icon=$SRC_DIR/$(basename -s .py $cfg).icns
  if [ -f $icon ]; then
    sed -i '' "s/PLACEHOLDERICON/$(escape_sed $icon)/" $cfg
  fi

  # set background image (if it exists)
  local background=$SRC_DIR/$(basename -s .py $cfg).png
  if [ -f $background ]; then
    sed -i '' "s/PLACEHOLDERBACKGROUND/$(escape_sed $background)/" $cfg
  fi

  # create disk image
  dmgbuild -s $cfg "$(basename -s .app $app)" $dmg
}

### run script via Terminal.app ################################################

# This is for commands that otherwise don't behave when running in CI.
# It requires a running desktop session (i.e. logged in user) and special
# permissions on newer macOS versions.
# Adapted from: https://stackoverflow.com/a/27970527
#               https://gist.github.com/masci/ff51d9cf40a87a80094c

function run_in_terminal
{
  local command=$1

  #local window_id=$(uuidgen)      # this would be really unique but...
  local window_id=$(date +%s)      # ...seconds would help more with debugging

  osascript <<EOF
tell application "Terminal"
  set _tab to do script "echo -n -e \"\\\033]0;$window_id\\\007\"; $command; exit"
  delay 1
  repeat while _tab is busy
    delay 1
  end repeat
  close (every window whose name contains "$window_id")
end tell
EOF
}

