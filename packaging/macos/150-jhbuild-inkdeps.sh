#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 150-jhbuild-inkdeps.sh ###
# Install additional dependencies into our jhbuild environment required for
# building Inkscape.

### load settings and functions ################################################

SELF_DIR=$(F=$0; while [ ! -z $(readlink $F) ] && F=$(readlink $F); cd $(dirname $F); F=$(basename $F); [ -L $F ]; do :; done; echo $(pwd -P))
for script in $SELF_DIR/0??-*.sh; do source $script; done

run_annotated

### install additional GNOME libraries #########################################

# adwaita-icon-theme - icons used by Inkscape/GTK
# gtkspell3 - GtkSpell spellchecking/highlighting
# libsoup - GNOME http client/server library
# lcms2 - little CMS colore engine v2 

jhbuild build \
  adwaita-icon-theme \
  gtkspell3 \
  libsoup \
  lcms

### install GNU Scientific Library #############################################

get_source $URL_GSL
configure_make_makeinstall

### install Garbage Collector for C/C++ ########################################

get_source $URL_GC
configure_make_makeinstall

### install GNOME Docking Library ##############################################

get_source $URL_GDL
patch -p1 < $SELF_DIR/gdl_disable_preview_window.patch
jhbuild run ./autogen.sh
configure_make_makeinstall

### install boost ##############################################################

get_source $URL_BOOST
jhbuild run ./bootstrap.sh --prefix=$OPT_DIR
jhbuild run ./b2 -j$CORES install

### install OpenJPEG ###########################################################

get_source $URL_OPENJPEG
cmake_make_makeinstall

### install CppUnit ############################################################

# required by librevenge

get_source $URL_CPPUNIT
configure_make_makeinstall

### install librevenge #########################################################

# required by libcdr

get_source $URL_LIBREVENGE
configure_make_makeinstall

### install libcdr #############################################################

get_source $URL_LIBCDR
jhbuild run ./autogen.sh
configure_make_makeinstall

### install ImageMagick 6 ######################################################

get_source $URL_IMAGEMAGICK

# Recently a new issue manifested out of thin air: linking to pango fails with
# missing symbols like '_g_object_unref'. Pango requires '-lgobject-2.0',
# but this does not appear in the linker flags (check invocation with
# 'make V=1'). Easiest way for a quick & dirty fix is to configure with
# PANGO_LIBS set accordingly, but the configure scripts deletes the setting
# although claiming to respect it. Therefore we patch the configure
# script first.

sed -i "" 's/^  pkg_cv_PANGO_LIBS=`$PKG_CONFIG --libs "pangocairo.*/  pkg_cv_PANGO_LIBS="$($PKG_CONFIG --libs pangocairo) -lgobject-2.0"/' configure

configure_make_makeinstall

### install Poppler ############################################################

get_source $URL_POPPLER
cmake_make_makeinstall -DENABLE_UNSTABLE_API_ABI_HEADERS=ON

### install double-conversion ##################################################

# Required by lib2geom.

get_source $URL_DOUBLE_CONVERSION
cmake_make_makeinstall

### install Potrace ############################################################

get_source $URL_POTRACE
configure_make_makeinstall --with-libpotrace

### install OpenMP #############################################################

get_source $URL_OPENMP
cmake_make_makeinstall

### install Ghostscript ########################################################

get_source $URL_GHOSTSCRIPT
configure_make_makeinstall
