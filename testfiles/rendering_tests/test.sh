#!/bin/bash

if [ "$#" -lt 2 ]; then
    echo "pass the path of the inkscape executable as parameter then the name of the test" $#
    exit 1
fi

command -v compare >/dev/null 2>&1 || { echo >&2 "I require ImageMagick's 'compare' but it's not installed.  Aborting."; exit 1; }

INKSCAPE_EXE=$1
exit_status=0
test=$2
EXPECTED=$(dirname $test)"/expected_rendering/"$(basename $test)


    ${INKSCAPE_EXE} -z ${test}.svg -d 96  -e ${test}.png #2>/dev/null >/dev/null
    compare -metric AE ${test}.png ${EXPECTED}.png ${test}-compare.png 2> .tmp
    test1=`cat .tmp`
    echo $test1
    if [ $test1 == 0 ]; then
        echo ${test} "PASSED"
        rm ${test}.png ${test}-compare.png
    else
        echo ${test} "FAILED"
        exit_status=1
    fi
    ${INKSCAPE_EXE} -z ${test}.svg -d 384 -e ${test}-large.png #2>/dev/null >/dev/null
    compare -metric AE ${test}-large.png ${EXPECTED}-large.png ${test}-compare-large.png 2>.tmp
    test2=`cat .tmp`
    if [ $test2 == 0 ]; then
        echo ${test}-large "PASSED"
        rm ${test}-large.png ${test}-compare-large.png
    else
        echo ${test}-large "FAILED"
        exit_status=1
    fi

rm .tmp
exit $exit_status
