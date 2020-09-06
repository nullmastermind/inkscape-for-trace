#!/bin/bash

########################################################################
# Install build-time and run-time dependencies
########################################################################

export DEBIAN_FRONTEND=noninteractive

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
cp ./appdir/usr/share/icons/hicolor/256x256/apps/org.inkscape.Inkscape.png ./appdir/
sed -i -e 's|^Icon=.*|Icon=org.inkscape.Inkscape|g' ./appdir/usr/share/applications/org.inkscape.Inkscape.desktop # FIXME
cd appdir/

########################################################################
# Bundle everyhting
# to allow the AppImage to run on older systems as well
########################################################################

apt_bundle() {
    apt-get download "$@"
    find *.deb -exec dpkg-deb -x {} . \;
    find *.deb -delete
}

# Bundle all of glibc; this should eventually be done by linuxdeployqt
apt update
apt_bundle libc6

# Make absolutely sure it will not load stuff from /lib or /usr
sed -i -e 's|/usr|/xxx|g' lib/x86_64-linux-gnu/ld-linux-x86-64.so.2

# Bundle Gdk pixbuf loaders without which the bundled Gtk does not work;
# this should eventually be done by linuxdeployqt
apt_bundle librsvg2-common libgdk-pixbuf2.0-0
cp /usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/* usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders/
cp /usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders.cache usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/
sed -i -e 's|/usr/lib/x86_64-linux-gnu/gdk-pixbuf-.*/.*/loaders/||g' usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders.cache

# Bundle fontconfig settings
mkdir -p etc/fonts/
cp /etc/fonts/fonts.conf etc/fonts/

# Bundle Python
PY_VER=3.8
apt_bundle \
    libpython${PY_VER}-stdlib \
    libpython${PY_VER}-minimal \
    python${PY_VER} \
    python${PY_VER}-minimal \
    python3-lxml \
    python3-numpy \
    python3-scour \
    python3-distutils \
    python3-gi \
    gir1.2-glib-2.0 \
    gir1.2-gtk-3.0 \
    gir1.2-gdkpixbuf-2.0 \
    gir1.2-pango-1.0
(
    cd usr/bin
    ln -s python${PY_VER} python3
    ln -s python${PY_VER} python
)

cd -

########################################################################
# Generate AppImage
########################################################################

wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

linuxdeployqtargs=()

for so in $(find \
    appdir/usr/lib/x86_64-linux-gnu/gdk-pixbuf-*/*/loaders \
    appdir/usr/lib/python3 \
    appdir/usr/lib/python${PY_VER} \
    -name \*.so); do
    linuxdeployqtargs+=("-executable=$(readlink -f "$so")")
done

./linuxdeployqt-continuous-x86_64.AppImage --appimage-extract-and-run appdir/usr/share/applications/org.inkscape.Inkscape.desktop \
  -appimage -unsupported-bundle-everything -executable=appdir/usr/bin/inkview \
  -executable=appdir/usr/lib/inkscape/libinkscape_base.so \
  -executable=appdir/usr/bin/python${PY_VER} \
  "${linuxdeployqtargs[@]}"

mv Inkscape*.AppImage* ../
