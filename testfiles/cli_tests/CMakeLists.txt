# SPDX-License-Identifier: GPL-2.0-or-later


# Helper function to add a CLI test
#
# Run an Inkscape command line and check for pass/fail condition:
# - by default only exit status is checked
# - optionally: pass/fail for specific command line output
# - optionally: compare output with a pre-rendered file
#
# Command line options:
#   INPUT_FILENAME     - name of input file (optional)
#   OUTPUT_FILENAME    - name of output file (optional)
#   PARAMETERS         - additional command line parameters to pass to Inkscape
#
# Pass/fail criteria:
#   PASS_FOR_OUTPUT    - pass if output matches the given value, otherwise fail
#                        see https://cmake.org/cmake/help/latest/prop_test/PASS_REGULAR_EXPRESSION.html for details
#   FAIL_FOR_OUTPUT    - fail if output matches the given value
#                        see https://cmake.org/cmake/help/latest/prop_test/FAIL_REGULAR_EXPRESSION.html for details
#   REFERENCE_FILENAME - compare OUTPUT_FILENAME with this pre-rendered reference file
#                        both files are converted to PNG and compared with ImageMagick's 'compare'
#   EXPECTED_FILES     - verify the command produced the expected files (i.e. they exist on disk)
function(add_cli_test name)
    # parse arguments
    set(oneValueArgs INPUT_FILENAME OUTPUT_FILENAME PASS_FOR_OUTPUT FAIL_FOR_OUTPUT REFERENCE_FILENAME)
    set(multiValueArgs PARAMETERS EXPECTED_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(testname  cli_${name})

    if(DEFINED ARG_OUTPUT_FILENAME)
        set(ARG_PARAMETERS ${ARG_PARAMETERS} "--export-filename=${ARG_OUTPUT_FILENAME}")
    endif()
    if(DEFINED ARG_INPUT_FILENAME)
        set(ARG_INPUT_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_INPUT_FILENAME}")
        set(ARG_PARAMETERS ${ARG_PARAMETERS} ${ARG_INPUT_FILENAME})
    endif()

    # add test for main command line
    add_test(NAME ${testname} COMMAND inkscape ${ARG_PARAMETERS})
    set_tests_properties(${testname} PROPERTIES ENVIRONMENT "${CMAKE_CTEST_ENV}")
    if(DEFINED ARG_PASS_FOR_OUTPUT)
        set_tests_properties(${testname} PROPERTIES PASS_REGULAR_EXPRESSION ${ARG_PASS_FOR_OUTPUT})
    endif()
    if(DEFINED ARG_FAIL_FOR_OUTPUT)
        set_tests_properties(${testname} PROPERTIES FAIL_REGULAR_EXPRESSION ${ARG_FAIL_FOR_OUTPUT})
    endif()

    # add test to check output files
    if(DEFINED ARG_REFERENCE_FILENAME OR DEFINED ARG_EXPECTED_FILES)
        if(DEFINED ARG_REFERENCE_FILENAME)
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_REFERENCE_FILENAME}" ARG_REFERENCE_FILENAME)
        endif()
        if(DEFINED ARG_EXPECTED_FILES)
            string(REPLACE ";" " " ARG_EXPECTED_FILES "${ARG_EXPECTED_FILES}")
        endif()

        add_test(NAME ${testname}_check_output
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/check_output.sh "${ARG_OUTPUT_FILENAME}" "${ARG_REFERENCE_FILENAME}" "${ARG_EXPECTED_FILES}")
        set_tests_properties(${testname}_check_output PROPERTIES
            ENVIRONMENT "${CMAKE_CTEST_ENV}"
            DEPENDS ${testname}
            SKIP_REGULAR_EXPRESSION "Skipping comparison test.")
    endif()
endfunction(add_cli_test)



### Tests follow below ###

# check if we can run inkscape and the revision is known
add_cli_test(version       PARAMETERS --version)
add_cli_test(version_known PARAMETERS --version FAIL_FOR_OUTPUT unknown)

# check --query family of parameters
string(CONCAT query_all_expected "rect1,10,10,80,80\n"
                                 "rect2,110,20,80,70\n"
                                 "rect3,210,30,80,60")
add_cli_test(query_x       PARAMETERS --query-id=rect2 --query-x      INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 110)
add_cli_test(query_y       PARAMETERS --query-id=rect2 --query-y      INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 20)
add_cli_test(query_width   PARAMETERS --query-id=rect2 --query-width  INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 80)
add_cli_test(query_height  PARAMETERS --query-id=rect2 --query-height INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 70)
add_cli_test(query_all     PARAMETERS --query-id=rect2 --query-all    INPUT_FILENAME rects.svg PASS_FOR_OUTPUT ${query_all_expected})

# check --export-type
add_cli_test(export_type   PARAMETERS --export-type=svg,png,ps,eps,pdf,emf,wmf,xaml
                           INPUT_FILENAME empty.svg OUTPUT_FILENAME empty
                           EXPECTED_FILES empty.svg empty.png empty.ps empty.eps empty.pdf empty.emf empty.wmf empty.xaml)

# test basic support of export functionality into various formats
add_cli_test(export_png    PARAMETERS --export-type=png INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.png REFERENCE_FILENAME shapes_expected.png)
add_cli_test(export_ps     PARAMETERS --export-type=ps  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.ps  REFERENCE_FILENAME shapes_expected.ps)
add_cli_test(export_eps    PARAMETERS --export-type=eps INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.eps REFERENCE_FILENAME shapes_expected.eps)
add_cli_test(export_pdf    PARAMETERS --export-type=pdf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf REFERENCE_FILENAME shapes_expected.pdf)

# test librevenge formats
if(WITH_LIBCDR)
  add_cli_test(import_cdr  PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/corel_draw.cdr OUTPUT_FILENAME format_corel_draw.png
                           REFERENCE_FILENAME librevenge_formats/corel_draw_expected.png)
endif()
if(WITH_LIBVISIO)
  add_cli_test(import_vsd  PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/visio.vsd OUTPUT_FILENAME format_visio.vsd.png
                           REFERENCE_FILENAME librevenge_formats/visio.vsd_expected.png)
  add_cli_test(import_vsdx PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/visio.vsdx OUTPUT_FILENAME format_visio.vsdx.png
                           REFERENCE_FILENAME librevenge_formats/visio.vsdx_expected.png)
endif()
if(WITH_LIBWPG)
  add_cli_test(import_wpg  PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/word_perfect.wpg OUTPUT_FILENAME format_word_perfect.png
                           REFERENCE_FILENAME librevenge_formats/word_perfect_expected.png)
endif()
