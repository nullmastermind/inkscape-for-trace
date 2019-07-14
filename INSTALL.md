Installation
============

Basic Installation
------------------

Inkscape version 0.92 and newer can be build using CMake:

```sh
mkdir build
cd build
cmake ..
make
make install
```

Other platforms such as Windows and Mac require a lot more and are considered
a developer and packager task. These instructions are kept on the Inkscape wiki.

Running Without Installing
--------------------------

For developers and others who want to run Inkscape without installing it please
see the ***Building*** section in the `CONTRIBUTING.md` file.

Required Dependencies
---------------------

The Inkscape core depends on several other libraries that you will need
install, if they are not already present on your system. The most
typical libraries you may need to install are:

* [Boehm-GC](http://www.hboehm.info/gc/)
* [libsigc++](https://github.com/libsigcplusplus/libsigcplusplus)
* [gtkmm](https://www.gtkmm.org/)

Please see [the wiki page on compiling Inkscape](http://wiki.inkscape.org/wiki/index.php/CompilingInkscape) for the
most current dependencies, including links to the source tarballs.


Extensions
----------

All Inkscape extensions have been moved into their own repository. They
can be installed from there and should be packaged into builds directly.
Report all bugs and ideas to that sub project.

[Inkscape Extensions](https://gitlab.com/inkscape/extensions/)

They are available as a sub-module which can be updated independently:

```sh
git submodule update --remote
```

This will update the module to the latest version and you will see the
extensions directory is now changes in the git status. So be mindful of that.

Build Options
-------------

A number of configuration settings can be overridden through CMake. To
see a list of the options available for Inkscape, run:

```sh
cmake -L
```
or, for more advanced cmake settings:

```sh
cmake --help
```

For example, to build Inkscape with only SVG 1 support, and no SVG 2, do:

```sh
cmake .. -DWITH_SVG2=OFF
```

Or, to build Inkscape with debugging symbols, do:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug ..
```
