#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 210-inkscape.sh ###
# Build Inscape and create an application bundle.

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### build Inkscape #############################################################

if [ -z $CI_JOB_ID ]; then   # running standalone
  cd $SRC_DIR
  git clone --depth 1 $URL_INKSCAPE
  #git clone $URL_INKSCAPE   # this is a >1.6 GiB download
  mkdir inkscape_build
  cd inkscape_build
  cmake -DCMAKE_PREFIX_PATH=$OPT_DIR -DCMAKE_INSTALL_PREFIX=$OPT_DIR -DWITH_OPENMP=OFF ../inkscape
else   # running as CI job
  if [ -d $SELF_DIR/../../build ]; then   # cleanup previous run
    rm -rf $SELF_DIR/../../build
  fi
  mkdir $SELF_DIR/../../build
  cd $SELF_DIR/../../build
  cmake -DCMAKE_PREFIX_PATH=$OPT_DIR -DCMAKE_INSTALL_PREFIX=$OPT_DIR -DWITH_OPENMP=OFF ..
fi

make
make install

# patch library locations before packaging
install_name_tool -change @rpath/libpoppler.85.dylib $LIB_DIR/libpoppler.85.dylib $BIN_DIR/inkscape
install_name_tool -change @rpath/libpoppler-glib.8.dylib $LIB_DIR/libpoppler-glib.8.dylib $BIN_DIR/inkscape

### package Inkscape ###########################################################

if [ ! -z $CI_JOB_ID ]; then   # running as CI job
  # $SELF_DIR is different between "standalone" and "CI job"
  cp $SRC_DIR/gtk-mac-bundler*/examples/gtk3-launcher.sh $SELF_DIR
fi

cd $SELF_DIR
jhbuild run gtk-mac-bundler inkscape.bundle

# patch library locations
install_name_tool -change @rpath/libinkscape_base.dylib @executable_path/../Resources/lib/inkscape/libinkscape_base.dylib $APP_EXE_DIR/Inkscape-bin
install_name_tool -change @rpath/libpoppler.85.dylib @executable_path/../Resources/lib/libpoppler.85.dylib $APP_LIB_DIR/libpoppler-glib.8.dylib
install_name_tool -change @rpath/libpoppler.85.dylib @executable_path/../Resources/lib/libpoppler.85.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib
install_name_tool -change @rpath/libpoppler-glib.8.dylib @executable_path/../Resources/lib/libpoppler-glib.8.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib

# add INKSCAPE_DATADIR to launch script
# TODO
# - instead of deleting and re-inserting, insert with "sed before" pattern
# - As follow-up to https://gitlab.com/inkscape/inkscape/merge_requests/612,
#   it should not be necessary to rely on $INKSCAPE_DATADIR. Paths in
#   'path-prefix.h' are supposed to work. Needs to be looked into with @ede123.
sed -i '' -e '$d' $APP_EXE_DIR/Inkscape
sed -i '' -e '$d' $APP_EXE_DIR/Inkscape
echo 'export INKSCAPE_DATADIR=$bundle_data' >> $APP_EXE_DIR/Inkscape
echo '$EXEC "$bundle_contents/MacOS/$name-bin" "$@" $EXTRA_ARGS' >> $APP_EXE_DIR/Inkscape

# add icon
curl -L -o $APP_RES_DIR/inkscape.icns $URL_INKSCAPE_ICNS

set -e   # TODO kind of cheap, need better error handling

if [ -z $CI_JOB_ID ]; then   # running standalone
  # update version information
  /usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString '1.0alpha-g$(get_repo_version $SRC_DIR/inkscape)'" $APP_PLIST
  /usr/libexec/PlistBuddy -c "Set CFBundleVersion '1.0alpha-g$(get_repo_version $SRC_DIR/inkscape)'" $APP_PLIST
else   # running as CI job
  # update version information
  /usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString '1.0alpha-g$(get_repo_version $SELF_DIR)'" $APP_PLIST
  /usr/libexec/PlistBuddy -c "Set CFBundleVersion '1.0alpha-g$(get_repo_version $SELF_DIR)'" $APP_PLIST
fi

### create disk image for distribution #########################################

# TODO

if [ ! -z $CI_JOB_ID ]; then   # create build artifcat for CI job
  cd $WRK_DIR
  tar c $(basename $APP_DIR) | xz > $SELF_DIR/../../build/Inkscape.tar.xz
fi
