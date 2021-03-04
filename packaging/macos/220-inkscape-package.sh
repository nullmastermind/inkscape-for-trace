#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 220-inkscape-package.sh ###
# Create Inkscape application bundle.

### settings and functions #####################################################

for script in $(dirname ${BASH_SOURCE[0]})/0??-*.sh; do source $script; done

include_file ansi_.sh
include_file error_.sh
error_trace_enable

ANSI_TERM_ONLY=false   # use ANSI control characters even if not in terminal

### create application bundle ##################################################

mkdir -p $ARTIFACT_DIR

( # use subshell to fence temporary variables

  BUILD_DIR=$SRC_DIR/gtk-mac-bundler.build
  mkdir -p $BUILD_DIR

  cp $SELF_DIR/inkscape.bundle $BUILD_DIR
  cp $SELF_DIR/inkscape.plist $BUILD_DIR

  export ARTIFACT_DIR   # referenced in inkscape.bundle
  cd $BUILD_DIR
  jhbuild run gtk-mac-bundler inkscape.bundle
)

# Rename to get from lowercase to capitalized "i". This works only on
# case-insensitive filesystems (default on macOS).

mv $APP_DIR $APP_DIR.tmp
mv $APP_DIR.tmp $APP_DIR

# patch library link paths for lib2geom
lib_change_path \
  @executable_path/../Resources/lib/lib2geom.1.1.0.dylib \
  $APP_LIB_DIR/inkscape/libinkscape_base.dylib

# patch library link path for libboost_filesystem
lib_change_path \
  @executable_path/../Resources/lib/libboost_filesystem.dylib \
  $APP_LIB_DIR/inkscape/libinkscape_base.dylib \
  $APP_EXE_DIR/inkscape

# patch library link path for libinkscape_base
lib_change_path \
  @executable_path/../Resources/lib/inkscape/libinkscape_base.dylib \
  $APP_EXE_DIR/inkscape

lib_change_siblings $APP_LIB_DIR

( # use subshell to fence temporary variables

  PLIST=$APP_CON_DIR/Info.plist
  IV=$(get_inkscape_version)
  RV=$(get_repo_version $INK_DIR)

  # update Inkscape version information
  /usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString '$IV ($RV)'" $PLIST
  /usr/libexec/PlistBuddy -c "Set CFBundleVersion '$IV ($RV)'" $PLIST

  # update minimum system version according to deployment target
  /usr/libexec/PlistBuddy -c "Set LSMinimumSystemVersion '$SDK_VER'" $PLIST
)

### generate application icon ##################################################

# svg to png

(
  export DYLD_FALLBACK_LIBRARY_PATH=$LIB_DIR
  jhbuild run cairosvg -f png -s 1 -o $SRC_DIR/inkscape.png \
    $INK_DIR/share/branding/inkscape-mac.svg
)

# png to icns

cd $SRC_DIR   # png2icns.sh outputs to current directory
png2icns.sh inkscape.png
mv inkscape.icns $APP_RES_DIR

### add file type icons ########################################################

cp $INK_DIR/packaging/macos/resources/*.icns $APP_RES_DIR

### bundle Python.framework ####################################################

# This section deals with bundling Python.framework into the application.

mkdir $APP_FRA_DIR
install_source file://$PKG_DIR/$(basename $PY3_URL) $APP_FRA_DIR \
  --exclude="Versions/$PY3_MAJOR.$PY3_MINOR/lib/python$PY3_MAJOR.$PY3_MINOR/test/"'*'

# link it to $APP_BIN_DIR so it'll be in $PATH for the app
mkdir -p $APP_BIN_DIR
ln -sf ../../Frameworks/Python.framework/Versions/Current/bin/python$PY3_MAJOR $APP_BIN_DIR

# create '.pth' file inside Framework to include our site-packages directory
echo "./../../../../../../../Resources/lib/python$PY3_MAJOR.$PY3_MINOR/site-packages" \
  > $APP_FRA_DIR/Python.framework/Versions/Current/lib/python$PY3_MAJOR.$PY3_MINOR/site-packages/inkscape.pth

### install Python package: lxml ###############################################

pip_install $PYTHON_LXML

lib_change_paths \
  @loader_path/../../.. \
  $APP_LIB_DIR \
  $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/etree.cpython-$PY3_MAJOR${PY3_MINOR}-darwin.so \
  $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/objectify.cpython-$PY3_MAJOR${PY3_MINOR}-darwin.so

### install Python package: NumPy ##############################################

pip_install $PYTHON_NUMPY
sed -i '' '1s/.*/#!\/usr\/bin\/env python3/' $APP_BIN_DIR/f2py
sed -i '' '1s/.*/#!\/usr\/bin\/env python3/' $APP_BIN_DIR/f2py3
sed -i '' '1s/.*/#!\/usr\/bin\/env python3/' $APP_BIN_DIR/f2py3.8

### install Python package: PyGObject ##########################################

pip_install $PYTHON_PYGOBJECT

lib_change_paths \
  @loader_path/../../.. \
  $APP_LIB_DIR \
  $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}-darwin.so \
  $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}-darwin.so

### install Python package: Pycairo ############################################

# This package got pulled in by PyGObject.

# patch '_cairo'
lib_change_paths \
  @loader_path/../../.. \
  $APP_LIB_DIR \
  $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/cairo/_cairo.cpython-$PY3_MAJOR${PY3_MINOR}-darwin.so

### install Python package: pySerial ###########################################

pip_install $PYTHON_PYSERIAL
find $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/serial \
  -type f -name "*.pyc" -exec rm {} \;
sed -i '' '1s/.*/#!\/usr\/bin\/env python3/' $APP_BIN_DIR/pyserial-miniterm
sed -i '' '1s/.*/#!\/usr\/bin\/env python3/' $APP_BIN_DIR/pyserial-ports

### install Python package: Scour ##############################################

pip_install $PYTHON_SCOUR
sed -i '' '1s/.*/#!\/usr\/bin\/env python3/' $APP_BIN_DIR/scour

### remove Python cache files ##################################################

rm -rf $APP_RES_DIR/share/glib-2.0/codegen/__pycache__

### fontconfig #################################################################

# Mimic the behavior of having all files under 'share' and linking the
# active ones to 'etc'.
cd $APP_ETC_DIR/fonts/conf.d

for file in ./*.conf; do
  ln -sf ../../../share/fontconfig/conf.avail/$(basename $file)
done

# Our customized version loses all the non-macOS paths and sets a cache
# directory below '$HOME/Library/Application Support/Inkscape'.
cp $SELF_DIR/fonts.conf $APP_ETC_DIR/fonts

### create GObject introspection repository ####################################

mkdir $APP_LIB_DIR/girepository-1.0

# remove fully qualified paths from libraries in *.gir files
for gir in $VER_DIR/share/gir-1.0/*.gir; do
  sed "s/$(escape_sed $LIB_DIR/)//g" $gir > $SRC_DIR/$(basename $gir)
done

# compile *.gir into *.typelib files
for gir in $SRC_DIR/*.gir; do
  jhbuild run g-ir-compiler \
    -o $APP_LIB_DIR/girepository-1.0/$(basename -s .gir $gir).typelib $gir
done
