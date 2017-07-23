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
    find_program(7z 7z PATHS "C:\\Program Files\\7-Zip"
                             "C:\\Program Files (x86)\\7-Zip")
    if(NOT 7z)
        set(7z echo "Could not find '7z'. Please add it to your search path." && exit 1 &&)
    endif()

    # default target with very good but slow compression (needs approx. 10 GB RAM)
    add_custom_target(dist-win-7z
        COMMAND ${7z} a -mx9 -md512m -mfb256
                      "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.7z"
                      "${CMAKE_INSTALL_PREFIX}")

    # fast target with moderate compression
    add_custom_target(dist-win-7z-fast
        COMMAND ${7z} a
                      "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.7z"
                      "${CMAKE_INSTALL_PREFIX}")

    add_dependencies(dist-win-7z install/strip)
    add_dependencies(dist-win-7z-fast install/strip)

    # -----------------------------------------------------------------------------
    # 'dist-win-exe' - generate .exe installer (NSIS) for Windows
    # -----------------------------------------------------------------------------
    find_program (makensis makensis PATHS "C:\\Program Files\\NSIS"
                                          "C:\\Program Files (x86)\\NSIS")
    if(NOT makensis)
        set(makensis echo "Could not find 'makensis'. Please add it to your search path." && exit 1 &&)
    endif()

    # default target with good but slow compression
    add_custom_target(dist-win-exe
        COMMAND ${makensis} /D"INKSCAPE_DIST_DIR=${CMAKE_INSTALL_PREFIX}"
                            /D"OutFile=${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.exe"
                            "${CMAKE_SOURCE_DIR}/packaging/win32/inkscape.nsi")

    # fast target with low compression for testing
    add_custom_target(dist-win-exe-fast
        COMMAND ${makensis} /X"SetCompressor /FINAL /SOLID bzip2"
                            /D"INKSCAPE_DIST_DIR=${CMAKE_INSTALL_PREFIX}"
                            /D"OutFile=${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.exe"
                            "${CMAKE_SOURCE_DIR}/packaging/win32/inkscape.nsi")

    add_dependencies(dist-win-exe install/strip)
    add_dependencies(dist-win-exe-fast install/strip)

    # -----------------------------------------------------------------------------
    # 'dist-win-msi' - generate .exe installer (NSIS) for Windows
    # -----------------------------------------------------------------------------
    find_program (candle candle PATHS "C:\\Program Files\\WiX Toolset v3.10\\bin"
                                      "C:\\Program Files (x86)\\WiX Toolset v3.10\\bin")
    find_program (light  light  PATHS "C:\\Program Files\\WiX Toolset v3.10\\bin"
                                      "C:\\Program Files (x86)\\WiX Toolset v3.10\\bin")
    if(NOT candle)
        set(candle echo "Could not find 'candle' (part of WiX Toolset). Please add it to your search path." && exit 1 &&)
    endif()
    if(NOT light)
        set(light echo "Could not find 'light' (part of WiX Toolset). Please add it to your search path." && exit 1 &&)
    endif()

    # default target with fair but slow compression
    add_custom_target(dist-win-msi
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/packaging/wix ${CMAKE_BINARY_DIR}/wix
        COMMAND cd wix
        COMMAND ${CMAKE_COMMAND} -E env INKSCAPE_DIST_PATH=${CMAKE_INSTALL_PREFIX}
                    python ${CMAKE_SOURCE_DIR}/packaging/wix/files.py
        COMMAND ${CMAKE_COMMAND} -E env INKSCAPE_DIST_PATH=${CMAKE_INSTALL_PREFIX}
                    python ${CMAKE_SOURCE_DIR}/packaging/wix/version.py
        COMMAND ${candle} inkscape.wxs -ext WiXUtilExtension
        COMMAND ${candle} files.wxs
        COMMAND ${light} -ext WixUIExtension -ext WiXUtilExtension inkscape.wixobj files.wixobj
                         -o ${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.msi)

    # moderately fast target with no compression for testing
    add_custom_target(dist-win-msi-fast
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/packaging/wix ${CMAKE_BINARY_DIR}/wix
        COMMAND cd wix
        COMMAND sed -i 's/CompressionLevel="high"/CompressionLevel="none"/' inkscape.wxs
        COMMAND ${CMAKE_COMMAND} -E env INKSCAPE_DIST_PATH=${CMAKE_INSTALL_PREFIX}
                    python ${CMAKE_SOURCE_DIR}/packaging/wix/files.py
        COMMAND ${CMAKE_COMMAND} -E env INKSCAPE_DIST_PATH=${CMAKE_INSTALL_PREFIX}
                    python ${CMAKE_SOURCE_DIR}/packaging/wix/version.py
        COMMAND ${candle} inkscape.wxs -ext WiXUtilExtension
        COMMAND ${candle} files.wxs
        COMMAND ${light} -ext WixUIExtension -ext WiXUtilExtension inkscape.wixobj files.wixobj
                         -o ${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.msi)

    add_dependencies(dist-win-msi install/strip)
    add_dependencies(dist-win-msi-fast install/strip)
endif()
