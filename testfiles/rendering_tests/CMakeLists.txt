
#add your test here (do not put the .svg extension)
set(RENDERING_TESTS 
    test-baseline-shift
    test-empty
    test-glyph-y-pos
    test-rtl-vertical
    selector-important-002
    selector-important-003
)


foreach(rendering_test ${RENDERING_TESTS})
    add_test(NAME ${rendering_test} COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inkscape ${CMAKE_CURRENT_SOURCE_DIR}/${rendering_test}  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/testfiles/rendering_tests )
    set_tests_properties(${rendering_test} PROPERTIES ENVIRONMENT "${CMAKE_CTEST_ENV}")
endforeach()

