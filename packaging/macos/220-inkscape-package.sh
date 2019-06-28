#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 220-inkscape-package.sh ###
# Create Inkscape application bundle.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

set -e

### package Inkscape ###########################################################

mkdir -p $ARTIFACT_DIR
export    ARTIFACT_DIR   # referenced in 'inkscape.bundle'

cp $SRC_DIR/gtk-mac-bundler*/examples/gtk3-launcher.sh $SELF_DIR
cd $SELF_DIR
jhbuild run gtk-mac-bundler inkscape.bundle

# patch library locations
relocate_dependency @executable_path/../Resources/lib/inkscape/libinkscape_base.dylib $APP_EXE_DIR/Inkscape-bin

relocate_dependency @executable_path/../Resources/lib/libpoppler.85.dylib $APP_LIB_DIR/libpoppler-glib.8.dylib
relocate_dependency @executable_path/../Resources/lib/libpoppler.85.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib
relocate_dependency @executable_path/../Resources/lib/libpoppler-glib.8.dylib $APP_LIB_DIR/inkscape/libinkscape_base.dylib

relocate_dependency @loader_path/libcrypto.1.1.dylib $APP_LIB_DIR/libssl.1.1.dylib

# set Inkscape's data directory via environment variables
# TODO: as follow-up to https://gitlab.com/inkscape/inkscape/merge_requests/612,
# it should not be necessary to rely on $INKSCAPE_DATADIR. Paths in
# 'path-prefix.h' are supposed to work. Needs to be looked into.

insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export INKSCAPE_DATADIR=$bundle_data'
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export INKSCAPE_LOCALEDIR=$bundle_data/locale'

# add XDG paths to use native locations on macOS
insert_before $APP_EXE_DIR/Inkscape 'export XDG_CONFIG_DIRS' '\
export XDG_DATA_HOME=\"$HOME/Library/Application\\ Support/Inkscape/data\"\
export XDG_CONFIG_HOME=\"$HOME/Library/Application Support/Inkscape/config\"\
export XDG_CACHE_HOME=\"$HOME/Library/Application Support/Inkscape/cache\"\
mkdir -p $XDG_DATA_HOME\
mkdir -p $XDG_CONFIG_HOME\
mkdir -p $XDG_CACHE_HOME\
'

# update Inkscape version information
/usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString '$(get_inkscape_version) ($(get_repo_version $INK_DIR))'" $APP_PLIST
/usr/libexec/PlistBuddy -c "Set CFBundleVersion '$(get_inkscape_version) ($(get_repo_version $INK_DIR))'" $APP_PLIST

# update minimum OS version
/usr/libexec/PlistBuddy -c "Set LSMinimumSystemVersion '$MACOSX_DEPLOYMENT_TARGET'" $APP_PLIST

### generate application icon ##################################################

# svg to png

jhbuild run pip3 install cairosvg==2.4.0
jhbuild run pip3 install cairocffi==1.0.2

(
  export DYLD_FALLBACK_LIBRARY_PATH=/work/opt/lib
  jhbuild run cairosvg -f png -s 8 -o $SRC_DIR/inkscape.png $INK_DIR/share/branding/inkscape.svg
)

# png to icns

get_source $URL_PNG2ICNS
./png2icns.sh $SRC_DIR/inkscape.png
mv inkscape.icns $APP_RES_DIR

### bundle Python.framework ####################################################

# This section deals with bundling Python.framework into the application.

save_file $URL_PYTHON3   # download a pre-built Python.framework

mkdir $APP_FRA_DIR
get_source file://$SRC_DIR/$(basename $URL_PYTHON3) $APP_FRA_DIR

# add it to '$PATH' in launch script
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' 'export PATH=$bundle_contents/Frameworks/Python.framework/Versions/Current/bin:$PATH'
# add it to '$PATH' here and now (for package installation below)
export PATH=$APP_FRA_DIR/Python.framework/Versions/Current/bin:$PATH

# create '.pth' file inside Framework to include our site-packages directory
echo "./../../../../../../../Resources/lib/python3.6/site-packages" > $APP_FRA_DIR/Python.framework/Versions/Current/lib/python3.6/site-packages/inkscape.pth

### install Python package: lxml ###############################################

(
  export CFLAGS=-I$OPT_DIR/include/libxml2   # This became necessary when switching
  export LDFLAGS=-L/$LIB_DIR                 # from builing on 10.13 to 10.11.
  pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed lxml==4.3.3
)

# patch 'etree'
relocate_dependency @loader_path/../../../libxml2.2.dylib $APP_LIB_DIR/python3.6/site-packages/lxml/etree.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libz.1.dylib $APP_LIB_DIR/python3.6/site-packages/lxml/etree.cpython-36m-darwin.so
# patch 'objectify'
relocate_dependency @loader_path/../../../libxml2.2.dylib $APP_LIB_DIR/python3.6/site-packages/lxml/objectify.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libz.1.dylib $APP_LIB_DIR/python3.6/site-packages/lxml/objectify.cpython-36m-darwin.so

# patch libxml2.dylib to use '@loader_path' to find neighbouring libraries
relocate_dependency @loader_path/libz.1.dylib $APP_LIB_DIR/libxml2.2.dylib
relocate_dependency @loader_path/liblzma.5.dylib $APP_LIB_DIR/libxml2.2.dylib

### install Python package: NumPy ##############################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed numpy==1.16.4

### install Python package: Pycairo ############################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed pycairo==1.18.1

# patch '_cairo'
relocate_dependency @loader_path/../../../libcairo.2.dylib $APP_LIB_DIR/python3.6/site-packages/cairo/_cairo.cpython-36m-darwin.so

# patch libcairo.2.dylib to use '@loader_path' to find neighbouring libraries
relocate_dependency @loader_path/libpixman-1.0.dylib $APP_LIB_DIR/libcairo.2.dylib
relocate_dependency @loader_path/libfontconfig.1.dylib $APP_LIB_DIR/libcairo.2.dylib
relocate_dependency @loader_path/libfreetype.6.dylib $APP_LIB_DIR/libcairo.2.dylib
relocate_dependency @loader_path/libpng16.16.dylib $APP_LIB_DIR/libcairo.2.dylib
relocate_dependency @loader_path/libz.1.dylib $APP_LIB_DIR/libcairo.2.dylib

### install Python package: PyGObject ##########################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed PyGObject==3.32.1

# patch '_gi'
relocate_dependency @loader_path/../../../libglib-2.0.0.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libintl.9.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libgio-2.0.0.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libgobject-2.0.0.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libgirepository-1.0.1.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libffi.6.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi.cpython-36m-darwin.so

# patch libglib-2.0.0.dylib to find neighbouring libraries
relocate_dependency @loader_path/libintl.9.dylib $APP_LIB_DIR/libglib-2.0.0.dylib

# patch libgio-2.0.0.dylib to find neighbouring libraries
relocate_dependency @loader_path/libgobject-2.0.0.dylib $APP_LIB_DIR/libgio-2.0.0.dylib
relocate_dependency @loader_path/libffi.6.dylib $APP_LIB_DIR/libgio-2.0.0.dylib
relocate_dependency @loader_path/libgmodule-2.0.0.dylib $APP_LIB_DIR/libgio-2.0.0.dylib
relocate_dependency @loader_path/libglib-2.0.0.dylib $APP_LIB_DIR/libgio-2.0.0.dylib
relocate_dependency @loader_path/libz.1.dylib $APP_LIB_DIR/libgio-2.0.0.dylib
relocate_dependency @loader_path/libintl.9.dylib $APP_LIB_DIR/libgio-2.0.0.dylib

# patch libgobject-2.0.0.dylib to find neighbouring libraries
relocate_dependency @loader_path/libglib-2.0.0.dylib $APP_LIB_DIR/libgobject-2.0.0.dylib
relocate_dependency @loader_path/libffi.6.dylib $APP_LIB_DIR/libgobject-2.0.0.dylib
relocate_dependency @loader_path/libintl.9.dylib $APP_LIB_DIR/libgobject-2.0.0.dylib

# patch libgirepository-1.0.1.dylib to find neighbouring libraries
relocate_dependency @loader_path/libgmodule-2.0.0.dylib $APP_LIB_DIR/libgirepository-1.0.1.dylib
relocate_dependency @loader_path/libgio-2.0.0.dylib $APP_LIB_DIR/libgirepository-1.0.1.dylib
relocate_dependency @loader_path/libgobject-2.0.0.dylib $APP_LIB_DIR/libgirepository-1.0.1.dylib
relocate_dependency @loader_path/libglib-2.0.0.dylib $APP_LIB_DIR/libgirepository-1.0.1.dylib
relocate_dependency @loader_path/libintl.9.dylib $APP_LIB_DIR/libgirepository-1.0.1.dylib
relocate_dependency @loader_path/libffi.6.dylib $APP_LIB_DIR/libgirepository-1.0.1.dylib

# patch '_gi_cairo'
relocate_dependency @loader_path/../../../libglib-2.0.0.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libintl.9.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libgio-2.0.0.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libgobject-2.0.0.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libgirepository-1.0.1.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libffi.6.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libcairo.2.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so
relocate_dependency @loader_path/../../../libcairo-gobject.2.dylib $APP_LIB_DIR/python3.6/site-packages/gi/_gi_cairo.cpython-36m-darwin.so

# patch libcairo-gobject.2.dylib
relocate_dependency @loader_path/libcairo.2.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libpixman-1.0.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libfontconfig.1.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libfreetype.6.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libpng16.16.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libz.1.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libgobject-2.0.0.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libglib-2.0.0.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib
relocate_dependency @loader_path/libintl.9.dylib $APP_LIB_DIR/libcairo-gobject.2.dylib

# patch libgmodule-2.0.0.dylib
relocate_dependency @loader_path/libglib-2.0.0.dylib $APP_LIB_DIR/libgmodule-2.0.0.dylib
relocate_dependency @loader_path/libintl.9.dylib $APP_LIB_DIR/libgmodule-2.0.0.dylib

### install Python package: Scour ##############################################

pip3 install --install-option="--prefix=$APP_RES_DIR" --ignore-installed scour==0.37

### set default Python interpreter #############################################

# If no override is present in 'preferences.xml' (see
# http://wiki.inkscape.org/wiki/index.php/Extension_Interpreters#Selecting_a_specific_interpreter_version_.28via_preferences_file.29
# ) we set the bundled Python to be the default one.

# Default interpreter is an unversioned environment lookup for 'python', so
# we prepar to override it.
mkdir -p $APP_BIN_DIR
cd $APP_BIN_DIR
ln -sf ../../Frameworks/Python.framework/Versions/Current/bin/python3 python

# add override check to launch script
insert_before $APP_EXE_DIR/Inkscape '\$EXEC' '\
INKSCAPE_PREFERENCES=$HOME/Library/Application\\ Support/Inkscape/config/inkscape/preferences.xml\
if [ -f $INKSCAPE_PREFERENCES ]; then   # Has Inkscape been launched before?\
  PYTHON_INTERPRETER=$\(xmllint --xpath '\''string\(//inkscape/group[@id=\"extensions\"]/@python-interpreter\)'\'' $INKSCAPE_PREFERENCES\)\
  if [ -z $PYTHON_INTERPRETER ]\; then   # No override for Python interpreter?\
    export PATH=$bundle_bin:$PATH        # make bundled Python default one\
  fi\
else                                     # Inkscape has not been launched before\
  export PATH=$bundle_bin:$PATH          # make bundled Python default one\
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

