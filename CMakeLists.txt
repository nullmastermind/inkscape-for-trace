cmake_minimum_required(VERSION 2.8.0)

message("------------------------------")
message("Building Makefile for Inkscape")
message("------------------------------")
message("Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message("Binary Dir: ${CMAKE_CURRENT_BINARY_DIR}")

# -----------------------------------------------------------------------------
# Set platform defaults (originally copied from darktable)
# -----------------------------------------------------------------------------
include(CMakeScripts/ConfigPaths.cmake)

if(WIN32)
    message("-- Windows build detected, setting default features")

	list(INSERT CMAKE_SYSTEM_INCLUDE_PATH 0 ${DEVLIBS_PATH})
	list(INSERT CMAKE_SYSTEM_LIBRARY_PATH 0 ${DEVLIBS_PATH})
	
	set(CMAKE_C_COMPILER "${MINGW_BIN}/gcc.exe")
	set(CMAKE_C_LINK_EXECUTABLE "${MINGW_BIN}/gcc.exe")
	set(CMAKE_CXX_COMPILER "${MINGW_BIN}/g++.exe")
	set(CMAKE_CXX_LINK_EXECUTABLE "${MINGW_BIN}/g++.exe")
	set(CMAKE_CXX_STANDARD 11)
	
	# Setup Windows resource files compiler.
	set(CMAKE_RC_COMPILER "${MINGW_BIN}/windres.exe")
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>")
	
	# These options are required for having i18n support on Windows.
	option(ENABLE_NLS "Compile with Native Language Support (using gettext)" ON)
	option(HAVE_BIND_TEXTDOMAIN_CODESET "Compile with 'bind_textdomain_codeset' function" ON)
	
	# Does not compile on Windows with these options.
	option(BR_PTHREADS "Use binreloc thread support" OFF)
	option(ENABLE_BINRELOC "Use AutoPackage" OFF)
endif(WIN32)

if(APPLE)
    message("-- Mac OS X build detected, setting default features")
	
    # prefer macports and/or user-installed libraries over system ones
    #LIST(APPEND CMAKE_PREFIX_PATH /opt/local /usr/local)
    set(CMAKE_FIND_FRAMEWORK "LAST")

    # test and display relevant env variables
    if(DEFINED ENV{CMAKE_PREFIX_PATH})
	message("CMAKE_PREFIX_PATH: $ENV{CMAKE_PREFIX_PATH}")
    endif()
	
    if(DEFINED ENV{GTKMM_BASEPATH})
	message("GTKMM_BASEPATH: $ENV{GTKMM_BASEPATH}")
    endif()

    # detect current GTK+ backend
    pkg_check_variable(gtk+-2.0 target)
    message("GTK2 backend: ${GTK+_2.0_TARGET}")
endif(APPLE)

# -----------------------------------------------------------------------------
# CMake Configuration
# -----------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeScripts/Modules")

# avoid having empty buildtype
set(CMAKE_BUILD_TYPE_INIT "Release")

project(inkscape)

set(INKSCAPE_VERSION 0.92pre1)
set(PROJECT_NAME inkscape)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if(APPLE)
    SET(CMAKE_MACOSX_RPATH TRUE)
    SET(CMAKE_INSTALL_RPATH "@loader_path/../lib/inkscape")
else()
    SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib/inkscape")
endif()


cmake_policy(SET CMP0003 NEW) # don't be prolific with library paths
cmake_policy(SET CMP0005 NEW) # proper define quoting
cmake_policy(SET CMP0009 NEW) # don't follow symbolic links when using GLOB

# workaround for omission in cmake 2.8.4's GNU.cmake, fixed in 2.8.5
if(CMAKE_COMPILER_IS_GNUCC)
    if(NOT DARWIN)
	set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ")
    endif()
endif()

include(CMakeScripts/HelperFunctions.cmake)

# -----------------------------------------------------------------------------
# Redirect output files
# -----------------------------------------------------------------------------
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" )
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" )

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(WITH_DBUS "Compile with support for DBus interface" OFF)
option(ENABLE_LCMS "Compile with LCMS support" ON)
option(WITH_GNOME_VFS "Compile with support for Gnome VFS" ON)
option(WITH_SVG2 "Compile with support for new SVG2 features" ON)
option(WITH_LPETOOL "Compile with LPE Tool and experimental LPEs enabled" ON)
#option(WITH_INKJAR "Enable support for openoffice files (SVG jars)" ON)
option(WITH_OPENMP "Compile with OpenMP support" ON)

option(WITH_PROFILING "Turn on profiling" OFF) # Set to true if compiler/linker should enable profiling

option(ENABLE_POPPLER "Compile with support of libpoppler" ON)
option(ENABLE_POPPLER_CAIRO "Compile with support of libpoppler-cairo for rendering PDF preview (depends on ENABLE_POPPLER)" ON)
option(WITH_IMAGE_MAGICK "Compile with support of ImageMagick for raster extensions and image import resolution" ON)
option(WITH_LIBCDR "Compile with support of libcdr for CorelDRAW Diagrams" ON)
option(WITH_LIBVISIO "Compile with support of libvisio for Microsoft Visio Diagrams" ON)
option(WITH_LIBWPG "Compile with support of libwpg for WordPerfect Graphics" ON)
option(WITH_NLS "Compile with Native Language Support (using gettext)" ON)
option(WITH_GTK3_EXPERIMENTAL "Enable compilation with GTK+3 (EXPERIMENTAL!)" OFF)

# -----------------------------------------------------------------------------
# Test Harness
# -----------------------------------------------------------------------------
set(GMOCK_DIR "${CMAKE_SOURCE_DIR}/gtest/gmock-1.7.0"
    CACHE PATH "The path to the GoogleMock test framework.")

if(EXISTS "${GMOCK_DIR}" AND IS_DIRECTORY "${GMOCK_DIR}")
    set(GMOCK_PRESENT ON)
else()
    set(GMOCK_PRESENT OFF)
    message("No gmock/gtest found! Perhaps you wish to run 'bash download-gtest.sh' to download it.")
endif()

include(CMakeScripts/DefineDependsandFlags.cmake) # Includes, Compiler Flags, and Link Libraries
include(CMakeScripts/HelperMacros.cmake) # Misc Utility Macros


# -----------------------------------------------------------------------------
# BAD HACKS, NEED TO INVESTIGATE MAKING THESE LESS BAD
add_definitions(-D_FORTIFY_SOURCE=2)
add_definitions(-DORBIT2=1)
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DHAVE_CAIRO_PDF=1)  # needed for src/libnrtype/Layout-TNG.h
add_definitions(-DHAVE_TR1_UNORDERED_SET) # XXX make an option!
if(NOT WIN32)
    add_definitions(-fPIC)
endif()
#
# end badness
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Dist Target
# -----------------------------------------------------------------------------
set(INKSCAPE_DIST_PREFIX "${PROJECT_NAME}-${INKSCAPE_VERSION}")
add_custom_target(dist
    COMMAND bzr export --root=${INKSCAPE_DIST_PREFIX}
    "${CMAKE_BINARY_DIR}/${INKSCAPE_DIST_PREFIX}.tar.bz2"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# -----------------------------------------------------------------------------
# Uninstall Target
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


# -----------------------------------------------------------------------------
# Subdirs (on only 1 atm), but a lot happens here
# -----------------------------------------------------------------------------
add_subdirectory(src)
if(ENABLE_NLS)
    add_subdirectory(po)
endif(ENABLE_NLS)


# -----------------------------------------------------------------------------
# Man pages
# -----------------------------------------------------------------------------
include("CMakeScripts/Pod2man.cmake")

# Load AUTHORS file contents into $INKSCAPE_AUTHORS
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS content)
string(REGEX REPLACE "^([^\n#]+)\n" "\\1;\n" content_list "${content}")
join(INKSCAPE_AUTHORS "," "${content_list}")

foreach(podfile
        man/inkscape.pod
        man/inkscape.de.pod
        man/inkscape.el.pod
        man/inkscape.fr.pod
        man/inkscape.ja.pod
        man/inkscape.sk.pod
        man/inkscape.zh_TW.pod
	man/inkview.pod)
    set(POD_IN ${CMAKE_CURRENT_SOURCE_DIR}/${podfile}.in)
    set(POD_OUT ${CMAKE_CURRENT_BINARY_DIR}/${podfile})
    configure_file(${POD_IN} ${POD_OUT})
    pod2man(${POD_OUT} ${INKSCAPE_VERSION} 1 "Inkscape Commands Manual")
endforeach()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------
if(UNIX)
	#Define the installation
	install(
	  TARGETS inkscape_base inkscape inkview 
	  RUNTIME DESTINATION bin
	  LIBRARY DESTINATION lib/inkscape
	  ARCHIVE DESTINATION lib/inkscape
	)
	
    #The install directive for the binaries and libraries are found in src/CMakeList.txt
    install(
	FILES ${CMAKE_BINARY_DIR}/inkscape.desktop
	DESTINATION ${CMAKE_INSTALL_PREFIX}/${SHARE_INSTALL}/applications
	)

    # this should probably be done no matter what the platform is, just set SHARE_INSTALL first
    add_subdirectory(share) 
elseif(WIN32)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/inkscape)
	
    install(PROGRAMS
		${EXECUTABLE_OUTPUT_PATH}/inkscape.exe
		${EXECUTABLE_OUTPUT_PATH}/inkview.exe
        DESTINATION ${CMAKE_INSTALL_PREFIX}
	)
	
    install(FILES
		${LIBRARY_OUTPUT_PATH}/libinkscape_base.dll
		${LIBRARY_OUTPUT_PATH}/libgrid2.dll
        DESTINATION ${CMAKE_INSTALL_PREFIX}
	)
		
    # devlibs and mingw dlls
    install(FILES
		AUTHORS
		COPYING
		NEWS
		README
		TRANSLATORS
		${DEVLIBS_BIN}/libatkmm-1.6-1.dll
		${DEVLIBS_BIN}/libglibmm-2.4-1.dll
		${DEVLIBS_BIN}/libgiomm-2.4-1.dll
		${DEVLIBS_BIN}/libgtkmm-2.4-1.dll
		${DEVLIBS_BIN}/libgdkmm-2.4-1.dll
		${DEVLIBS_BIN}/libpangomm-1.4-1.dll
		${DEVLIBS_BIN}/libcairomm-1.0-1.dll
		${DEVLIBS_BIN}/libsigc-2.0-0.dll
		${DEVLIBS_BIN}/libfreetype-6.dll
		${DEVLIBS_BIN}/libffi-6.dll
		${DEVLIBS_BIN}/libgc-1.dll
		${DEVLIBS_BIN}/libgsl-19.dll
		${DEVLIBS_BIN}/libgslcblas-0.dll
		${DEVLIBS_BIN}/libharfbuzz-0.dll
		${DEVLIBS_BIN}/liblzma-5.dll
		${DEVLIBS_BIN}/libaspell-15.dll
		${DEVLIBS_BIN}/libatk-1.0-0.dll
		${DEVLIBS_BIN}/libgdk-win32-2.0-0.dll
		${DEVLIBS_BIN}/libgdk_pixbuf-2.0-0.dll
		${DEVLIBS_BIN}/libglib-2.0-0.dll
		${DEVLIBS_BIN}/libgmodule-2.0-0.dll
		${DEVLIBS_BIN}/libgobject-2.0-0.dll
		${DEVLIBS_BIN}/libgtk-win32-2.0-0.dll
		${DEVLIBS_BIN}/libgthread-2.0-0.dll
		${DEVLIBS_BIN}/libgio-2.0-0.dll
		${DEVLIBS_BIN}/libcairo-2.dll
		${DEVLIBS_BIN}/libpixman-1-0.dll
		${DEVLIBS_BIN}/libpoppler-58.dll
		${DEVLIBS_BIN}/libpoppler-glib-8.dll
		${DEVLIBS_BIN}/libpangocairo-1.0-0.dll
		${DEVLIBS_BIN}/libpango-1.0-0.dll
		${DEVLIBS_BIN}/libpangoft2-1.0-0.dll
		${DEVLIBS_BIN}/libpangowin32-1.0-0.dll
		${DEVLIBS_BIN}/libfontconfig-1.dll
		${DEVLIBS_BIN}/libxml2-2.dll
		${DEVLIBS_BIN}/libxslt-1.dll
		${DEVLIBS_BIN}/libexslt-0.dll
		${DEVLIBS_BIN}/libexpat-1.dll
		${DEVLIBS_BIN}/librevenge-0.0.dll
		${DEVLIBS_BIN}/librevenge-stream-0.0.dll
		${DEVLIBS_BIN}/libwpg-0.3.dll
		${DEVLIBS_BIN}/libwpd-0.10.dll
		${DEVLIBS_BIN}/libvisio-0.1.dll
		${DEVLIBS_BIN}/libcdr-0.1.dll
		${DEVLIBS_BIN}/icuin56.dll
		${DEVLIBS_BIN}/icudt56.dll
		${DEVLIBS_BIN}/icuuc56.dll
		${DEVLIBS_BIN}/libjpeg-9.dll
		${DEVLIBS_BIN}/libpng16-16.dll
		${DEVLIBS_BIN}/libtiff-5.dll
		${DEVLIBS_BIN}/libexif-12.dll
		${DEVLIBS_BIN}/libcurl-4.dll
		${DEVLIBS_BIN}/zlib1.dll
		${DEVLIBS_BIN}/bz2-1.dll
		${DEVLIBS_BIN}/libiconv-2.dll
		${DEVLIBS_BIN}/libpopt-0.dll
		${DEVLIBS_BIN}/liblcms2-2.dll
		${DEVLIBS_BIN}/libMagick++-6.Q16-6.dll
		${DEVLIBS_BIN}/libMagickCore-6.Q16-2.dll
		${DEVLIBS_BIN}/libMagickWand-6.Q16-2.dll
		${DEVLIBS_BIN}/libintl-8.dll
		${DEVLIBS_BIN}/libpotrace-0.dll
		${MINGW_BIN}/libstdc++-6.dll
        ${MINGW_BIN}/libwinpthread-1.dll
        ${MINGW_BIN}/libgcc_s_seh-1.dll
        ${MINGW_BIN}/libgomp-1.dll 
        DESTINATION ${CMAKE_INSTALL_PREFIX})
		
	# Setup application data directories, poppler files, locales, icons and themes
	file(MAKE_DIRECTORY
		data
		doc
		modules
		plugins)
	
	install(DIRECTORY
		data
		doc
		modules
		plugins
		share
		DESTINATION ${CMAKE_INSTALL_PREFIX}
		PATTERN Adwaita EXCLUDE					# NOTE: The theme is not used on Windows.
		PATTERN hicolor/index.theme EXCLUDE 	# NOTE: Empty index.theme in hicolor icon theme causes SIGSEGV.
		PATTERN CMakeLists.txt EXCLUDE
		PATTERN *.am EXCLUDE)
		
	install(DIRECTORY ${DEVLIBS_PATH}/share/themes
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share)
		
	install(DIRECTORY ${DEVLIBS_PATH}/share/poppler
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share)
		
	install(DIRECTORY ${DEVLIBS_PATH}/etc/fonts
		DESTINATION ${CMAKE_INSTALL_PREFIX}/etc)

	install(DIRECTORY ${DEVLIBS_PATH}/share/locale
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share)
		
	install(DIRECTORY ${DEVLIBS_PATH}/etc/gtk-2.0
		DESTINATION ${CMAKE_INSTALL_PREFIX}/etc)
		
	# GTK 2.0
	install(DIRECTORY ${DEVLIBS_LIB}/gtk-2.0
		DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
		FILES_MATCHING
		PATTERN "*.dll"
		PATTERN "*.cache")
	
	install(DIRECTORY ${DEVLIBS_LIB}/gdk-pixbuf-2.0
		DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
		FILES_MATCHING
		PATTERN "*.dll"
		PATTERN "*.cache")
		
	# Aspell dictionaries
	install(DIRECTORY ${DEVLIBS_LIB}/aspell-0.60
		DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
		
	# Necessary to run extensions on windows if it is not in the path
	if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
		install(FILES
			${DEVLIBS_BIN}/gspawn-win64-helper.exe
			${DEVLIBS_BIN}/gspawn-win64-helper-console.exe
			DESTINATION ${CMAKE_INSTALL_PREFIX})
	else()
		install(FILES
			${DEVLIBS_BIN}/gspawn-win32-helper.exe
			${DEVLIBS_BIN}/gspawn-win32-helper-console.exe
			DESTINATION ${CMAKE_INSTALL_PREFIX})
	endif()
		
	# Perl
	install(FILES
		${DEVLIBS_PATH}/perl/bin/perl58.dll
        DESTINATION ${CMAKE_INSTALL_PREFIX})
	
	# Python
	install(FILES
		${DEVLIBS_PATH}/python/python.exe
		${DEVLIBS_PATH}/python/pythonw.exe
		${DEVLIBS_PATH}/python/python27.dll
        DESTINATION ${CMAKE_INSTALL_PREFIX}/python)
		
	install(DIRECTORY ${DEVLIBS_PATH}/python/lib
		DESTINATION ${CMAKE_INSTALL_PREFIX}/python)
		
	install(DIRECTORY ${DEVLIBS_PATH}/python/dlls
		DESTINATION ${CMAKE_INSTALL_PREFIX}/python)
elseif(APPLE)
    # TODO
endif()

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
add_custom_target(clean-cmake-files
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/CleanAll.cmake"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/CMakeScripts/CleanAll.cmake"
)

#-----------------------------------------------------------------------------
if(GMOCK_PRESENT)
    set(CMAKE_CTEST_COMMAND ctest -V)
    enable_testing()
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
    add_subdirectory(testfiles)
endif()

# Canonicalize the flags to speed up recompilation using ccache/etc.
# This should be the last thing we do:
include(CMakeScripts/CanonicalizeFlagsVar.cmake)
canonicalize_flags_var("${CMAKE_CXX_FLAGS}" _new_cxx)
set(CMAKE_CXX_FLAGS "${_new_cxx}" CACHE STRING "" FORCE)
# message(FATAL_ERROR "CMAKE_CXX_FLAGS = <${CMAKE_CXX_FLAGS}>")


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
message("GMOCK_PRESENT:           ${GMOCK_PRESENT}")
message("WITH_DBUS:               ${WITH_DBUS}")
message("WITH_GNOME_VFS:          ${WITH_GNOME_VFS}")
message("WITH_GTK3_EXPERIMENTAL:  ${WITH_GTK3_EXPERIMENTAL}")
message("WITH_GTKSPELL:           ${WITH_GTKSPELL}")
message("WITH_IMAGE_MAGICK:       ${WITH_IMAGE_MAGICK}")
message("WITH_LIBCDR:             ${WITH_LIBCDR}")
message("WITH_LIBVISIO:           ${WITH_LIBVISIO}")
message("WITH_LIBWPG:             ${WITH_LIBWPG}")
message("WITH_NLS:                ${WITH_NLS}")
message("WITH_OPENMP:             ${WITH_OPENMP}")
message("WITH_PROFILING:          ${WITH_PROFILING}")

if(WIN32)
message("")
message("HAVE_MINGW:              ${HAVE_MINGW}")
message("HAVE_MINGW64:            ${HAVE_MINGW64}")
message("MINGW_PATH:              ${MINGW_PATH}")
message("MINGW_ARCH:              ${MINGW_ARCH}")
message("MINGW_ARCH_PATH:         ${MINGW_ARCH_PATH}")
message("MINGW64_INCLUDE:         ${MINGW64_INCLUDE}")
message("MINGW64_LIB:             ${MINGW64_LIB}")
message("DEVLIBS_PATH:            ${DEVLIBS_PATH}")
message("DEVLIBS_LIB:             ${DEVLIBS_LIB}")
message("DEVLIBS_BIN:             ${DEVLIBS_BIN}")
message("PKG_CONFIG_PATH:         ${PKG_CONFIG_PATH}")
message("GS_PATH                  ${GS_PATH}")
message("GS_BIN                   ${GS_BIN}")
endif()

message("------------------------------------------------------------------------")
