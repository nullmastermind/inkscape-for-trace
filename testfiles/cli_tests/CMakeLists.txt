# SPDX-License-Identifier: GPL-2.0-or-later


# Helper function to add a CLI test
#
# Run an Inkscape command line and check for pass/fail condition (by default only exit status is checked)
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
#   TEST_SCRIPT        - additional script to run after performing all checks and before cleaning up
#
# Other options:
#   ENVIRONMENT        - Additional environment variables to set while running the test
function(add_cli_test name)
    # parse arguments
    set(oneValueArgs INPUT_FILENAME OUTPUT_FILENAME PASS_FOR_OUTPUT FAIL_FOR_OUTPUT REFERENCE_FILENAME)
    set(multiValueArgs PARAMETERS EXPECTED_FILES TEST_SCRIPT ENVIRONMENT)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(testname  cli_${name})

    if(DEFINED ARG_OUTPUT_FILENAME)
        set(ARG_PARAMETERS ${ARG_PARAMETERS} "--export-filename=${ARG_OUTPUT_FILENAME}")
    endif()
    if(DEFINED ARG_INPUT_FILENAME)
        set(ARG_INPUT_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_INPUT_FILENAME}")
        set(ARG_PARAMETERS ${ARG_PARAMETERS} ${ARG_INPUT_FILENAME})
    endif()

    set(CMAKE_CTEST_ENV "${INKSCAPE_TEST_PROFILE_DIR_ENV}/${testname};${CMAKE_CTEST_ENV}")
    if(DEFINED ARG_ENVIRONMENT)
        if(ARG_ENVIRONMENT STREQUAL "unset")
            unset(CMAKE_CTEST_ENV)
        else()
            # variables might already be set, however the last value wins
            list(APPEND CMAKE_CTEST_ENV ${ARG_ENVIRONMENT})
        endif()
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
    if(DEFINED ARG_REFERENCE_FILENAME OR DEFINED ARG_EXPECTED_FILES OR DEFINED ARG_TEST_SCRIPT)
        if(DEFINED ARG_REFERENCE_FILENAME)
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/testcases/${ARG_REFERENCE_FILENAME}" ARG_REFERENCE_FILENAME)
        endif()
        if(DEFINED ARG_EXPECTED_FILES)
            string(REPLACE ";" " " ARG_EXPECTED_FILES "${ARG_EXPECTED_FILES}")
        endif()
        if(DEFINED ARG_TEST_SCRIPT)
            set(ARG_TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_TEST_SCRIPT}")
        endif()

        add_test(NAME ${testname}_check_output
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/check_output.sh
                    "${ARG_OUTPUT_FILENAME}" "${ARG_REFERENCE_FILENAME}" "${ARG_EXPECTED_FILES}" "${ARG_TEST_SCRIPT}")
        set_tests_properties(${testname}_check_output PROPERTIES
            ENVIRONMENT "${CMAKE_CTEST_ENV}" DEPENDS ${testname} SKIP_RETURN_CODE 42)
    endif()
endfunction(add_cli_test)



##### Tests follow below #####



#############################################################################################
### Command line options (basic tests for all program options as listed in --help output) ###
#############################################################################################

# --help

# --version (check if we can run inkscape and the revision is known)
add_cli_test(version       PARAMETERS --version)
add_cli_test(version_known PARAMETERS --version FAIL_FOR_OUTPUT unknown)

# --system-data-directory / --user-data-directory (unset environment variables to override our override)
# TODO: Can we make these tests more specific without making too many assumptions?
add_cli_test(system-data-directory PARAMETERS --system-data-directory ENVIRONMENT unset PASS_FOR_OUTPUT "inkscape\n$")
add_cli_test(user-data-directory   PARAMETERS --user-data-directory   ENVIRONMENT unset PASS_FOR_OUTPUT "inkscape\n$")

# --pipe

# --pdf-page=PAGE

# --pdf-poppler
add_cli_test(pdf-poppler-mesh-import
                        PARAMETERS --pdf-poppler
                        INPUT_FILENAME pdf-mesh.pdf
                        OUTPUT_FILENAME pdf-mesh_poppler.svg
                        TEST_SCRIPT match_regex.sh pdf-mesh_poppler.svg "<image")
add_cli_test(pdf-internal-mesh-import
                        INPUT_FILENAME pdf-mesh.pdf
                        OUTPUT_FILENAME pdf-mesh_internal.svg
                        TEST_SCRIPT match_regex_fail.sh pdf-mesh_internal.svg "<image")

# --convert-dpi-method=METHOD

# --no-convert-text-baseline-spacing

# --export-filename=FILENAME
## check for https://gitlab.com/inkscape/inkscape/-/issues/1712
install(DIRECTORY DESTINATION ${test.dir})
add_cli_test(export-type-dot-separated PARAMETERS --export-type=svg,png
                         INPUT_FILENAME empty.svg OUTPUT_FILENAME test.dir/empty
                         EXPECTED_FILES test.dir/empty.svg test.dir/empty.png)

# --export-overwrite

# --export-type=TYPE[,TYPE]*

##  test whether we can export all default types in principle (and in one command)
add_cli_test(export-type PARAMETERS --export-type=svg,png,ps,eps,pdf,emf,wmf,xaml
                         INPUT_FILENAME empty.svg OUTPUT_FILENAME empty
                         EXPECTED_FILES empty.svg empty.png empty.ps empty.eps empty.pdf empty.emf empty.wmf empty.xaml)
##  test whether this also works if the export types are not lowercase
add_cli_test(export-type-caseinsensitive PARAMETERS --export-type=sVg,Png,PS,eps,Pdf,emf,wmf,xaml
                         INPUT_FILENAME empty.svg OUTPUT_FILENAME empty
                         EXPECTED_FILES empty.svg empty.png empty.ps empty.eps empty.pdf empty.emf empty.wmf empty.xaml)

## test whether we produce sane output for the default types
add_cli_test(export-type_svg  PARAMETERS --export-type=svg INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg)
add_cli_test(export-type_png  PARAMETERS --export-type=png INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.png REFERENCE_FILENAME shapes_expected.png)
add_cli_test(export-type_ps   PARAMETERS --export-type=ps  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.ps  REFERENCE_FILENAME shapes_expected.ps)
add_cli_test(export-type_eps  PARAMETERS --export-type=eps INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.eps REFERENCE_FILENAME shapes_expected.eps)
add_cli_test(export-type_pdf  PARAMETERS --export-type=pdf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf REFERENCE_FILENAME shapes_expected.pdf)
add_cli_test(export-type_emf  PARAMETERS --export-type=emf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.emf REFERENCE_FILENAME shapes_expected.emf)
add_cli_test(export-type_wmf  PARAMETERS --export-type=wmf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.wmf REFERENCE_FILENAME shapes_expected.wmf)
# XAML is not supported by ImageMagick's convert, so simply compare binary
add_cli_test(export-type_xaml PARAMETERS --export-type=xaml INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.xaml
                              TEST_SCRIPT compare.sh shapes.xaml "${CMAKE_CURRENT_SOURCE_DIR}/testcases/shapes_expected.xaml")

# --export-area-page
add_cli_test(export-area-page_png PARAMETERS --export-area-page --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.png REFERENCE_FILENAME export-area-page_expected.png)
add_cli_test(export-area-page_svg PARAMETERS --export-area-page --export-type=svg INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.svg REFERENCE_FILENAME export-area-page_expected.svg)
add_cli_test(export-area-page_pdf PARAMETERS --export-area-page --export-type=pdf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.pdf REFERENCE_FILENAME export-area-page_expected.pdf)
add_cli_test(export-area-page_ps  PARAMETERS --export-area-page --export-type=ps  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.ps  REFERENCE_FILENAME export-area-page_expected.ps)
# EPS: Currently not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1074
# add_cli_test(export-area-page_eps PARAMETERS --export-area-page --export-type=eps INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.eps REFERENCE_FILENAME export-area-page_expected.eps)
add_cli_test(export-area-page_emf PARAMETERS --export-area-page --export-type=emf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.emf REFERENCE_FILENAME export-area-page_expected.emf)
add_cli_test(export-area-page_wmf PARAMETERS --export-area-page --export-type=wmf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page.wmf REFERENCE_FILENAME export-area-page_expected.wmf)

# --export-area-page + --export-id  (+ --export-id-only)
# TODO: PDF/PS/EPS always behave as if --export-id-only was given, see https://gitlab.com/inkscape/inkscape/-/issues/1173
add_cli_test(export-area-page_export-id_png PARAMETERS --export-area-page --export-id=MyStar --export-id-only INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.png REFERENCE_FILENAME export-area-page_export-id.png)
add_cli_test(export-area-page_export-id_svg PARAMETERS --export-area-page --export-id=MyStar --export-id-only INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.svg REFERENCE_FILENAME export-area-page_export-id.svg)
add_cli_test(export-area-page_export-id_pdf PARAMETERS --export-area-page --export-id=MyStar                  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.pdf REFERENCE_FILENAME export-area-page_export-id.pdf)
add_cli_test(export-area-page_export-id_ps  PARAMETERS --export-area-page --export-id=MyStar                  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.ps  REFERENCE_FILENAME export-area-page_export-id.ps)
# EPS: Currently not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1074
#add_cli_test(export-area-page_export-id_eps PARAMETERS --export-area-page --export-id=MyStar INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-page_export-id.eps REFERENCE_FILENAME export-area-page_export-id.eps)
# EMF, WMF: Nont supported.

# --export-area-drawing
add_cli_test(export-area-drawing_png PARAMETERS --export-area-drawing --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.png REFERENCE_FILENAME export-area-drawing_expected.png)
add_cli_test(export-area-drawing_svg PARAMETERS --export-area-drawing --export-type=svg INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.svg REFERENCE_FILENAME export-area-drawing_expected.svg)
add_cli_test(export-area-drawing_pdf PARAMETERS --export-area-drawing --export-type=pdf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.pdf REFERENCE_FILENAME export-area-drawing_expected.pdf)
add_cli_test(export-area-drawing_ps  PARAMETERS --export-area-drawing --export-type=ps  INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.ps  REFERENCE_FILENAME export-area-drawing_expected.ps)
add_cli_test(export-area-drawing_eps PARAMETERS --export-area-drawing --export-type=eps INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.eps REFERENCE_FILENAME export-area-drawing_expected.eps)
# EMF, WMF: Currently not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1056
# add_cli_test(export-area-drawing_emf PARAMETERS --export-area-drawing --export-type=emf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.emf REFERENCE_FILENAME export-area-drawing_expected.emf)
# add_cli_test(export-area-drawing_wmf PARAMETERS --export-area-drawing --export-type=wmf INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area-drawing.wmf REFERENCE_FILENAME export-area-drawing_expected.wmf)

# --export-area=x0:y0:x1:y1
add_cli_test(export-area_png PARAMETERS --export-area=150:150:350:300 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-area.png REFERENCE_FILENAME export-area_expected.png)
# SVG, PDF, PS, EPS, EMF, WMF: Not supported. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/678

# --export-area-snap
add_cli_test(export-area-snap_export-id           PARAMETERS --export-id=rect_misaligned --export-area-snap --export-type=png INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-id.png      REFERENCE_FILENAME export-area-snap_expected.png)
add_cli_test(export-area-snap_export-area-drawing PARAMETERS --export-area-drawing       --export-area-snap --export-type=png INPUT_FILENAME pyramids.svg OUTPUT_FILENAME export-area-snap-drawing.png REFERENCE_FILENAME export-area-snap_expected.png)
# SVG, PDF, PS, EPS, EMF, WMF: doesn't make sense

# --export-dpi=DPI
add_cli_test(export-dpi_png PARAMETERS --export-dpi=12 --export-type=png INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.png REFERENCE_FILENAME export-dpi_expected.png)
# SVG: doesn't make sense
add_cli_test(export-dpi_pdf PARAMETERS --export-dpi=12 --export-type=pdf INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.pdf REFERENCE_FILENAME export-dpi_expected.pdf)
add_cli_test(export-dpi_ps  PARAMETERS --export-dpi=12 --export-type=ps  INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.ps  REFERENCE_FILENAME export-dpi_expected.ps)
add_cli_test(export-dpi_eps PARAMETERS --export-dpi=12 --export-type=eps INPUT_FILENAME filter.svg OUTPUT_FILENAME export-dpi.eps REFERENCE_FILENAME export-dpi_expected.eps)
# EMF, WMF: doesn't make sense

# --export-width=WIDTH / --export-height=HEIGHT
add_cli_test(export-width                   PARAMETERS --export-width=380                                      --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-width.png   REFERENCE_FILENAME export-width_expected.png)
add_cli_test(export-width_export-dpi        PARAMETERS --export-width=380 --export-dpi=300                     --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-width2.png  REFERENCE_FILENAME export-width_expected.png)
add_cli_test(export-width_export-use-hints  PARAMETERS --export-width=380 --export-use-hints --export-id=rect1 --export-type=png INPUT_FILENAME export_hints.svg
                                            PASS_FOR_OUTPUT "Area 10:10:90:70 exported to 380 x 285 pixels \\(456 dpi\\)"
                                            EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")
add_cli_test(export-height                  PARAMETERS --export-height=40                                      --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-height.png  REFERENCE_FILENAME export-height_expected.png)
add_cli_test(export-height_export-dpi       PARAMETERS --export-height=40 --export-dpi=300                     --export-type=png INPUT_FILENAME export_hints.svg OUTPUT_FILENAME export-height2.png REFERENCE_FILENAME export-height_expected.png)
add_cli_test(export-height_export-use-hints PARAMETERS --export-height=40 --export-use-hints --export-id=rect1 --export-type=png INPUT_FILENAME export_hints.svg
                                            PASS_FOR_OUTPUT "Area 10:10:90:70 exported to 53 x 40 pixels \\(64 dpi\\)"
                                            EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")
# SVG, PDF, PS, EPS, EMF, WMF: doesn't make sense

# --export-margin=MARGIN
# There are many problems:
# - PNG, EPS, EMF, WMF: --export-margin is't supported. This affects all PNG, EPS, EMF, WMF tests below. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1142
# - PDF: Defaults to margin in millimeters. This affects all mm based PDF tests below. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1142
# - PS: Defaults to margin in pixels. This affects all px based OS tests below. Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1142
# - --export-id for PDF/PS/EPS is buggy: works as --export-id + --export-id-only should work. See: https://gitlab.com/inkscape/inkscape/-/issues/1173
# - --export-margin + --export-area=x0:y0:x1:y1 is PNG only feature.
# - --export-margin + --export-use-hints is PNG only feature.
# There is no test for --export-margin + --export-area-page combination because it's equivalent to --export-margin

## simple --export-margin (millimeter based)
# add_cli_test(export-margin_mm_png PARAMETERS --export-margin=50 --export-type=png INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.png REFERENCE_FILENAME export-margin_mm_expected.png )
add_cli_test(export-margin_mm_svg PARAMETERS --export-margin=50 --export-type=svg INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.svg REFERENCE_FILENAME export-margin_mm_expected.svg )
add_cli_test(export-margin_mm_pdf PARAMETERS --export-margin=50 --export-type=pdf INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.pdf REFERENCE_FILENAME export-margin_mm_expected.pdf )
# add_cli_test(export-margin_mm_ps  PARAMETERS --export-margin=50 --export-type=ps  INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.ps  REFERENCE_FILENAME export-margin_mm_expected.ps  )
# add_cli_test(export-margin_mm_eps PARAMETERS --export-margin=50 --export-type=eps INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.eps REFERENCE_FILENAME export-margin_mm_expected.eps )
# add_cli_test(export-margin_mm_emf PARAMETERS --export-margin=50 --export-type=emf INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.emf REFERENCE_FILENAME export-margin_mm_expected.emf )
# add_cli_test(export-margin_mm_wmf PARAMETERS --export-margin=50 --export-type=wmf INPUT_FILENAME square_mm.svg OUTPUT_FILENAME export-margin_mm.wmf REFERENCE_FILENAME export-margin_mm_expected.wmf )

## simple --export-margin (pixel based)
# add_cli_test(export-margin_px_png PARAMETERS --export-margin=50 --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.png REFERENCE_FILENAME export-margin_px_expected.png )
add_cli_test(export-margin_px_svg PARAMETERS --export-margin=50 --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.svg REFERENCE_FILENAME export-margin_px_expected.svg )
# add_cli_test(export-margin_px_pdf PARAMETERS --export-margin=50 --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.pdf REFERENCE_FILENAME export-margin_px_expected.pdf )
add_cli_test(export-margin_px_ps  PARAMETERS --export-margin=50 --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.ps  REFERENCE_FILENAME export-margin_px_expected.ps  )
# add_cli_test(export-margin_px_eps PARAMETERS --export-margin=50 --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.eps REFERENCE_FILENAME export-margin_px_expected.eps )
# add_cli_test(export-margin_px_emf PARAMETERS --export-margin=50 --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.emf REFERENCE_FILENAME export-margin_px_expected.emf )
# add_cli_test(export-margin_px_wmf PARAMETERS --export-margin=50 --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_px.wmf REFERENCE_FILENAME export-margin_px_expected.wmf )

## --export-margin + --export-id (pixel based)
# add_cli_test(export-margin_export-id_png PARAMETERS --export-margin=50 --export-id=square-red --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.png REFERENCE_FILENAME export-margin_export-id_expected.png )
add_cli_test(export-margin_export-id_svg PARAMETERS --export-margin=50 --export-id=square-red --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.svg REFERENCE_FILENAME export-margin_export-id_expected.svg )
# add_cli_test(export-margin_export-id_pdf PARAMETERS --export-margin=50 --export-id=square-red --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.pdf REFERENCE_FILENAME export-margin_export-id_expected.pdf )
# add_cli_test(export-margin_export-id_ps  PARAMETERS --export-margin=50 --export-id=square-red --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.ps  REFERENCE_FILENAME export-margin_export-id_expected.ps  )
# add_cli_test(export-margin_export-id_eps PARAMETERS --export-margin=50 --export-id=square-red --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.eps REFERENCE_FILENAME export-margin_export-id_expected.eps )
# add_cli_test(export-margin_export-id_emf PARAMETERS --export-margin=50 --export-id=square-red --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.emf REFERENCE_FILENAME export-margin_export-id_expected.emf )
# add_cli_test(export-margin_export-id_wmf PARAMETERS --export-margin=50 --export-id=square-red --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id.wmf REFERENCE_FILENAME export-margin_export-id_expected.wmf )

## --export-margin + --export-id + export-id-only (pixel based)
# add_cli_test(export-margin_export-id_export-id-only_png PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.png REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.png )
add_cli_test(export-margin_export-id_export-id-only_svg PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.svg REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.svg )
# add_cli_test(export-margin_export-id_export-id-only_pdf PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.pdf REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.pdf )
add_cli_test(export-margin_export-id_export-id-only_ps  PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.ps  REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.ps  )
# add_cli_test(export-margin_export-id_export-id-only_eps PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.eps REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.eps )
# add_cli_test(export-margin_export-id_export-id-only_emf PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.emf REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.emf )
# add_cli_test(export-margin_export-id_export-id-only_wmf PARAMETERS --export-margin=50 --export-id=square-red --export-id-only --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-id_export-id-only.wmf REFERENCE_FILENAME export-margin_export-id_export-id-only_expected.wmf )

## --export-margin + --export-area-drawing (pixel based)
# add_cli_test(export-margin_export-area-drawing_png PARAMETERS --export-margin=50 --export-area-drawing --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.png REFERENCE_FILENAME export-margin_drawing_expected.png )
add_cli_test(export-margin_export-area-drawing_svg PARAMETERS --export-margin=50 --export-area-drawing --export-type=svg INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.svg REFERENCE_FILENAME export-margin_drawing_expected.svg )
# add_cli_test(export-margin_export-area-drawing_pdf PARAMETERS --export-margin=50 --export-area-drawing --export-type=pdf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.pdf REFERENCE_FILENAME export-margin_drawing_expected.pdf )
add_cli_test(export-margin_export-area-drawing_ps  PARAMETERS --export-margin=50 --export-area-drawing --export-type=ps  INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.ps  REFERENCE_FILENAME export-margin_drawing_expected.ps  )
# add_cli_test(export-margin_export-area-drawing_eps PARAMETERS --export-margin=50 --export-area-drawing --export-type=eps INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.eps REFERENCE_FILENAME export-margin_drawing_expected.eps )
# add_cli_test(export-margin_export-area-drawing_emf PARAMETERS --export-margin=50 --export-area-drawing --export-type=emf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.emf REFERENCE_FILENAME export-margin_drawing_expected.emf )
# add_cli_test(export-margin_export-area-drawing_wmf PARAMETERS --export-margin=50 --export-area-drawing --export-type=wmf INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_drawing.wmf REFERENCE_FILENAME export-margin_drawing_expected.wmf )

## --export-margin + --export-area=x0:y0:x1:y1 (pixel based)
# add_cli_test(export-margin_export-area_png PARAMETERS --export-margin=50 --export-area=50:50:100:100 --export-type=png INPUT_FILENAME square_px.svg OUTPUT_FILENAME export-margin_export-area.png REFERENCE_FILENAME export-margin_export-area_expected.png )

## --export-margin + --export-use-hints (pixel based)
# add_cli_test(export-margin_export-use-hints_export-id PARAMETERS --export-margin=50 --export-use-hints --export-id=rect1 INPUT_FILENAME export_hints.svg PASS_FOR_OUTPUT "Area 10:10:90:70 exported to 203 x 177 pixels \\(123 dpi\\)" EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")

## --export-margin (millimeter based, user units rescaled via viewBox)
# add_cli_test(export-margin_viewbox_png PARAMETERS --export-margin=50 --export-type=png INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.png REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.png )
add_cli_test(export-margin_viewbox_svg PARAMETERS --export-margin=50 --export-type=svg INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.svg REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.svg )
# add_cli_test(export-margin_viewbox_pdf PARAMETERS --export-margin=50 --export-type=pdf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.pdf REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.pdf )
# add_cli_test(export-margin_viewbox_ps  PARAMETERS --export-margin=50 --export-type=ps  INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.ps  REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.ps  )
# add_cli_test(export-margin_viewbox_eps PARAMETERS --export-margin=50 --export-type=eps INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.eps REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.eps )
# add_cli_test(export-margin_viewbox_emf PARAMETERS --export-margin=50 --export-type=emf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.emf REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.emf )
# add_cli_test(export-margin_viewbox_wmf PARAMETERS --export-margin=50 --export-type=wmf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox.wmf REFERENCE_FILENAME export-margin_mm_viewbox_page_expected.wmf )

## --export-margin + --export-area-drawing (millimeter based, user units rescaled via viewBox)
# add_cli_test(export-margin_viewbox_drawing_png PARAMETERS --export-margin=50 --export-area-drawing --export-type=png INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.png REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.png )
add_cli_test(export-margin_viewbox_drawing_svg PARAMETERS --export-margin=50 --export-area-drawing --export-type=svg INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.svg REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.svg )
# add_cli_test(export-margin_viewbox_drawing_pdf PARAMETERS --export-margin=50 --export-area-drawing --export-type=pdf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.pdf REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.pdf )
# add_cli_test(export-margin_viewbox_drawing_ps  PARAMETERS --export-margin=50 --export-area-drawing --export-type=ps  INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.ps  REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.ps  )
# add_cli_test(export-margin_viewbox_drawing_eps PARAMETERS --export-margin=50 --export-area-drawing --export-type=eps INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.eps REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.eps )
# add_cli_test(export-margin_viewbox_drawing_emf PARAMETERS --export-margin=50 --export-area-drawing --export-type=emf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.emf REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.emf )
# add_cli_test(export-margin_viewbox_drawing_wmf PARAMETERS --export-margin=50 --export-area-drawing --export-type=wmf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_drawing.wmf REFERENCE_FILENAME export-margin_mm_viewbox_drawing_expected.wmf )

## --export-margin + --export-id (millimeter based, user units rescaled via viewBox)
# add_cli_test(export-margin_viewbox_id_png PARAMETERS --export-margin=50 --export-id=square-red --export-type=png INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.png REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.png )
add_cli_test(export-margin_viewbox_id_svg PARAMETERS --export-margin=50 --export-id=square-red --export-type=svg INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.svg REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.svg )
# add_cli_test(export-margin_viewbox_id_pdf PARAMETERS --export-margin=50 --export-id=square-red --export-type=pdf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.pdf REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.pdf )
# add_cli_test(export-margin_viewbox_id_ps  PARAMETERS --export-margin=50 --export-id=square-red --export-type=ps  INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.ps  REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.ps  )
# add_cli_test(export-margin_viewbox_id_eps PARAMETERS --export-margin=50 --export-id=square-red --export-type=eps INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.eps REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.eps )
# add_cli_test(export-margin_viewbox_id_emf PARAMETERS --export-margin=50 --export-id=square-red --export-type=emf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.emf REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.emf )
# add_cli_test(export-margin_viewbox_id_wmf PARAMETERS --export-margin=50 --export-id=square-red --export-type=wmf INPUT_FILENAME square_mm_viewbox.svg OUTPUT_FILENAME export-margin_viewbox_id.wmf REFERENCE_FILENAME export-margin_mm_viewbox_id_expected.wmf )

# --export-id=OBJECT-ID[;OBJECT-ID]*
add_cli_test(export-id_png PARAMETERS --export-id=red --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.png REFERENCE_FILENAME export-id_expected.png )
add_cli_test(export-id_svg PARAMETERS --export-id=red --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.svg REFERENCE_FILENAME export-id_expected.svg )
# PDF, PS, EPS doesn't respect --export-id-only, bug report: https://gitlab.com/inkscape/inkscape/-/issues/1173
# add_cli_test(export-id_pdf PARAMETERS --export-id=red --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.pdf REFERENCE_FILENAME export-id_expected.pdf )
# add_cli_test(export-id_ps  PARAMETERS --export-id=red --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.ps  REFERENCE_FILENAME export-id_expected.ps  )
# add_cli_test(export-id_eps PARAMETERS --export-id=red --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.eps REFERENCE_FILENAME export-id_expected.eps )
# EMF, WMF There is no support, feature request: https://gitlab.com/inkscape/inkscape/-/issues/1204
# add_cli_test(export-id_emf PARAMETERS --export-id=red --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.emf REFERENCE_FILENAME export-id_expected.emf )
# add_cli_test(export-id_wmf PARAMETERS --export-id=red --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id.wmf REFERENCE_FILENAME export-id_expected.wmf )

## --export-id + --export-area-page = --export-area-page
## --export-id + --export-area-drawing = --export-area-drawing
## --export-id + --export-area-snap → export-area-snap_export-id
## --export-id + --export-margin → export-margin_export-id_png|svg|pdf|ps|eps|emf|wmf
## --export-id + --export-use-hints → export-use-hints_export-id
## --export-id + --export-use-hints + --export-width | --export-height → export-width_export-use-hints

# --export-id-only
## --export-id + --export-id-only
add_cli_test(export-id_export-id-only_png PARAMETERS --export-id=blue --export-id-only --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.png REFERENCE_FILENAME export-id_export-id-only_expected.png )
add_cli_test(export-id_export-id-only_svg PARAMETERS --export-id=blue --export-id-only --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.svg REFERENCE_FILENAME export-id_export-id-only_expected.svg )
add_cli_test(export-id_export-id-only_pdf PARAMETERS --export-id=blue --export-id-only --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.pdf REFERENCE_FILENAME export-id_export-id-only_expected.pdf )
add_cli_test(export-id_export-id-only_ps  PARAMETERS --export-id=blue --export-id-only --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.ps  REFERENCE_FILENAME export-id_export-id-only_expected.ps  )
add_cli_test(export-id_export-id-only_eps PARAMETERS --export-id=blue --export-id-only --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.eps REFERENCE_FILENAME export-id_export-id-only_expected.eps )
# EMF, WMF There is no support, feature request: https://gitlab.com/inkscape/inkscape/-/issues/1204
# add_cli_test(export-id_export-id-only_emf PARAMETERS --export-id=blue --export-id-only --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.emf REFERENCE_FILENAME export-id_export-id-only_expected.emf )
# add_cli_test(export-id_export-id-only_wmf PARAMETERS --export-id=blue --export-id-only --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only.wmf REFERENCE_FILENAME export-id_export-id-only_expected.wmf )

## --export-id + --export-id-only + --export-area-page
add_cli_test(export-id_export-id-only_export-area-page_png PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.png REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.png )
add_cli_test(export-id_export-id-only_export-area-page_svg PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.svg REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.svg )
add_cli_test(export-id_export-id-only_export-area-page_pdf PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.pdf REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.pdf )
add_cli_test(export-id_export-id-only_export-area-page_ps  PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.ps  REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.ps  )
# EPS: Depents on https://gitlab.com/inkscape/inkscape/-/issues/1074
# add_cli_test(export-id_export-id-only_export-area-page_eps PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.eps REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.eps )
# EMF, WMF: There is no support, feature request: https://gitlab.com/inkscape/inkscape/-/issues/1204
# add_cli_test(export-id_export-id-only_export-area-page_emf PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.emf REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.emf )
# add_cli_test(export-id_export-id-only_export-area-page_wmf PARAMETERS --export-id=yellow --export-id-only --export-area-page --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-page.wmf REFERENCE_FILENAME export-id_export-id-only_export-area-page_expected.wmf )

## --export-id + --export-id-only + --export-area-drawing
# PNG, PDF, PS, EPS, EMF, WMF: Feature request: https://gitlab.com/inkscape/inkscape/-/issues/1215
# add_cli_test(export-id_export-id-only_export-area-drawing_png PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=png INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.png REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.png )
add_cli_test(export-id_export-id-only_export-area-drawing_svg PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=svg INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.svg REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.svg )
# add_cli_test(export-id_export-id-only_export-area-drawing_pdf PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=pdf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.pdf REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.pdf )
# add_cli_test(export-id_export-id-only_export-area-drawing_ps  PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=ps  INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.ps  REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.ps  )
# add_cli_test(export-id_export-id-only_export-area-drawing_eps PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=eps INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.eps REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.eps )
# add_cli_test(export-id_export-id-only_export-area-drawing_emf PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=emf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.emf REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.emf )
# add_cli_test(export-id_export-id-only_export-area-drawing_wmf PARAMETERS --export-id=yellow --export-id-only --export-area-drawing --export-type=wmf INPUT_FILENAME theta.svg OUTPUT_FILENAME export-id_export-id-only_export-area-drawing.wmf REFERENCE_FILENAME export-id_export-id-only_export-area-drawing_expected.wmf )

## --export-id + --export-id-only + --export-margin → export-margin_export-id_export-id-only_png|svg|pdf|ps|eps|emw|wmf

# --export-plain-svg
add_cli_test(export-plain-svg PARAMETERS --export-type=svg --export-plain-svg
                              INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg
                              TEST_SCRIPT match_regex_fail.sh "shapes.svg" "inkscape:|sodipodi:")

# --export-ps-level=LEVEL
add_cli_test(export-ps-level_2          PARAMETERS --export-ps-level=2 --export-type=ps INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-2.ps REFERENCE_FILENAME export-ps-level-2_expected.ps )
add_cli_test(export-ps-level_2_content  PARAMETERS --export-ps-level=2 --export-type=ps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-2.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-2.ps" "%%LanguageLevel: 2")
add_cli_test(export-ps-level_3          PARAMETERS --export-ps-level=3 --export-type=ps INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-3.ps REFERENCE_FILENAME export-ps-level-3_expected.ps )
add_cli_test(export-ps-level_3_content  PARAMETERS --export-ps-level=3 --export-type=ps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-3.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-3.ps" "%%LanguageLevel: 3")
add_cli_test(export-eps-level_2_content PARAMETERS --export-ps-level=2 --export-type=eps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-2.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-2.eps" "%%LanguageLevel: 2")
add_cli_test(export-eps-level_3_content PARAMETERS --export-ps-level=3 --export-type=eps
                                        INPUT_FILENAME gradient.svg OUTPUT_FILENAME export-ps-level-3.ps
                                        TEST_SCRIPT match_regex.sh "export-ps-level-3.eps" "%%LanguageLevel: 3")

# --export-pdf-version=VERSION
add_cli_test(export-pdf-version-14 PARAMETERS --export-pdf-version=1.4 --export-type=pdf
                                   INPUT_FILENAME rects.svg OUTPUT_FILENAME export-pdf-version-14.pdf
                                   TEST_SCRIPT match_regex.sh "export-pdf-version-14.pdf" "^%PDF-1.4$")
add_cli_test(export-pdf-version-15 PARAMETERS --export-pdf-version=1.5 --export-type=pdf
                                   INPUT_FILENAME rects.svg OUTPUT_FILENAME export-pdf-version-15.pdf
                                   TEST_SCRIPT match_regex.sh "export-pdf-version-15.pdf" "^%PDF-1.5$")

# --export-text-to-path
# PNG: Doesn't make sense.
# EMF/WMF: No way to check.
add_cli_test(preserve-text_pdf       PARAMETERS --export-type=pdf
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME preserve-text.pdf
                                     TEST_SCRIPT match_regex.sh "preserve-text.pdf" "/FontDescriptor")
add_cli_test(preserve-text_ps        PARAMETERS --export-type=ps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME preserve-text.ps
                                     TEST_SCRIPT match_regex.sh "preserve-text.ps" "^%%BeginResource: font")
add_cli_test(preserve-text_eps       PARAMETERS --export-type=eps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME preserve-text.eps
                                     TEST_SCRIPT match_regex.sh "preserve-text.eps" "^%%BeginResource: font")
add_cli_test(export-text-to-path_svg PARAMETERS --export-text-to-path --export-type=svg
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.svg
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.svg" "<text")
add_cli_test(export-text-to-path_pdf PARAMETERS --export-text-to-path --export-type=pdf
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.pdf
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.pdf" "/FontDescriptor")
add_cli_test(export-text-to-path_ps  PARAMETERS --export-text-to-path --export-type=ps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.ps
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.ps" "^%%BeginResource: font")
add_cli_test(export-text-to-path_eps PARAMETERS --export-text-to-path --export-type=eps
                                     INPUT_FILENAME text.svg OUTPUT_FILENAME export-text-to-path.eps
                                     TEST_SCRIPT match_regex_fail.sh "export-text-to-path.eps" "^%%BeginResource: font")

# --export-latex
# PDF, PS, EPS only function by design.
add_cli_test(export-latex PARAMETERS --export-type=pdf,ps,eps --export-latex
                          INPUT_FILENAME text.svg OUTPUT_FILENAME export-latex
                          EXPECTED_FILES export-latex.pdf export-latex.pdf_tex export-latex.ps export-latex.ps_tex export-latex.eps export-latex.eps_tex
                          TEST_SCRIPT match_regex.sh "export-latex.pdf_tex" "some text")

# --export-ignore-filters
## testing exporting with filters
add_cli_test(export-with-filters_svg  PARAMETERS --export-type=svg INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.svg REFERENCE_FILENAME export-with-filters_expected.svg)
add_cli_test(export-with-filters_png  PARAMETERS --export-type=png INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.png REFERENCE_FILENAME export-with-filters_expected.png)
add_cli_test(export-with-filters_ps   PARAMETERS --export-type=ps  INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.ps  REFERENCE_FILENAME export-with-filters_expected.ps )
add_cli_test(export-with-filters_eps  PARAMETERS --export-type=eps INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.eps REFERENCE_FILENAME export-with-filters_expected.eps)
add_cli_test(export-with-filters_pdf  PARAMETERS --export-type=pdf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.pdf REFERENCE_FILENAME export-with-filters_expected.pdf)
# EMF, WMF: No support for exporting filters. Feature request: https://gitlab.com/inkscape/inbox/-/issues/2275
# add_cli_test(export-with-filters_emf  PARAMETERS --export-type=emf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.emf REFERENCE_FILENAME export-with-filters_expected.emf)
# add_cli_test(export-with-filters_wmf  PARAMETERS --export-type=wmf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-with-filters.wmf REFERENCE_FILENAME export-with-filters_expected.wmf)
## --export-ignore-filters
# SVG, PNG: No support for --export-ignore-filters. Feature request: https://gitlab.com/inkscape/inbox/-/issues/2275
# add_cli_test(export-ignore-filters_svg  PARAMETERS --export-ignore-filters --export-type=svg INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.svg REFERENCE_FILENAME export-ignore-filters_expected.svg)
# add_cli_test(export-ignore-filters_png  PARAMETERS --export-ignore-filters --export-type=png INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.png REFERENCE_FILENAME export-ignore-filters_expected.png)
add_cli_test(export-ignore-filters_ps   PARAMETERS --export-ignore-filters --export-type=ps  INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.ps  REFERENCE_FILENAME export-ignore-filters_expected.ps )
# EPS: Works, but depends on https://gitlab.com/inkscape/inkscape/-/issues/1074
# add_cli_test(export-ignore-filters_eps  PARAMETERS --export-ignore-filters --export-type=eps INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.eps REFERENCE_FILENAME export-ignore-filters_expected.eps)
add_cli_test(export-ignore-filters_pdf  PARAMETERS --export-ignore-filters --export-type=pdf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.pdf REFERENCE_FILENAME export-ignore-filters_expected.pdf)
add_cli_test(export-ignore-filters_emf  PARAMETERS --export-ignore-filters --export-type=emf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.emf REFERENCE_FILENAME export-ignore-filters_expected.emf)
add_cli_test(export-ignore-filters_wmf  PARAMETERS --export-ignore-filters --export-type=wmf INPUT_FILENAME offset.svg OUTPUT_FILENAME export-ignore-filters.wmf REFERENCE_FILENAME export-ignore-filters_expected.wmf)

# --export-use-hints
add_cli_test(export-use-hints_export-id           PARAMETERS --export-use-hints --export-id=rect1 INPUT_FILENAME export_hints.svg
                                                  PASS_FOR_OUTPUT "Area 10:10:90:70 exported to 103 x 77 pixels \\(123 dpi\\)"
                                                  EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_rectangle.png")
add_cli_test(export-use-hints_export-area-drawing PARAMETERS --export-use-hints --export-area-drawing INPUT_FILENAME export_hints.svg
                                                  PASS_FOR_OUTPUT "Area 10:10:180:70 exported to 197 x 69 pixels \\(111 dpi\\)"
                                                  EXPECTED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/testcases/export_hints_drawing.png")

# --export-background=COLOR
## check for default background color and opacity
# SVG, PDF, PS, EPS: Format not supoorted. Feature request: https://gitlab.com/inkscape/inbox/issues/2293
# EMF, WMF: Format does not support opacity.
 add_cli_test(export-default-background_png  PARAMETERS --export-type=png INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.png REFERENCE_FILENAME export-default-background_expected.png)
 add_cli_test(export-default-background_svg  PARAMETERS --export-type=svg INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.svg REFERENCE_FILENAME export-default-background_expected.svg)
# add_cli_test(export-default-background_pdf  PARAMETERS --export-type=pdf INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.pdf REFERENCE_FILENAME export-default-background_expected.pdf)
# add_cli_test(export-default-background_ps   PARAMETERS --export-type=ps  INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.ps  REFERENCE_FILENAME export-default-background_expected.ps )
# add_cli_test(export-default-background_eps  PARAMETERS --export-type=eps INPUT_FILENAME lambda-background.svg OUTPUT_FILENAME export-default-background.eps REFERENCE_FILENAME export-default-background_expected.eps)
## test --export-background=COLOR
 add_cli_test(export-background_png  PARAMETERS --export-background=yellow --export-type=png INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.png REFERENCE_FILENAME export-background_expected.png)
 add_cli_test(export-background_svg  PARAMETERS --export-background=yellow --export-type=svg INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.svg REFERENCE_FILENAME export-background_expected.svg)
# add_cli_test(export-background_pdf  PARAMETERS --export-background=yellow --export-type=pdf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.pdf REFERENCE_FILENAME export-background_expected.pdf)
# add_cli_test(export-background_ps   PARAMETERS --export-background=yellow --export-type=ps  INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.ps  REFERENCE_FILENAME export-background_expected.ps )
# add_cli_test(export-background_eps  PARAMETERS --export-background=yellow --export-type=eps INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.eps REFERENCE_FILENAME export-background_expected.eps)
# add_cli_test(export-background_emf  PARAMETERS --export-background=yellow --export-type=emf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.emf REFERENCE_FILENAME export-background_expected.emf)
# add_cli_test(export-background_wmf  PARAMETERS --export-background=yellow --export-type=wmf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background.wmf REFERENCE_FILENAME export-background_expected.wmf)

# --export-background-opacity=VALUE
# SVG, PDF, PS, EPS: Format not supoorted. Feature request: https://gitlab.com/inkscape/inbox/issues/2293
# EMF, WMF: Format does not support opacity.
add_cli_test(export-background-opacity_png  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=png INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.png REFERENCE_FILENAME export-background-opacity_expected.png)
 add_cli_test(export-background-opacity_svg  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=svg INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.svg REFERENCE_FILENAME export-background-opacity_expected.svg)
# add_cli_test(export-background-opacity_pdf  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=pdf INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.pdf REFERENCE_FILENAME export-background-opacity_expected.pdf)
# add_cli_test(export-background-opacity_ps   PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=ps  INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.ps  REFERENCE_FILENAME export-background-opacity_expected.ps )
# add_cli_test(export-background-opacity_eps  PARAMETERS --export-background=yellow --export-background-opacity=0.5 --export-type=eps INPUT_FILENAME lambda.svg OUTPUT_FILENAME export-background-opacity.eps REFERENCE_FILENAME export-background-opacity_expected.eps)

# --export-png-color-mode=COLOR-MODE
# SVG, PDF, PS, EPS, EMF, WMF: Vector formats - bitmap bit-depth and color-type not relevant there.
 add_cli_test(export-png-color-mode-gray-8_png  PARAMETERS --export-png-color-mode=Gray_8 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-png-color-mode-gray-8.png REFERENCE_FILENAME export-png-color-mode-gray-8_expected.png)
 add_cli_test(export-png-color-mode-rgb-8_png  PARAMETERS --export-png-color-mode=RGB_8 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-png-color-mode-rgb-8.png REFERENCE_FILENAME export-png-color-mode-rgb-8_expected.png)
 add_cli_test(export-png-color-mode-rgba-8_png  PARAMETERS --export-png-color-mode=RGBA_8 --export-type=png INPUT_FILENAME areas.svg OUTPUT_FILENAME export-png-color-mode-rgba-8.png REFERENCE_FILENAME export-png-color-mode-rgba-8_expected.png)

## test whether we produce correct output for default export extensions
add_cli_test(export-extension_svg  PARAMETERS --export-type=svg --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg)
add_cli_test(export-extension_ps   PARAMETERS --export-type=ps --export-extension=org.inkscape.print.ps.cairo INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.ps  REFERENCE_FILENAME shapes_expected.ps)
add_cli_test(export-extension_eps  PARAMETERS --export-type=eps --export-extension=org.inkscape.print.eps.cairo  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.eps REFERENCE_FILENAME shapes_expected.eps)
add_cli_test(export-extension_pdf  PARAMETERS --export-type=pdf --export-extension=org.inkscape.output.pdf.cairorenderer INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf REFERENCE_FILENAME shapes_expected.pdf)
add_cli_test(export-extension_emf  PARAMETERS --export-type=emf --export-extension=org.inkscape.output.emf  INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.emf REFERENCE_FILENAME shapes_expected.emf)
add_cli_test(export-extension_wmf  PARAMETERS --export-type=wmf --export-extension=org.inkscape.output.wmf INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.wmf REFERENCE_FILENAME shapes_expected.wmf)
add_cli_test(export-plain-extension-svg PARAMETERS --export-type=svg --export-extension=org.inkscape.output.svg.plain
                              INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.svg REFERENCE_FILENAME shapes.svg
                              TEST_SCRIPT match_regex_fail.sh "shapes.svg" "inkscape:|sodipodi:")
## guess file type from --export-extension
add_cli_test(export-type-extension_svg_guess_file_type  PARAMETERS --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes)

## check error messages for --export-extension
add_cli_test(export-type-extension_png_error  PARAMETERS --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.png 
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: The parameter --export-extension is invalid for PNG export")
add_cli_test(export-type-extension_type_error  PARAMETERS --export-extension=org.inkscape.output.svg.inkscape INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.pdf
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: The supplied extension ID \\(org\\.inkscape\\.output\\.svg\\.inkscape\\) does not match any of the extensions available for this file type.*")
add_cli_test(export-type-extension_id_error  PARAMETERS --export-extension=whatever INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: The supplied --export-extension was not found. Specify a file extension to get a list of available extensions for this file type.")
add_cli_test(export-type_filetype_error INPUT_FILENAME shapes.svg OUTPUT_FILENAME shapes.xxx
                            PASS_FOR_OUTPUT "InkFileExportCmd::do_export: Unknown export type: xxx. Allowed values: \\[.*]")

# --query-id=OBJECT-ID[,OBJECT-ID]*

# --query-all / --query-x / --query-y / --query-width / --query-height
string(CONCAT query_all_expected "rect1,10,10,80,80\n"
                                 "rect2,110,20,80,70\n"
                                 "rect3,210,30,80,60")
add_cli_test(query-all     PARAMETERS --query-id=rect2 --query-all    INPUT_FILENAME rects.svg PASS_FOR_OUTPUT ${query_all_expected})
add_cli_test(query-x       PARAMETERS --query-id=rect2 --query-x      INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 110)
add_cli_test(query-y       PARAMETERS --query-id=rect2 --query-y      INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 20)
add_cli_test(query-width   PARAMETERS --query-id=rect2 --query-width  INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 80)
add_cli_test(query-height  PARAMETERS --query-id=rect2 --query-height INPUT_FILENAME rects.svg PASS_FOR_OUTPUT 70)

# --vacuum-defs

# --select=OBJECT-ID[,OBJECT-ID]*

# --actions / --verbs
# (see below)

# --action-list / --verb-list
add_cli_test(action-list   PARAMETERS --action-list PASS_FOR_OUTPUT "file-new")
add_cli_test(verb-list     PARAMETERS --verb-list   PASS_FOR_OUTPUT "FileNew:")

# --with-gui

# --batch-process

# --shell



#########################
### actions and verbs ###
#########################

# (TODO)



###########################
### file format support ###
###########################

# librevenge formats
if(WITH_LIBCDR)
  # add_cli_test(import_cdr  PARAMETERS --export-type=png   # fails to open (regression in libcdr 1.6.0)
  #                          INPUT_FILENAME librevenge_formats/corel_draw.cdr OUTPUT_FILENAME format_corel_draw.png
  #                          REFERENCE_FILENAME librevenge_formats/corel_draw_expected.png)
  add_cli_test(import_cdr2 PARAMETERS --export-type=png
                           INPUT_FILENAME librevenge_formats/corel_draw2.cdr OUTPUT_FILENAME format_corel_draw2.png
                           REFERENCE_FILENAME librevenge_formats/corel_draw2_expected.png)
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



##############################
### advanced functionality ###
##############################

# check whether INKSCAPE_DATADIR / INKSCAPE_PROFILE_DIR environment variables work
# TODO: INKSCAPE_PROFILE_DIR does not seem to be sanitized at all (i.e. is used verbatim by Inkscape)
set(fancy_dir "i_certainly_do_not_exist")
file(TO_NATIVE_PATH "${fancy_dir}/inkscape" expected_dir)
string(REPLACE "\\" "\\\\" expected_dir "${expected_dir}")
add_cli_test(inkscape_datadir      PARAMETERS --system-data-directory
                                   ENVIRONMENT INKSCAPE_DATADIR=${fancy_dir}
                                   PASS_FOR_OUTPUT "${expected_dir}\n$")
add_cli_test(inkscape_profile_dir  PARAMETERS --user-data-directory
                                   ENVIRONMENT INKSCAPE_PROFILE_DIR=${fancy_dir}/inkscape
                                   PASS_FOR_OUTPUT "${fancy_dir}/inkscape\n$")
add_cli_test(inkscape_profile_dir_handle_illegal
                                   ENVIRONMENT INKSCAPE_PROFILE_DIR=invalid:dir
                                   INPUT_FILENAME empty.svg OUTPUT_FILENAME empty.svg)

# check if "systemLanguage" attribute is properly handled
add_cli_test(systemLanguage_en     ENVIRONMENT LANGUAGE=en          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_en.png
                                                                    REFERENCE_FILENAME systemLanguage_en.png)
add_cli_test(systemLanguage_fr     ENVIRONMENT LANGUAGE=fr_FR       INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_fr.png
                                                                    REFERENCE_FILENAME systemLanguage_fr.png)
add_cli_test(systemLanguage_fr2    ENVIRONMENT LANGUAGE=fr_FR.UTF-8 INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_fr2.png
                                                                    REFERENCE_FILENAME systemLanguage_fr.png)
add_cli_test(systemLanguage_de     ENVIRONMENT LANGUAGE=de          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_de.png
                                                                    REFERENCE_FILENAME systemLanguage_de.png)
add_cli_test(systemLanguage_de-CH  ENVIRONMENT LANGUAGE=de_CH       INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_de-CH.png
                                                                    REFERENCE_FILENAME systemLanguage_de.png)
add_cli_test(systemLanguage_pt     ENVIRONMENT LANGUAGE=pt          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_pt.png
                                                                    REFERENCE_FILENAME systemLanguage_pt.png)
add_cli_test(systemLanguage_xy     ENVIRONMENT LANGUAGE=xy          INPUT_FILENAME     systemLanguage.svg
                                                                    OUTPUT_FILENAME    systemLanguage_xy.png
                                                                    REFERENCE_FILENAME systemLanguage_default.png)
add_cli_test(systemLanguage_fr_RDF ENVIRONMENT LANGUAGE=xy          INPUT_FILENAME     systemLanguage_RDF.svg
                                                                    OUTPUT_FILENAME    systemLanguage_fr_RDF.png
                                                                    REFERENCE_FILENAME systemLanguage_fr.png)
