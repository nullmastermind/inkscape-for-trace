
#add your test here (do not put the .svg extension)
set(RENDERING_TESTS 
    test-empty
)


foreach(rendering_test ${RENDERING_TESTS})
    add_test(NAME ${rendering_test} COMMAND test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/inkscape ${rendering_test} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/testfiles/rendering_tests )
endforeach()

