Inkscape. Draw Freely.
======================

[https://www.inkscape.org/](https://www.inkscape.org/)

Inkscape is an open source drawing tool with capabilities similar to
Illustrator, Freehand, and CorelDraw that uses the W3C standard scalable
vector graphics  format (SVG). Some supported SVG features include
basic shapes, paths, text, markers, clones, alpha blending, transforms,
gradients, and grouping. In addition, Inkscape supports Creative Commons
meta-data, node-editing, layers, complex path operations, text-on-path,
and SVG XML editing. It also imports several formats like EPS, Postscript,
JPEG, PNG, BMP, and TIFF and exports PNG as well as multiple vector-based
formats.

Inkscape's main motivations are to provide the Open Source community
with a fully W3C compliant XML, SVG, and CSS2 drawing tool emphasizing a
lightweight core with powerful features added as extensions, and the
establishment of a friendly, open, community-oriented development
processes.

[![build status](https://gitlab.com/inkscape/inkscape/badges/master/build.svg)](https://gitlab.com/inkscape/inkscape/commits/master)

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
features such as support for non-SVG file formats, etc.  In theory, all
extensions are optional, however in practice you will want to have these
installed and working.  Unfortunately, there is a great deal of
variability in how you can get these functioning properly.  Here are
some recommendations:

First, make sure you have Perl and Python.  If you are on Windows you
should also install [Cygwin](https://www.cygwin.com/).

Next, you will need to ensure the dependencies for each extension is
present.  Depending on which extensions you need, the dependencies are
going to vary, but here are some you may need to install:

   * [XML::Parser](http://search.cpan.org/dist/XML-Parser/)
   * [XML::XQL](http://search.cpan.org/dist/XML-XQL/)

If you install dependencies to non-standard locations, such as
installing XML::Parser someplace in your home directory, you may need to
take some extra steps to indicate where those dependencies are to be
found.  For instance, with Perl modules, set the PERLLIB or PERL5LIB
variable (see `man perlrun`)

