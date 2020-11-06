# SPDX-License-Identifier: GPL-2.0-or-later
# -----------------------------------------------------------------------------

# custom "check" target with proper dependencies (builds inkscape and tests)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                        DEPENDS tests
                        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(check inkscape)


# create symlink "inkscape_datadir" to use as INKSCAPE_DATADIR
# - ensures tests can be run without installing the project
# - also helpful for running Inkscape uninstalled: 'INKSVAPE_DATADIR=inkscape_datadir bin/inkscape'
set(INKSCAPE_DATADIR ${CMAKE_BINARY_DIR}/inkscape_datadir)
if(NOT EXISTS ${INKSCAPE_DATADIR}/inkscape)
    set(link_source ${INKSCAPE_DATADIR}/inkscape)
    set(link_target ${CMAKE_SOURCE_DIR}/share)
    message(STATUS "Creating link '${link_source}' --> '${link_target}'")
    execute_process(COMMAND mkdir inkscape_datadir)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${link_target} ${link_source}
                    RESULT_VARIABLE result)
    if(result)
        message(WARNING "Creation of link failed: ${result}")
    endif()
endif()
# check if creation succeeded
if(EXISTS ${INKSCAPE_DATADIR}/inkscape)
    set(CMAKE_CTEST_ENV INKSCAPE_DATADIR=${INKSCAPE_DATADIR})
else()
    message(WARNING "Directory 'inkscape_datadir/inkscape' missing. Tests might not run properly.\n"
                    "Possible solutions:\n"
                    " - create a suitable symlink yourself, e.g.\n"
                    "   ln -s ${CMAKE_SOURCE_DIR}/share ${INKSCAPE_DATADIR}/inkscape\n"
                    " - run '${CMAKE_MAKE_PROGRAM} install' before running tests (only for not relocatable packages.\n"
                    " - set the environment variable 'INKSCAPE_DATADIR' manually (every time you run tests)")
endif()


# Set custom profile directory for tests using environment variable.
# Copy CTestCustom.cmake into binary dir, where it will be picked up automatically by ctest for cleanup.
set(INKSCAPE_TEST_PROFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_profile_dir)
set(INKSCAPE_TEST_PROFILE_DIR_ENV INKSCAPE_PROFILE_DIR=${INKSCAPE_TEST_PROFILE_DIR})
configure_file(CTestCustom.cmake.in ${CMAKE_BINARY_DIR}/CTestCustom.cmake)



### tests using gtest
include_directories("${CMAKE_SOURCE_DIR}/src/3rdparty/adaptagrams") # TODO: remove this hack

set(TEST_SOURCES
    uri-test
    util-test
    drag-and-drop-svgz
    extract-uri-test
    attributes-test
    color-profile-test
    dir-util-test
    sp-object-test
    object-set-test
    object-style-test
    rebase-hrefs-test
    style-elem-test
    style-internal-test
    style-test
    svg-affine-test
    svg-color-test
    svg-length-test
    svg-stringstream-test
    sp-gradient-test
    svg-path-geom-test
    object-test
    sp-glyph-kerning-test
    cairo-utils-test
    svg-extension-test
    curve-test
    2geom-characterization-test
    lpe-bool-test
    xml-test
    sp-item-group-test)

add_library(cpp_test_static_library SHARED unittest.cpp doc-per-case-test.cpp)
target_link_libraries(cpp_test_static_library PUBLIC ${GTEST_LIBRARIES} inkscape_base)

add_custom_target(tests)
foreach(test_source ${TEST_SOURCES})
    string(REPLACE "-test" "" testname "test_${test_source}")
    add_executable(${testname} src/${test_source}.cpp)
    target_include_directories(${testname} SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(${testname} cpp_test_static_library 2Geom::2geom)
    add_test(NAME ${testname} COMMAND ${testname})
    set_tests_properties(${testname} PROPERTIES ENVIRONMENT "${INKSCAPE_TEST_PROFILE_DIR_ENV}/${testname};${CMAKE_CTEST_ENV}")
    add_dependencies(tests ${testname})
endforeach()


### CLI and rendering tests
add_subdirectory(cli_tests)
add_subdirectory(rendering_tests)


### Fuzz test
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
