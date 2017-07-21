# dist targets for various platforms

set(INKSCAPE_DIST_PREFIX "${PROJECT_NAME}-${INKSCAPE_VERSION}")

# get INKSCAPE_REVISION of the source
set(INKSCAPE_SOURCE_DIR ${CMAKE_SOURCE_DIR})
include(CMakeScripts/inkscape-version.cmake)

if(INKSCAPE_VERSION_SUFFIX AND INKSCAPE_REVISION_DATE AND INKSCAPE_REVISION_HASH)
    set(INKSCAPE_DIST_PREFIX ${INKSCAPE_DIST_PREFIX}_${INKSCAPE_REVISION_DATE}_${INKSCAPE_REVISION_HASH})
endif()


# -----------------------------------------------------------------------------
# 'dist' - generate source release tarball
# -----------------------------------------------------------------------------

add_custom_target(dist
    COMMAND sed -i "s/unknown/${INKSCAPE_REVISION}/" CMakeScripts/inkscape-version.cmake
         && tar cfz ${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.tar.gz ${CMAKE_SOURCE_DIR}/doc --exclude=".*" --exclude-vcs-ignores
         || git checkout ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-version.cmake
    COMMAND git checkout ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-version.cmake # duplicate to make sure we actually revert in case of error
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM)
