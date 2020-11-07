# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.1.0)

cmake_policy(SET CMP0003 NEW) # don't be prolific with library paths
cmake_policy(SET CMP0005 NEW) # proper define quoting
cmake_policy(SET CMP0009 NEW) # don't follow symbolic links when using GLOB

if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW) # link check-executable to CMAKE_REQUIRED_LIBRARIES (CMake 3.12.1)
endif(POLICY CMP0075)

message("------------------------------")
message("Building Makefile for Inkscape")
message("------------------------------")
message("Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message("Binary Dir: ${CMAKE_CURRENT_BINARY_DIR}")

# -----------------------------------------------------------------------------
# CMake Configuration
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF) # enforces -std=c++17 instead of -std=gnu++17
                                # TODO: build currently fails with it as we actually depend on GNU compiler extensions...

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeScripts/Modules")

# avoid having empty buildtype
set(CMAKE_BUILD_TYPE_INIT "Release")

include(CMakeScripts/HelperFunctions.cmake)
include(CMakeScripts/ConfigEnv.cmake)

project(inkscape)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME inkscape) # needs to be before any install() commands

include(GNUInstallDirs) # for the CMAKE_INSTALL_LIBDIR variable
include(CMakeScripts/ConfigPaths.cmake)

set(PROJECT_NAME inkscape)

# see https://gitlab.com/inkscape/inkscape/-/issues/551 for versioning suffixes
set(INKSCAPE_VERSION_MAJOR  1)
set(INKSCAPE_VERSION_MINOR  1)
set(INKSCAPE_VERSION_PATCH  0)
set(INKSCAPE_VERSION_SUFFIX "-dev")

set(INKSCAPE_VERSION ${INKSCAPE_VERSION_MAJOR}.${INKSCAPE_VERSION_MINOR})
if(INKSCAPE_VERSION_PATCH)
    set(INKSCAPE_VERSION ${INKSCAPE_VERSION}.${INKSCAPE_VERSION_PATCH})
endif()
if(INKSCAPE_VERSION_SUFFIX)
    set(INKSCAPE_VERSION ${INKSCAPE_VERSION}${INKSCAPE_VERSION_SUFFIX})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

file(RELATIVE_PATH
    INKSCAPE_INSTALL_LIBDIR_RELATIVE_TO_BINDIR
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/inkscape")

if(APPLE)
    SET(CMAKE_MACOSX_RPATH TRUE)
    SET(CMAKE_INSTALL_RPATH "@loader_path/${INKSCAPE_INSTALL_LIBDIR_RELATIVE_TO_BINDIR}")
else()
    SET(CMAKE_INSTALL_RPATH "$ORIGIN/${INKSCAPE_INSTALL_LIBDIR_RELATIVE_TO_BINDIR}")
endif()

# this can be removed if/when cmake 3.1 is made the minimum required version
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

# console output is slow as hell on Windows and as a result status messages of the "install" target slow down
# the whole build process considerably (especially since we also copy a lot of files from the devlibs)
# TODO: Is this worth to be configurable / also applicable to other platforms?
if(WIN32 AND NOT CMAKE_INSTALL_MESSAGE)
    set(CMAKE_INSTALL_MESSAGE "LAZY")
endif()

# Define a very strict set of build flags that will prevent any use of deprecated symbols.
# This will almost certainly cause compilation failure and is intended only for developer use.
set(CMAKE_CXX_FLAGS_STRICT "${CMAKE_CXX_FLAGS_DEBUG} -Werror=deprecated-declarations -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTKMM_DISABLE_DEPRECATED -DGDKMM_DISABLE_DEPRECATED -DGLIBMM_DISABLE_DEPRECATED"
	CACHE STRING
	"Flags used by C++ compiler during Strict builds"
	FORCE)
set(CMAKE_C_FLAGS_STRICT   "${CMAKE_C_FLAGS_DEBUG}   -Werror=deprecated-declarations -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTKMM_DISABLE_DEPRECATED -DGDKMM_DISABLE_DEPRECATED"
	CACHE STRING
	"Flags used by C compiler during Strict builds"
	FORCE)
mark_as_advanced(
	CMAKE_CXX_FLAGS_STRICT
	CMAKE_C_FLAGS_STRICT)

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING
	"Choose the type of build, options are: None(CMAKE_CXXFLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel Strict."
	FORCE)

# -----------------------------------------------------------------------------
# Redirect output files
# -----------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Output directory for runtime binaries")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR} CACHE PATH "Output directory for shared libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR} CACHE PATH "Output directory for static libraries")

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(WITH_GNU_READLINE "Compile with GNU Readline support (shell mode)" ON)
option(WITH_DBUS "Compile with support for DBus interface" OFF)
option(ENABLE_LCMS "Compile with LCMS support" ON)
option(WITH_SVG2 "Compile with support for new SVG2 features" ON)
option(WITH_LPETOOL "Compile with LPE Tool" OFF)
option(LPE_ENABLE_TEST_EFFECTS "Compile with test experimental LPEs enabled" OFF)
option(WITH_OPENMP "Compile with OpenMP support" ON)
option(WITH_PROFILING "Turn on profiling" OFF) # Set to true if compiler/linker should enable profiling
option(BUILD_SHARED_LIBS "Compile libraries as shared and not static" ON)

option(ENABLE_POPPLER "Compile with support of libpoppler" ON)
option(ENABLE_POPPLER_CAIRO "Compile with support of libpoppler-cairo for rendering PDF preview (depends on ENABLE_POPPLER)" ON)
option(WITH_IMAGE_MAGICK "Compile with support of ImageMagick for raster extensions and image import resolution (requires ImageMagick 6; set to OFF if you prefer GraphicsMagick)" ON)
option(WITH_GRAPHICS_MAGICK "Compile with support of GraphicsMagick for raster extensions and image import resolution" ON)
option(WITH_LIBCDR "Compile with support of libcdr for CorelDRAW Diagrams" ON)
option(WITH_LIBVISIO "Compile with support of libvisio for Microsoft Visio Diagrams" ON)
option(WITH_LIBWPG "Compile with support of libwpg for WordPerfect Graphics" ON)
option(WITH_GSPELL "Compile with support of gspell" ON)
option(WITH_NLS "Compile with Native Language Support (using gettext)" ON)
option(WITH_JEMALLOC "Compile with JEMALLOC support" OFF)
option(WITH_ASAN "Compile with Clang's AddressSanitizer (for debugging purposes)" OFF)
option(WITH_INTERNAL_2GEOM "Prefer internal copy of lib2geom" OFF)

option(WITH_FUZZ "Compile for fuzzing purpose (use 'make fuzz' only)" OFF)
mark_as_advanced(WITH_FUZZ)
option(WITH_MANPAGE_COMPRESSION "gzips manpages if gzip is available" ON)
mark_as_advanced(WITH_MANPAGE_COMPRESSION)

option(ENABLE_BINRELOC "Enable relocatable binaries" OFF)


include(CMakeScripts/DefineDependsandFlags.cmake) # Includes, Compiler Flags, and Link Libraries
include(CMakeScripts/HelperMacros.cmake) # Misc Utility Macros

# -----------------------------------------------------------------------------
# BAD HACKS, NEED TO INVESTIGATE MAKING THESE LESS BAD
if(BUILD_SHARED_LIBS AND NOT WIN32)
    add_definitions(-fPIC)
endif()
#
# end badness
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(src)

if(ENABLE_NLS)
    add_subdirectory(po)
endif(ENABLE_NLS)

if(NOT WIN32)
    include(CMakeScripts/Pod2man.cmake)
    add_subdirectory(man)
endif()



# -----------------------------------------------------------------------------
# Check License Headers
# -----------------------------------------------------------------------------
add_custom_target(check-license-headers WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ./buildtools/check_license_headers.py)


# -----------------------------------------------------------------------------
# Test Harness
# -----------------------------------------------------------------------------

if(BUILD_TESTING OR NOT DEFINED BUILD_TESTING)
    find_package(GTest)
    if(GTEST_FOUND)
        set(DART_TESTING_TIMEOUT 180 CACHE STRING "Test timeout") # if running for > 180 s something must be wrong)
        include(CTest)
        add_subdirectory(testfiles EXCLUDE_FROM_ALL)
    else()
        set(BUILD_TESTING OFF)
        message(WARNING "No gtest found! Disabling testing...\n"
                        "Consider installing it via package manager (install 'libgtest-dev' or equivalent).")
    endif()
endif()


# -----------------------------------------------------------------------------
# Clean Targets
# -----------------------------------------------------------------------------
add_custom_target(clean-cmake-files
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/CleanAll.cmake"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/CleanAll.cmake"
)

# -----------------------------------------------------------------------------
# Install Target
# -----------------------------------------------------------------------------
add_subdirectory(share)

if(WIN32)
  include(CMakeScripts/InstallMSYS2.cmake)
endif()

# -----------------------------------------------------------------------------
# Uninstall Target
# -----------------------------------------------------------------------------
if(WIN32)
    add_custom_target(uninstall
        "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_INSTALL_PREFIX}")
else()
    configure_file(
        "${CMAKE_SOURCE_DIR}/CMakeScripts/cmake_uninstall.cmake.in"
        "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
        @ONLY)
    add_custom_target(uninstall
        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

# -----------------------------------------------------------------------------
# Dist Targets
# -----------------------------------------------------------------------------
include(CMakeScripts/Dist.cmake)

# -----------------------------------------------------------------------------
# Packaging (CPack)
# -----------------------------------------------------------------------------
include(CMakeScripts/ConfigCPack.cmake)



# ----------------------------------------------------------------------
# Information Summary
# ----------------------------------------------------------------------
message("------------------------------------------------------------------------")
message("Configuration Summary")
message("------------------------------------------------------------------------")
# project info
message("PROJECT_NAME:            ${PROJECT_NAME}")
message("INKSCAPE_VERSION:        ${INKSCAPE_VERSION}")
message("INKSCAPE_DIST_PREFIX:    ${INKSCAPE_DIST_PREFIX}")
message("")

# cmake info
message("CMAKE_BINARY_DIR:        ${CMAKE_BINARY_DIR}")
message("CMAKE_INSTALL_PREFIX:    ${CMAKE_INSTALL_PREFIX}")
message("CMAKE_INSTALL_LIBDIR:    ${CMAKE_INSTALL_LIBDIR}")
message("PACKAGE_LOCALE_DIR       ${PACKAGE_LOCALE_DIR}")
message("CMAKE_SYSTEM_NAME:       ${CMAKE_SYSTEM_NAME}")
message("CMAKE_SYSTEM_VERSION:    ${CMAKE_SYSTEM_VERSION}")
message("CMAKE_SYSTEM_PROCESSOR:  ${CMAKE_SYSTEM_PROCESSOR}")
message("CMAKE_C_COMPILER:        ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER:      ${CMAKE_CXX_COMPILER}")
message("CMAKE_BUILD_TYPE:        ${CMAKE_BUILD_TYPE}")
message("")

if(WIN32)
message("CMAKE_PREFIX_PATH:           ${CMAKE_PREFIX_PATH}")
message("CMAKE_FIND_LIBRARY_PREFIXES: ${CMAKE_FIND_LIBRARY_PREFIXES}")
message("CMAKE_FIND_LIBRARY_SUFFIXES: ${CMAKE_FIND_LIBRARY_SUFFIXES}")
message("")
endif()

# dependency info
message("ENABLE_LCMS:             ${ENABLE_LCMS}")
message("ENABLE_POPPLER:          ${ENABLE_POPPLER}")
message("ENABLE_POPPLER_CAIRO:    ${ENABLE_POPPLER_CAIRO}")
message("WITH_GNU_READLINE:       ${WITH_GNU_READLINE}")
message("WITH_DBUS:               ${WITH_DBUS}")
message("WITH_GSPELL:             ${WITH_GSPELL}")
message("WITH_IMAGE_MAGICK:       ${WITH_IMAGE_MAGICK}")
message("WITH_GRAPHICS_MAGICK:    ${WITH_GRAPHICS_MAGICK}")
message("WITH_LIBCDR:             ${WITH_LIBCDR}")
message("WITH_LIBVISIO:           ${WITH_LIBVISIO}")
message("WITH_LIBWPG:             ${WITH_LIBWPG}")
message("WITH_NLS:                ${WITH_NLS}")
message("WITH_OPENMP:             ${WITH_OPENMP}")
message("WITH_JEMALLOC:           ${WITH_JEMALLOC}")
message("WITH_INTERNAL_2GEOM:     ${WITH_INTERNAL_2GEOM}")

message("WITH_PROFILING:          ${WITH_PROFILING}")
message("BUILD_TESTING:           ${BUILD_TESTING}")

if(WIN32)
message("")
message("HAVE_MINGW64:            ${HAVE_MINGW64}")
message("MINGW_PATH:              ${MINGW_PATH}")
message("MINGW_ARCH:              ${MINGW_ARCH}")
message("MINGW_ARCH_PATH:         ${MINGW_ARCH_PATH}")
endif()

message("------------------------------------------------------------------------")
