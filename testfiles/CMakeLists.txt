# -----------------------------------------------------------------------------
#
# -----------------------------------------------------------------------------
if(NOT GTEST_FOUND) # not required for gtest 1.8.0 and later
    add_subdirectory(${GMOCK_DIR} ${CMAKE_BINARY_DIR}/gmock)
    set(GTEST_INCLUDE_DIR ${GMOCK_DIR}/../googletest/include)

    include_directories(SYSTEM ${GMOCK_DIR}/gtest/include
                               ${GMOCK_DIR}/include
			       ${GTEST_INCLUDE_DIR}
                               ${CMAKE_SOURCE_DIR}/gtest/gtest/include)
endif()
include_directories("${CMAKE_SOURCE_DIR}/src/3rdparty/adaptagrams")

set(TEST_SOURCES
	attributes-test
	color-profile-test
	dir-util-test
	sp-object-test
	object-set-test
	style-test)

set(TEST_LIBS
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
	gmock_main
	inkscape_base)


add_custom_target(tests)
foreach(source ${TEST_SOURCES})
	add_executable(${source} src/${source}.cpp unittest.cpp doc-per-case-test.cpp)
	target_link_libraries(${source} ${TEST_LIBS})
	add_test(NAME ${source} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source})
    set_tests_properties(${source} PROPERTIES ENVIRONMENT "${CMAKE_CTEST_ENV}")
	add_dependencies(tests ${source})
endforeach()
add_subdirectory(rendering_tests)


if(WITH_FUZZ)
    # to use the fuzzer, make sure you use the right compiler (clang)
    # with the right flags -fsanitize=address -fsanitize-coverage=edge,trace-pc-guard,indirect-calls,trace-cmp,trace-div,trace-gep -fno-omit-frame-pointer
    # (see libfuzzer doc for info in flags)
    # first line is for integration into oss-fuzz https://github.com/google/oss-fuzz
    add_executable(fuzz fuzzer.cpp)
    if(LIB_FUZZING_ENGINE)
        target_link_libraries(fuzz inkscape_base -lFuzzingEngine)
    else()
        target_link_libraries(fuzz inkscape_base -lFuzzer)
    endif()
endif()
