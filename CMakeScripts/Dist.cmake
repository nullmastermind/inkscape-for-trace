# dist targets for various platforms


# get INKSCAPE_REVISION of the source
set(INKSCAPE_SOURCE_DIR ${CMAKE_SOURCE_DIR})
include(CMakeScripts/inkscape-version.cmake)

# set distribution prefix (used as filename for distributable packages)
set(INKSCAPE_DIST_PREFIX "${PROJECT_NAME}-${INKSCAPE_VERSION}")
if(INKSCAPE_REVISION_DATE AND INKSCAPE_REVISION_HASH)
    set(INKSCAPE_DIST_PREFIX ${INKSCAPE_DIST_PREFIX}_${INKSCAPE_REVISION_DATE}_${INKSCAPE_REVISION_HASH})
endif()



# -----------------------------------------------------------------------------
# 'dist' - generate source release tarball
# -----------------------------------------------------------------------------

message("INKSCAPE_DIST_PREFIX: ${INKSCAPE_DIST_PREFIX}")
add_custom_target(dist
	COMMAND sed -i "s/unknown/${INKSCAPE_REVISION}/" ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-version.cmake
	COMMAND cmake --build ${CMAKE_BINARY_DIR} --target package_source
	COMMAND git checkout ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-version.cmake
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM)


# -----------------------------------------------------------------------------
# 'dist-win' - Windows Targets
# -----------------------------------------------------------------------------
if(WIN32)
    if(HAVE_MINGW64)
        set(bitness "x64")
    else()
        set(bitness "x86")
    endif()
    set(INKSCAPE_DIST_PREFIX ${INKSCAPE_DIST_PREFIX}-${bitness})

    # -----------------------------------------------------------------------------
    # 'dist-win-7z' - generate binary 7z archive for Windows
    # -----------------------------------------------------------------------------
    find_program(7z 7z PATHS "C:/Program Files/7-Zip"
                             "C:/Program Files (x86)/7-Zip")
    if(NOT 7z)
        set(7z echo "Could not find '7z'. Please add it to your search path." && exit 1 &&)
    endif()

    # default target with very good but slow compression (needs approx. 10 GB RAM)
    add_custom_target(dist-win-7z
        COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.7z"
        COMMAND ${7z} a -mx9 -md512m -mfb256
                      "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.7z"
                      "${CMAKE_INSTALL_PREFIX}")

    # fast target with moderate compression
    add_custom_target(dist-win-7z-fast
        COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.7z"
        COMMAND ${7z} a
                      "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.7z"
                      "${CMAKE_INSTALL_PREFIX}")

    add_dependencies(dist-win-7z install/strip)
    add_dependencies(dist-win-7z-fast install/strip)

    # -----------------------------------------------------------------------------
    # 'dist-win-exe' - generate .exe installer (NSIS) for Windows
    # -----------------------------------------------------------------------------
    add_custom_target(dist-win-exe COMMAND ${CMAKE_CPACK_COMMAND} -G NSIS)
    add_dependencies(dist-win-exe install/strip) # TODO: we'd only need to depend on the "all" target

    # -----------------------------------------------------------------------------
    # 'dist-win-msi' - generate .exe installer (NSIS) for Windows
    # -----------------------------------------------------------------------------
    add_custom_target(dist-win-msi COMMAND ${CMAKE_CPACK_COMMAND} -G WIX)
    add_dependencies(dist-win-msi install/strip) # TODO: we'd only need to depend on the "all" target

    # -----------------------------------------------------------------------------
    # 'dist-win-all' - generate all 'dist' targets for Windows
    # -----------------------------------------------------------------------------
    add_custom_target(dist-win-all DEPENDS dist-win-7z dist-win-exe dist-win-msi)
endif()
