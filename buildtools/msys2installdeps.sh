#!/usr/bin/env bash
# -------------------------------------------------------------------------------
# This script installs all dependencies required for building Inkscape with MSYS2
#   execute it once on an MSYS shell, i.e.
#    - use the "MSYS2 MSYS" shortcut in the start menu or
#    - run "msys2.exe" in MSYS2's installation folder
#
# MSYS2 and installed libraries can be updated later by executing
#   pacman -Syu --ignore=mingw-w64-*-imagemagick
# in an MSYS shell
# -------------------------------------------------------------------------------

# select if you want to build 32-bit (i686), 64-bit (x86_64), or both
case "$MSYSTEM" in
  MINGW32)
    ARCH=mingw-w64-i686
    ;;
  MINGW64)
    ARCH=mingw-w64-x86_64
    ;;
  *)
    ARCH={mingw-w64-i686,mingw-w64-x86_64}
    ;;
esac

# set default options for invoking pacman (in CI this variable is already set globally)
if [ -z $CI ]; then
    PACMAN_OPTIONS="--needed --noconfirm"
fi

# sync package databases
pacman -Sy

# install basic development system, compiler toolchain and build tools
eval pacman -S $PACMAN_OPTIONS \
git \
intltool \
base-devel \
$ARCH-toolchain \
$ARCH-cmake \
$ARCH-ninja

# install Inkscape dependencies (required)
eval pacman -S $PACMAN_OPTIONS \
$ARCH-double-conversion \
$ARCH-gc \
$ARCH-gsl \
$ARCH-libxslt \
$ARCH-boost \
$ARCH-gtk3 \
$ARCH-gtkmm3 \
$ARCH-libsoup

# install Inkscape dependencies (optional)
eval pacman -S $PACMAN_OPTIONS \
$ARCH-poppler \
$ARCH-potrace \
$ARCH-libcdr \
$ARCH-libvisio \
$ARCH-libwpg \
$ARCH-aspell \
$ARCH-aspell-en \
$ARCH-gspell

# install ImageMagick (as Inkscape requires old version ImageMagick 6 we have to specify it explicitly)
# to prevent future updates:
#     add the line
#        "IgnorePkg = mingw-w64-*-imagemagick"
#     to
#        "C:\msys64\etc\pacman.conf"
#     or (always!) run pacman with the additional command line switch
#        --ignore=mingw-w64-*-imagemagick
for arch in $(eval echo $ARCH); do
  wget -nv https://gitlab.com/ede123/bintray/-/raw/master/${arch}-imagemagick-6.9.10.69-1-any.pkg.tar.xz \
    && pacman -U $PACMAN_OPTIONS ${arch}-imagemagick-6.9.10.69-1-any.pkg.tar.xz \
    && rm  ${arch}-imagemagick-6.9.10.69-1-any.pkg.tar.xz
done


# install Python and modules used by Inkscape
eval pacman -S $PACMAN_OPTIONS \
$ARCH-python \
$ARCH-python-pip \
$ARCH-python-lxml \
$ARCH-python-numpy \
$ARCH-python-pillow \
$ARCH-python-six \
$ARCH-python-gobject \
$ARCH-python-pyserial \
$ARCH-python-coverage \
$ARCH-scour

# install Python modules not provided as MSYS2/MinGW packages
PACKAGES=""
for arch in $(eval echo $ARCH); do
  case ${arch} in
    mingw-w64-i686)
      #/mingw32/bin/pip3 install --upgrade ${PACKAGES}
      ;;
    mingw-w64-x86_64)
      #/mingw64/bin/pip3 install --upgrade ${PACKAGES}
      ;;
  esac
done
