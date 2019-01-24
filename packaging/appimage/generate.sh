#!/bin/bash

########################################################################
# Install build-time and run-time dependencies
########################################################################

export DEBIAN_FRONTEND=noninteractive

# Backported pango (from 18.10) for variable font support
add-apt-repository -y ppa:bryce/pango1.0
apt-get update -yqq
apt-get install -y libpango1.0-0 libpango1.0-dev

########################################################################
# Build Inkscape and install to appdir/
########################################################################

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_BINRELOC=ON \
-DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_C_COMPILER_LAUNCHER=ccache \
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache
make -j$(nproc)
make DESTDIR=appdir -j$(nproc) install ; find appdir/
cp ../packaging/appimage/AppRun appdir/AppRun ; chmod +x appdir/AppRun
( cd appdir/usr/lib/ ; ln -s ../* . ) # FIXME: Why is this needed?
( cd appdir/ ; ln -s usr/* . ) # FIXME: Why is this needed?
cp ./appdir/usr/share/icons/application/256x256/org.inkscape.Inkscape.png ./appdir/
sed -i -e 's|^Icon=.*|Icon=org.inkscape.Inkscape|g' ./appdir/usr/share/applications/org.inkscape.Inkscape.desktop # FIXME
cd appdir/

########################################################################
# Bundle everyhting
# to allow the AppImage to run on older systems as well
########################################################################

# Bundle all of glibc; this should eventually be done by linuxdeployqt
apt-get download libc6
find *.deb -exec dpkg-deb -x {} . \;
rm *deb

# Make absolutely sure it will not load stuff from /lib or /usr
sed -i -e 's|/usr|/xxx|g' lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
sed -i -e 's|/usr/lib|/ooo/ooo|g' lib/x86_64-linux-gnu/ld-linux-x86-64.so.2

# Bundle Gdk pixbuf loaders without which the bundled Gtk does not work;
# this should eventually be done by linuxdeployqt
apt-get download librsvg2-common libgdk-pixbuf2.0-0
find *.deb -exec dpkg-deb -x {} . \;
rm *deb
cp /usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/* usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/
cp /usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders.cache usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/
sed -i -e 's|/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/2.10.0/loaders/||g' usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders.cache

# Optionally bundle Gtk+ themes
apt-get download gnome-themes-extra gnome-themes-extra-data adwaita-icon-theme-full
find *.deb -exec dpkg-deb -x {} . \;
rm *deb

# Bundle fontconfig settings
mkdir -p etc/fonts/
cp /etc/fonts/fonts.conf etc/fonts/

# Bundle Python
apt-get download libpython2.7-stdlib python2.7 python2.7-minimal libpython2.7-minimal python-lxml
find *.deb -exec dpkg-deb -x {} . \;
rm *deb
cd -

########################################################################
# Generate AppImage
########################################################################

wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

./linuxdeployqt-continuous-x86_64.AppImage --appimage-extract-and-run appdir/usr/share/applications/org.inkscape.Inkscape.desktop \
  -appimage -unsupported-bundle-everything -executable=appdir/usr/bin/inkview \
  -executable=appdir/usr/lib/inkscape/libinkscape_base.so \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-xpm.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-xbm.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-tiff.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-tga.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-svg.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-qtif.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-pnm.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-png.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-jpeg.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-ico.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-icns.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-gif.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-bmp.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/libpixbufloader-ani.so) \
  -executable=$(readlink -f appdir/usr/lib/x86_64-linux-gnu/gtk-*/*/engines/libadwaita.so) \
  -executable=appdir/usr/bin/python2.7

mv Inkscape*.AppImage* ../
