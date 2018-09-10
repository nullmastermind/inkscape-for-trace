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


Extensions
==========

All inkscape extensions have been moved into their own reporsitory, they
can be installed from there and should be packaged into builds directly.
Report all bugs and ideas to that sub project.

[Inkscape Extensions](https://gitlab.com/inkscape/extensions/)

Build Options
=============

A number of configuration settings can be overridden through cmake.  To
see a list of the options available for inkscape, run:

 $ cmake -L

or, for more advanced cmake settings:

 $ cmake --help

For example, to build inkscape with only SVG 1 support, and no SVG 2, do:

 $ cmake . -DWITH_SVG2=OFF
