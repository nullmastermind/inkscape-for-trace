#!/bin/bash

### HOWTO ###
# Add a rendering test: 
#  - create the svg file
#  - inkscape <yourfile>.svg -d 96  -e expected_rendering/<yourfile>.png
#  - inkscape <yourfile>.svg -d 384 -e expected_rendering/<yourfile>-large.png
#  - add the test in the list below
#  - use stable if possible to generate the reference png files
#  - git add <yourfile>.svg expected_rendering/<yourfile>-large.png expected_rendering/<yourfile>.png
#  
# Fix a failing test (due to a change in code):
#  - DO *NOT* MODIFY the expected rendering (or the svg) before getting advice from inkscape-devel@
#  - fix your code if possible
#  - IF you change introduces a greater compatibility with css or browsers
#  - AND you cannot reasonably "update" files from older versions to match the appearance
#  - AND inkscape-devel@ has a consensus that it's the only way
#  -> do as you must
#  - manually double check the changes
# Fix a failing test (due to a change in pixman or cairo):
#  - update renderings. Use a *stable* version to generate the renderings, NOT TRUNK
#  - manually check appearances
#############

### test list ###

tests="\
    test-empty\
    "

### script ###



if [ "$#" -lt 1 ]; then
    echo "pass the path of the inkscape executable as parameter" $#
    exit 1
fi
INKSCAPE_EXE=$1
exit_status=0
for test in $tests
do
    ${INKSCAPE_EXE} -z ${test}.svg -d 96  -e ${test}.png #2>/dev/null >/dev/null
    compare -metric AE ${test}.png expected_rendering/${test}.png ${test}-compare.png 2> .tmp 
    test1=`cat .tmp`
    echo $test1
    if [ $test1 == 0 ]; then
        echo ${test} "PASSED"
        rm ${test}.png ${test}-compare.png
    else
        echo ${test} "FAILED"
        exist_status=1
    fi
    ${INKSCAPE_EXE} -z ${test}.svg -d 384 -e ${test}-large.png #2>/dev/null >/dev/null
    compare -metric AE ${test}-large.png expected_rendering/${test}-large.png ${test}-compare-large.png 2>.tmp 
    test2=`cat .tmp`
    if [ $test2 == 0 ]; then
        echo ${test}-large "PASSED"
        rm ${test}-large.png ${test}-compare-large.png
    else
        echo ${test}-large "FAILED"
    fi
done
rm .tmp
exit $exit_status
