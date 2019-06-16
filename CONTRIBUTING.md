Contributing to Inkscape
========================

Inkscape welcomes your contributions to help turn it into a fully
SVG-compliant drawing program for the Open Source community.

While many developers work on fixing bugs and creating new features, it
is worth strong emphasis that even non-programmers can help make
Inkscape more powerful and successful. You probably already have an idea
of something you'd like to work on. If not, here are just a few ways you
can help:

   * Pick a bug, fix it, and send in a merge request on GitLab.
   * Choose a feature you want to see developed, and make it.
   * If you speak a language in addition to English, work on your
     language's i18n file in the po/ directory.
   * Find a new bug and report it.
   * Help answer questions for new Inkscapers on IRC, forum, or the
     mailing lists.
   * Write an article advocating Inkscape.
   * Author a HOWTO describing a trick or technique you've figured out.


GIT Access
----------

Inkscape is currently developed on git, with the code hosted on GitLab.

 * https://gitlab.com/inkscape/inkscape

We give write access out to people with proven interest in helping develop
the codebase. Proving your interest is straightforward:  Make two
contributions and request access.

Compiling the development version
---------------------------------

See http://wiki.inkscape.org/wiki/index.php/CompilingInkscape for general
remarks about compiling, including how to find some of the needed packages for
your distribution, and suggestions for developers.


Patch Decisions
---------------

Our motto for changes to the codebase is "Patch first, ask questions
later". When someone has an idea, rather than endlessly debating it, we
encourage folks to go ahead and code something up (even prototypish).
This is then incorporated into the development branch of the code for
folks to try out, poke and prod, and tinker with. We figure, the best
way to see if an idea fits is to try it on for size.


Coding Style
------------

Please refer to the Coding Style Guidelines
(https://inkscape.org/en/develop/coding-style/) if you have specific questions
on the style to use for code. If reading style guidelines doesn't interest
you, just follow the general style of the surrounding code, so that it is at
least consistent.


Documentation
-------------

Code needs to be documented. Future Inkscape developers will really
appreciate this. New files should have one or two lines describing the
purpose of the code inside the file.


Building
--------

This is the best set of instructions for setting up your build directory...

You should install ninja and ccache for the fastest build:

```bash
sudo apt-get install ninja-build ccache
```

Next we prepare a build directory with a symlink to Inkscape's share folder, add a profile dir and set the bin folder (optional):

```bash
ln -s share share/inkscape
mkdir -p build/conf
cd build
export INKSCAPE_PROFILE_DIR=$PWD/conf
PATH=$PWD/bin/:$PATH
```

Now we invoke cmake, letting it know to use our new build directory prefix, ccache and the Ninja compiler:

```bash
cmake -DCMAKE_INSTALL_PREFIX:PATH=$PWD/../ -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
```

Invoke ninja to build the code. You may also use plain gcc's `make` if you didn't specific `-G` in the command above:
```bash
ninja
```

Now we can run `inkscape` that we have built, with the latest resources and code:

```bash
./bin/inkscape
```

Testing
-------

Before landing a patch, the unit tests should pass.

See: http://wiki.inkscape.org/wiki/index.php/CMake#Using_CMake_to_run_tests
