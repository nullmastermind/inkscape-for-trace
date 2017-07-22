Basic Installation
==================

Inkscape version 0.92 and newer can be build using cmake:

```bash
mkdir build
cd build
cmake ..
make
make install
```

Other platforms such as Windows and Mac require a lot more and are considered
a developer and packager task. These instructions are kept on the Inkscape wiki.

Running Without Installing
==========================

For developers and others who want to run inkscape without installing it:

```bash
ln -s . share/inkscape
mkdir -p build/conf
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=$PWD/../ ..
make -j4
export INKSCAPE_PROFILE_DIR=$PWD/conf
PATH=$PWD/bin/:$PATH
./bin/inkscape
```

This won't work for other platforms such as Windows and Mac, see above. But
what it is doing is linking the share directory into a location where
the inkscape binary will be able to find it. Allowing you to change the
inkscape shared files without rebuilding or installing.

Then setting a local configuration directory, keeping your configurations
separate from any installed version.


Required Dependencies
=====================

The Inkscape core depends on several other libraries that you will need
install, if they are not already present on your system.  The most
typical libraries you may need to install are:

   * [Boehm-GC](http://www.hboehm.info/gc/)
   * [libsigc++](http://libsigc.sourceforge.net/)
   * [gtkmm](https://www.gtkmm.org/)

Please see [the wiki page on compiling Inkscape](http://wiki.inkscape.org/wiki/index.php/CompilingInkscape) for the
most current dependencies, including links to the source tarballs.


Extension Dependencies
======================
Inkscape also has a number of extensions for implementing various
features such as support for non-SVG file formats.  In theory, all
extensions are optional, however in practice you will want to have these
installed and working.  Unfortunately, there is a great deal of
variability in how you can get these functioning properly.  Here are
some recommendations:

First, make sure you have Python.  If you are on Windows you
should also install [Cygwin](https://www.cygwin.com/).

Second, if an extension does not work, check the file
`extensions-errors.log` located on Linux at `~/.config/inkscape` and on
Windows at `%userprofile%\Application Data\Inkscape\`. Any missing
programs will be listed.

