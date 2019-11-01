# SPDX-License-Identifier: GPL-2.0-or-later

#add your test here (do not put the .svg extension)
set(RENDERING_TESTS 
    test-baseline-shift
    test-empty
    # test-glyph-y-pos 
    ## to be fixed since an update happened between harfbuzz 1.5.1(OK) and 1.6.0(FAIL).
    ## If you re-enable the test, you may have to *slightly* fix the expected rendering (hoping the fix happens upstream). 
    ## Please also check that the rendering with harfbuzz <=1.5.1 is not *too* wrong (forolder systems)
    ## cf Tav's post : https://www.patreon.com/posts/into-sinkhole-19021727
    ## and bug https://bugzilla.gnome.org/show_bug.cgi?id=787526
    test-rtl-vertical
    selector-important-002
    selector-important-003
    test-dont-crash
    test-lpe-0.92
    test-lpe-1.0
)


foreach(rendering_test ${RENDERING_TESTS})
    add_test(NAME ${rendering_test} COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inkscape ${CMAKE_CURRENT_SOURCE_DIR}/${rendering_test}  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/testfiles/rendering_tests )
    set_tests_properties(${rendering_test} PROPERTIES ENVIRONMENT "${CMAKE_CTEST_ENV}")
endforeach()

