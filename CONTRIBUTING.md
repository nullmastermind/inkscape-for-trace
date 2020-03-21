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
ln -s $PWD/share ./share/inkscape
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

Profiling
---------

To run with profiling, add `-DWITH_PROFILING=ON` to the above cmake script. Compiling and running will be slower and the gmon file will apear only after Inkscape quits.

See: https://wiki.inkscape.org/wiki/index.php/Profiling

Testing
-------

Before landing a patch, the unit tests should pass.

See: http://wiki.inkscape.org/wiki/index.php/CMake#Using_CMake_to_run_tests

General Guidelines for developers
----------------------------------

* If you are new, fork the inkscape project (https://gitlab.com/inkscape/inkscape) and create a new branch for each bug/feature you want to work on. Try to Set the CI time to a high value like 2 hour (Go to your fork > Settings > General Pipelines > Timeout)
* Merge requests (MR) are encouraged for the smallest of contributions. This helps other developers review the code you've written and check for the mistakes that may have slipped by you.
* Before working on anything big, be sure to discuss your idea with us ([IRC](irc://irc.freenode.org/#inkscape) or [RocketChat](https://chat.inkscape.org/)). Someone else might already have plans you can build upon and we will try to guide you !
* Adopt the coding style (indentation, bracket placement, reference/pointer placement, variable naming etc. - developer's common sense required!) of existing source so that your changes and code doesn't stand out/feel foreign.
* Carefully explain your ideas and the changes you've made along with their importance in the MR. Feel free to use pictures !
* Check the "Allow commits from members who can merge to this target branch" option while submitting the MR.
* Write informative commit messages ([check this](https://chris.beams.io/posts/git-commit/)). Use full url of bug instead of mentioning just the number in messages and discussions.
* Try to keep your MR current instead of creating a new one. Rebase your MR sometimes.
* Inkscape has contributors/developers from across the globe. Some may be unavailable at times but be patient and we will try our best to help you. We are glad to have you!
