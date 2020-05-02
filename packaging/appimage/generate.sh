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
( cd appdir/usr/lib/ ; ln -s ../* . ) # FIXME: Why is this needed?
( cd appdir/ ; ln -s usr/* . ) # FIXME: Why is this needed?
cp ./appdir/usr/share/icons/hicolor/256x256/apps/org.inkscape.Inkscape.png ./appdir/
sed -i -e 's|^Icon=.*|Icon=org.inkscape.Inkscape|g' ./appdir/usr/share/applications/org.inkscape.Inkscape.desktop # FIXME
cd appdir/

########################################################################
# Bundle everyhting
# to allow the AppImage to run on older systems as well
########################################################################

# Bundle all of glibc; this should eventually be done by linuxdeployqt
apt update
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

# Bundle fontconfig settings
mkdir -p etc/fonts/
cp /etc/fonts/fonts.conf etc/fonts/

# Bundle Python
apt-get download libpython3.8-stdlib python3.8 python3.8-minimal libpython3.8-minimal python3-lxml python3-numpy python3-scour python3-gi
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
  -executable=appdir/usr/bin/python3.8 \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/lxml/_elementpath.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/lxml/builder.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/lxml/etree.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/lxml/html/clean.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/lxml/html/diff.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/lxml/objectify.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/_dummy.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/multiarray.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/multiarray_tests.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/operand_flag_tests.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/struct_ufunc_test.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/test_rational.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/umath.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/core/umath_tests.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/fft/fftpack_lite.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/linalg/_umath_linalg.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/linalg/lapack_lite.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3/dist-packages/numpy/random/mtrand.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_bsddb.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_codecs_cn.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_codecs_hk.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_codecs_iso2022.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_codecs_jp.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_codecs_kr.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_codecs_tw.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_csv.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_ctypes.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_ctypes_test.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_curses.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_curses_panel.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_elementtree.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_hashlib.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_hotshot.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_json.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_lsprof.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_multibytecodec.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_multiprocessing.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_sqlite3.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_ssl.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/_testcapi.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/audioop.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/bz2.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/crypt.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/dbm.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/future_builtins.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/linuxaudiodev.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/mmap.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/nis.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/ossaudiodev.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/parser.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/pyexpat.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/readline.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/resource.*.so) \
  -executable=$(readlink -f appdir/usr/lib/python3.8/lib-dynload/termios.*.so)

mv Inkscape*.AppImage* ../
