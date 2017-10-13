# -----------------------------------------------------------------------------
#
# -----------------------------------------------------------------------------
if(NOT GTEST_FOUND) # not required for gtest 1.8.0 and later
    add_subdirectory(${GMOCK_DIR} ${CMAKE_BINARY_DIR}/gmock)

    include_directories(SYSTEM ${GMOCK_DIR}/gtest/include
                               ${GMOCK_DIR}/include
                               ${CMAKE_SOURCE_DIR}/gtest/gtest/include)
endif()

set(TEST_SOURCES
	attributes-test
	color-profile-test
	dir-util-test
	sp-object-test
	object-set-test
	style-test)

set(TEST_LIBS
	gmock_main
	inkscape_base)


add_custom_target(tests)
foreach(source ${TEST_SOURCES})
	add_executable(${source} src/${source}.cpp unittest.cpp doc-per-case-test.cpp)
	target_link_libraries(${source} ${TEST_LIBS})
	add_test(NAME ${source} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source})
    set_tests_properties(${source} PROPERTIES ENVIRONMENT ${CMAKE_CTEST_ENV})
	add_dependencies(tests ${source})
endforeach()
add_subdirectory(rendering_tests)
