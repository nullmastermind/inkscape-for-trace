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

set -e

run_annotated

### create application bundle ##################################################

mkdir -p $ARTIFACT_DIR

( # use subshell to fence temporary variables

  BUILD_DIR=$SRC_DIR/gtk-mac-bundler.build
  mkdir -p $BUILD_DIR

  cp $SRC_DIR/gtk-mac-bundler*/examples/gtk3-launcher.sh $BUILD_DIR
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

# Patch library link paths.

relocate_dependency @executable_path/../Resources/lib/inkscape/libinkscape_base.dylib $APP_EXE_DIR/Inkscape-bin

relocate_dependency @loader_path/../libpoppler.85.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib
relocate_dependency @loader_path/../libpoppler-glib.8.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib

relocate_neighbouring_libs $APP_LIB_DIR

# set Inkscape's data directory via environment variables
# TODO: as follow-up to https://gitlab.com/inkscape/inkscape/merge_requests/612,
# it should not be necessary to rely on $INKSCAPE_DATADIR. Paths in
# 'path-prefix.h' are supposed to work. Needs to be looked into.

insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export INKSCAPE_DATADIR=$bundle_data'
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export INKSCAPE_LOCALEDIR=$bundle_data/locale'

# add XDG paths to use native locations on macOS
insert_before $APP_EXE_DIR/Inkscape 'export XDG_CONFIG_DIRS' '\
export XDG_DATA_HOME=\"$HOME/Library/Application Support/Inkscape/data\"\
export XDG_CONFIG_HOME=\"$HOME/Library/Application Support/Inkscape/config\"\
export XDG_CACHE_HOME=\"$HOME/Library/Application Support/Inkscape/cache\"\
mkdir -p \"$XDG_DATA_HOME\"\
mkdir -p \"$XDG_CONFIG_HOME\"\
mkdir -p \"$XDG_CACHE_HOME\"\
'

# update Inkscape version information
/usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString '$(get_inkscape_version) ($(get_repo_version $INK_DIR))'" $APP_PLIST
/usr/libexec/PlistBuddy -c "Set CFBundleVersion '$(get_inkscape_version) ($(get_repo_version $INK_DIR))'" $APP_PLIST

# update minimum OS version
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
get_source file://$SRC_DIR/$(basename $URL_PYTHON3_BIN) $APP_FRA_DIR --exclude='Versions/3.7/lib/python3.7/test/*'

# add it to '$PATH' in launch script
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export PATH=$bundle_contents/Frameworks/Python.framework/Versions/Current/bin:$PATH'
# Add it to '$PATH' here and now (for package installation below). This is an
# exception: normally we'd not change the global environment but fence it
# using subshells.
export PATH=$APP_FRA_DIR/Python.framework/Versions/Current/bin:$PATH

# create '.pth' file inside Framework to include our site-packages directory
echo "./../../../../../../../Resources/lib/python3.7/site-packages" > $APP_FRA_DIR/Python.framework/Versions/Current/lib/python3.7/site-packages/inkscape.pth

### install Python package: lxml ###############################################

(
  export CFLAGS=-I$OPT_DIR/include/libxml2   # This became necessary when switching
  export LDFLAGS=-L/$LIB_DIR                 # from builing on 10.13 to 10.11.
  pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_LXML
)

# patch 'etree'
relocate_dependency @loader_path/../../../libxml2.2.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/etree.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libz.1.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/etree.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libxslt.1.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/etree.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libexslt.0.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/etree.cpython-37m-darwin.so
# patch 'objectify'
relocate_dependency @loader_path/../../../libxml2.2.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/objectify.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libz.1.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/objectify.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libxslt.1.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/objectify.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libexslt.0.dylib $APP_LIB_DIR/python3.7/site-packages/lxml/objectify.cpython-37m-darwin.so

### install Python package: NumPy ##############################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_NUMPY

### install Python package: PyGObject ##########################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_PYGOBJECT

# patch '_gi'
relocate_dependency @loader_path/../../../libglib-2.0.0.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libintl.9.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libgio-2.0.0.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libgobject-2.0.0.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libgirepository-1.0.1.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libffi.6.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi.cpython-37m-darwin.so

# patch '_gi_cairo'
relocate_dependency @loader_path/../../../libglib-2.0.0.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libintl.9.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libgio-2.0.0.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libgobject-2.0.0.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libgirepository-1.0.1.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libffi.6.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libcairo.2.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so
relocate_dependency @loader_path/../../../libcairo-gobject.2.dylib $APP_LIB_DIR/python3.7/site-packages/gi/_gi_cairo.cpython-37m-darwin.so

### install Python package: Pycairo ############################################

# PyGObject pulls in Pycairo, so not going to install again.
#pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_PYCAIRO

# patch '_cairo'
relocate_dependency @loader_path/../../../libcairo.2.dylib $APP_LIB_DIR/python3.7/site-packages/cairo/_cairo.cpython-37m-darwin.so

### install Python package: pySerial ###########################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_PYSERIAL

### install Python package: Scour ##############################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed $PYTHON_SCOUR

### precompile all Python packages #############################################

python3 -m compileall -f $APP_DIR || true

### set default Python interpreter #############################################

# If no override is present in 'preferences.xml' (see
# http://wiki.inkscape.org/wiki/index.php/Extension_Interpreters#Selecting_a_specific_interpreter_version_.28via_preferences_file.29
# ) we set the bundled Python to be the default one.

# Default interpreter is an unversioned environment lookup for 'python', so
# we prepare to override it.
mkdir -p $APP_BIN_DIR/python_override
cd $APP_BIN_DIR/python_override
ln -sf ../../../Frameworks/Python.framework/Versions/Current/bin/python3 python

# add override check to launch script
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' '\
INKSCAPE_PREFERENCES=\"$HOME/Library/Application Support/Inkscape/config/inkscape/preferences.xml\"\
if [ -f \"$INKSCAPE_PREFERENCES\" ]; then            # Has Inkscape been launched before?\
  PYTHON_INTERPRETER=$\(xmllint --xpath '\''string\(//inkscape/group[@id=\"extensions\"]/@python-interpreter\)'\'' \"$INKSCAPE_PREFERENCES\"\)\
  if [ -z $PYTHON_INTERPRETER ]\; then               # No override for Python interpreter?\
    export PATH=$bundle_bin/python_override:$PATH    # make bundled Python default one\
  fi\
else                                                 # Inkscape has not been launched before?\
  export PATH=$bundle_bin/python_override:$PATH      # make bundled Python default one\
fi\
'

### fontconfig #################################################################

# Mimic the behavior of having all files under 'share' and linking the
# active ones to 'etc'.
cd $APP_ETC_DIR/fonts/conf.d

for file in ./*.conf; do
  ln -sf ../../../share/fontconfig/conf.avail/$(basename $file)
done

# Set environment variable so fontconfig looks in the right place for its
# files. (https://www.freedesktop.org/software/fontconfig/fontconfig-user.html)
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' \
  'export FONTCONFIG_PATH=$bundle_res/etc/fonts'

# Our customized version loses all the non-macOS paths and sets a cache
# directory below '$HOME/Library/Application Support/Inkscape'.
cp $SELF_DIR/fonts.conf $APP_ETC_DIR/fonts

### GIO modules path ###########################################################

# Set environment variable for GIO to find its modules.
# (https://developer.gnome.org/gio//2.52/running-gio-apps.html)
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export GIO_MODULE_DIR=$bundle_lib/gio/modules'

### fix messages in GTK launch script ##########################################

# Some parts of the GTK launch script cause unnecessary messages.

# silence "does not exist" message
replace_line $APP_EXE_DIR/Inkscape AppleCollationOrder \
  'APPLECOLLATION=$(defaults read .GlobalPreferences AppleCollationOrder 2>/dev/null)'

# add quotes so test does not complain to missing (because empty) argument
replace_line $APP_EXE_DIR/Inkscape '-a -n $APPLECOLLATION;' \
  'if test -z ${LANG} -a -n "$APPLECOLLATION"; then'

### Ghostscript ################################################################

relocate_dependency @executable_path/../lib/libz.1.dylib $APP_BIN_DIR/gs
relocate_dependency @executable_path/../lib/libfontconfig.1.dylib $APP_BIN_DIR/gs
relocate_dependency @executable_path/../lib/libfreetype.6.dylib $APP_BIN_DIR/gs

### final changes to PATH ######################################################

insert_before $APP_EXE_DIR/Inkscape '\$EXEC' \
  'export PATH=$bundle_bin:$PATH'

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

# tell GObject where to find the repository
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' \
  'export GI_TYPELIB_PATH=$bundle_lib/girepository-1.0'

# set library path so dlopen() can find libraries without fully qualified paths
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' \
  'export DYLD_LIBRARY_PATH=$bundle_lib'

