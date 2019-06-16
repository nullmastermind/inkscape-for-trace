Snap
====

This directory and `../snappy/` are used for building the snap (https://snapcraft.io/) package of Inkscape.

Each commit to master sends a new build to the "edge" version.

For build status and logs, see https://launchpad.net/~ted/+snap/inkscape-master. That account on launchpad.net is owned by Ted Gould <ted@gould.cx>.

If the snap does no longer build or run, the most probable reason is that we added a new dependency. Have a look at the recent changes in https://gitlab.com/inkscape/inkscape-ci-docker, and try to make a similar change to `build-packages` (build dependency) or `stage-packages` (runtime dependency) in `snapcraft.yaml`.
