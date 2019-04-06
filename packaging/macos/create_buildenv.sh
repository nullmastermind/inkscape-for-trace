#!/usr/bin/env bash
# create_buildenv.sh
# https://github.com/dehesselle/mibap
#
# create jhbuild environment for Inkscape

for script in 1??-*.sh; do
  ./$script
done

