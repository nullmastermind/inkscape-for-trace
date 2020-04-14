#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 220-inkscape-package.sh ###
# Create Inkscape application bundle.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -o errtrace
trap 'catch_error "$SELF_NAME" "$LINENO" "$FUNCNAME" "${BASH_COMMAND}" "${?}"' ERR

### create application bundle ##################################################

mkdir -p $ARTIFACT_DIR

( # use subshell to fence temporary variables

  BUILD_DIR=$SRC_DIR/gtk-mac-bundler.build
  mkdir -p $BUILD_DIR

  cp $SELF_DIR/inkscape.bundle $BUILD_DIR
  cp $SELF_DIR/inkscape.plist $BUILD_DIR

  # Due to an undiagnosed instability that only occurs during CI runs (not when
  # run interactively from the terminal), the following code will be put into
  # a separate script and be executed via Terminal.app.

  cat <<EOF >$SRC_DIR/run_gtk-mac-bundler.sh
#!/usr/bin/env bash
SCRIPT_DIR=$SELF_DIR
for script in \$SCRIPT_DIR/0??-*.sh; do source \$script; done
export ARTIFACT_DIR
BUILD_DIR=$BUILD_DIR
cd \$BUILD_DIR
jhbuild run gtk-mac-bundler inkscape.bundle
EOF
)

chmod 755 $SRC_DIR/run_gtk-mac-bundler.sh
run_in_terminal $SRC_DIR/run_gtk-mac-bundler.sh

# Rename to get from lowercase to capitalized "i". This works only on
# case-insensitive filesystems (default on macOS).

mv $APP_DIR $APP_DIR.tmp
mv $APP_DIR.tmp $APP_DIR

# Patch library link paths.

relocate_dependency @executable_path/../Resources/lib/inkscape/libinkscape_base.dylib $APP_EXE_DIR/inkscape

relocate_dependency @loader_path/../libpoppler.94.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib
relocate_dependency @loader_path/../libpoppler-glib.8.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib

relocate_neighbouring_libs $APP_LIB_DIR

# update Inkscape version information
/usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString '$(get_inkscape_version) ($(get_repo_version $INK_DIR))'" $APP_PLIST
/usr/libexec/PlistBuddy -c "Set CFBundleVersion '$(get_inkscape_version) ($(get_repo_version $INK_DIR))'" $APP_PLIST

# update minimum system version according to deployment target
/usr/libexec/PlistBuddy -c "Set LSMinimumSystemVersion '$MACOSX_DEPLOYMENT_TARGET'" $APP_PLIST

### generate application icon ##################################################

# svg to png

(
  export DYLD_FALLBACK_LIBRARY_PATH=$LIB_DIR
  jhbuild run cairosvg -f png -s 8 -o $SRC_DIR/inkscape.png $INK_DIR/share/branding/inkscape.svg
)

# png to icns

cd $SRC_DIR   # png2icns.sh outputs to current directory
png2icns.sh inkscape.png
mv inkscape.icns $APP_RES_DIR

### bundle Python.framework ####################################################

# This section deals with bundling Python.framework into the application.

mkdir $APP_FRA_DIR
install_source file://$SRC_DIR/$(basename $URL_PYTHON) $APP_FRA_DIR --exclude="Versions/$PY3_MAJOR.$PY3_MINOR/lib/python$PY3_MAJOR.$PY3_MINOR/test/"'*'

# link it to $APP_BIN_DIR so it'll be in $PATH for the app
ln -sf ../../Frameworks/Python.framework/Versions/Current/bin/python$PY3_MAJOR $APP_BIN_DIR

# create '.pth' file inside Framework to include our site-packages directory
echo "./../../../../../../../Resources/lib/python$PY3_MAJOR.$PY3_MINOR/site-packages" > $APP_FRA_DIR/Python.framework/Versions/Current/lib/python$PY3_MAJOR.$PY3_MINOR/site-packages/inkscape.pth

### install Python package: lxml ###############################################

pip_install $PYTHON_LXML

# patch 'etree'
relocate_dependency @loader_path/../../../libxml2.2.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/etree.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libz.1.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/etree.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libxslt.1.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/etree.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libexslt.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/etree.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
# patch 'objectify'
relocate_dependency @loader_path/../../../libxml2.2.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/objectify.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libz.1.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/objectify.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libxslt.1.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/objectify.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libexslt.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/lxml/objectify.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so

### install Python package: NumPy ##############################################

pip_install $PYTHON_NUMPY

### install Python package: PyGObject ##########################################

pip_install $PYTHON_PYGOBJECT

# patch '_gi'
relocate_dependency @loader_path/../../../libglib-2.0.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libintl.9.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libgio-2.0.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libgobject-2.0.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libgirepository-1.0.1.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libffi.6.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so

# patch '_gi_cairo'
relocate_dependency @loader_path/../../../libglib-2.0.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libintl.9.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libgio-2.0.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libgobject-2.0.0.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libgirepository-1.0.1.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libffi.6.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libcairo.2.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so
relocate_dependency @loader_path/../../../libcairo-gobject.2.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/gi/_gi_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so

### install Python package: Pycairo ############################################

# PyGObject pulls in Pycairo, so not going to install again.
#pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_PYCAIRO

# patch '_cairo'
relocate_dependency @loader_path/../../../libcairo.2.dylib $APP_LIB_DIR/python$PY3_MAJOR.$PY3_MINOR/site-packages/cairo/_cairo.cpython-$PY3_MAJOR${PY3_MINOR}m-darwin.so

### install Python package: pySerial ###########################################

pip_install $PYTHON_PYSERIAL

### install Python package: Scour ##############################################

pip_install $PYTHON_SCOUR

### precompile all Python packages #############################################

$APP_FRA_DIR/Python.framework/Versions/Current/bin/python$PY3_MAJOR -m compileall -f $APP_DIR || true

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

### Ghostscript ################################################################

relocate_dependency @executable_path/../lib/libz.1.dylib $APP_BIN_DIR/gs
relocate_dependency @executable_path/../lib/libfontconfig.1.dylib $APP_BIN_DIR/gs
relocate_dependency @executable_path/../lib/libfreetype.6.dylib $APP_BIN_DIR/gs

### create GObject introspection repository ####################################

mkdir $APP_LIB_DIR/girepository-1.0

# remove fully qualified paths from libraries in *.gir files
for gir in $OPT_DIR/share/gir-1.0/*.gir; do
  sed "s/$(escape_sed $LIB_DIR/)//g" $gir > $SRC_DIR/$(basename $gir)
done

# compile *.gir into *.typelib files
for gir in $SRC_DIR/*.gir; do
  jhbuild run g-ir-compiler -o $APP_LIB_DIR/girepository-1.0/$(basename -s .gir $gir).typelib $gir
done
