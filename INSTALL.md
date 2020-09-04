Installation
============

Required Dependencies
---------------------

The Inkscape core depends on several other libraries that you will need
install, if they are not already present on your system. The most
typical libraries you may need to install are: 
[Boehm-GC](http://www.hboehm.info/gc/), 
[libsigc++](https://github.com/libsigcplusplus/libsigcplusplus), 
[gtkmm](https://www.gtkmm.org/).

Please see [the wiki page on compiling Inkscape](http://wiki.inkscape.org/wiki/index.php/CompilingInkscape) for the
most current dependencies, including links to the source tarballs. 
For common linux-distributions (Ubuntu, Debian, Fedora) you can use 
[a bash-script](https://gitlab.com/inkscape/inkscape-ci-docker/-/raw/master/install_dependencies.sh?inline=false) 
for getting required libaries.

Basic Installation
------------------

For Linux based Free Desktops, Inkscape version 0.92 and newer can be built using CMake:

```sh
mkdir build
cd build
cmake ..
make
make install
```

See `CONTRIBUTING.md` for more developer details and the [wiki](https://wiki.inkscape.org/wiki/index.php?title=Compiling_Inkscape).

For non-linux platforms, please see the Inkscape wiki pages here:

For building on ChromeOS, please click [here](
https://wiki.inkscape.org/wiki/index.php?title=Compiling_Inkscape_on_Chrome_OS)

For building on Windows, please click [here](
https://wiki.inkscape.org/wiki/index.php?title=Compiling_Inkscape_on_Windows_with_MSYS2)

For building on Mac, please click [here](
https://wiki.inkscape.org/wiki/index.php?title=CompilingMacOsX)


Running Without Installing
--------------------------

For developers and others who want to run Inkscape without installing it please
see the ***Building*** section in the `CONTRIBUTING.md` file.

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
