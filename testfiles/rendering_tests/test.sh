#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

if [ "$#" -lt 2 ]; then
    echo "pass the path of the inkscape executable as parameter then the name of the test" $#
    exit 1
fi

command -v compare >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'compare' but it's not installed.  Aborting."; exit 1; }

INKSCAPE_EXE=$1
exit_status=0
test=$2
EXPECTED=$(dirname $test)"/expected_rendering/"$(basename $test)
testname=$(basename $test)


    ${INKSCAPE_EXE} --export-file=${testname}.png -d 96 ${test}.svg #2>/dev/null >/dev/null
    compare -metric AE ${testname}.png ${EXPECTED}.png ${testname}-compare.png 2> .tmp
    test1=`cat .tmp`
    echo $test1
    if [ "$test1" == 0 ]; then
        echo ${testname} "PASSED"
        rm ${testname}.png ${testname}-compare.png
    else
        echo ${testname} "FAILED"
        exit_status=1
    fi

if [ -f "${EXPECTED}-large.png" ]; then
    ${INKSCAPE_EXE} --export-file=${testname}-large.png -d 384 ${test}.svg #2>/dev/null >/dev/null
    compare -metric AE ${testname}-large.png ${EXPECTED}-large.png ${testname}-compare-large.png 2>.tmp
    test2=`cat .tmp`
    if [ "$test2" = 0 ]; then
        echo ${testname}-large "PASSED"
        rm ${testname}-large.png ${testname}-compare-large.png
    else
        echo ${testname}-large "FAILED"
        exit_status=1
    fi
else
    echo ${testname}-large "SKIPPED"
fi

rm .tmp
exit $exit_status
