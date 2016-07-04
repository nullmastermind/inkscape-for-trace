message(STATUS "Creating build files in: ${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
# Directory containing the precompiled Inkscape libraries. Usually c:\devlibs or c:\devlibs64
	set(DEVLIBS_PATH C:/devlibs64)

	# Directory containing the MinGW instance used for compilation. Usually c:\mingw or c:\mingw64
	set(MINGW_PATH C:/mingw64)

	# Directory containing the Ghostscript installation.
	set(GS_PATH C:/latex/gs/gs8.61)
		
	# Normalize directory separator slashes.
	string(REGEX REPLACE "\\\\" "/" DEVLIBS_PATH ${DEVLIBS_PATH})
	string(REGEX REPLACE "\\\\" "/" MINGW_PATH ${MINGW_PATH})
	string(REGEX REPLACE "\\\\" "/" GS_PATH ${GS_PATH})
	
	# =============== DEVLIBS CHECKS ===============
	
	# Directory containing the compile time .dll.a and .a files.
	set(DEVLIBS_LIB "${DEVLIBS_PATH}/lib")
	
	if(NOT EXISTS "${DEVLIBS_LIB}")
		message(FATAL_ERROR "Inkscape development libraries directory does not exist: ${DEVLIBS_LIB}")
	endif()
	
	# Add devlibs libraries to linker path.
	link_directories(${DEVLIBS_LIB})
	
	set(DEVLIBS_INCLUDE ${DEVLIBS_PATH}/include)
	
	if(NOT EXISTS ${DEVLIBS_INCLUDE})
		message(FATAL_ERROR "Inkscape development libraries directory does not exist: ${DEVLIBS_INCLUDE}")
	endif()
	
	# Add general MinGW headers to compiler include path.
	#include_directories(${DEVLIBS_INCLUDE})
	
	# Directory containing the precompiled .dll files.
	set(DEVLIBS_BIN ${DEVLIBS_PATH}/bin)
	
	if(NOT EXISTS ${DEVLIBS_BIN})
		message(FATAL_ERROR "Inkscape development binaries directory does not exist: ${DEVLIBS_BIN}")
	endif()
	
	# Directory containing the pkgconfig .pc files.
	set(PKG_CONFIG_PATH "${DEVLIBS_PATH}/lib/pkgconfig")
	
	if(NOT EXISTS "${PKG_CONFIG_PATH}")
		message(FATAL_ERROR "pkgconfig directory does not exist: ${PKG_CONFIG_PATH}")
	endif()
	
	# Add the devlibs directories to the paths used to find libraries and programs.
	list(APPEND CMAKE_PREFIX_PATH ${DEVLIBS_PATH})
	
	# Eliminate error messages when not having mingw in the environment path variable.
	list(APPEND CMAKE_PROGRAM_PATH  ${DEVLIBS_BIN})
	
	# =============== MINGW CHECKS ===============
	
	# We are in a MinGW environment.
	set(HAVE_MINGW ON)
		
	# Try to determine the MinGW processor architecture.
	if(EXISTS ${MINGW_PATH}/mingw32)
		set(HAVE_MINGW64 OFF)
		set(MINGW_ARCH mingw32)
	elseif(EXISTS ${MINGW_PATH}/x86_64-w64-mingw32)
		set(HAVE_MINGW64 ON)
		set(MINGW_ARCH x86_64-w64-mingw32)
	else()
		message(FATAL_ERROR "Unable to determine MinGW processor architecture. Are you using an unsupported MinGW version?")
	endif()
	
	# Path to processor architecture specific binaries and libs.
	set(MINGW_ARCH_PATH ${MINGW_PATH}/${MINGW_ARCH})
	
	set(MINGW_BIN ${MINGW_PATH}/bin)
	
	if(NOT EXISTS ${MINGW_BIN})
		message(FATAL_ERROR "MinGW binary directory does not exist: ${MINGW_BIN}")
	endif()
	
	# Eliminate error messages when not having mingw in the environment path variable.
	list(APPEND CMAKE_PROGRAM_PATH  ${MINGW_BIN})
	
	set(MINGW_LIB ${MINGW_PATH}/lib)
	
	if(NOT EXISTS ${MINGW_LIB})
		message(FATAL_ERROR "MinGW library directory does not exist: ${MINGW_LIB}")
	endif()
	
	# Add MinGW libraries to linker path.
	link_directories(${MINGW_LIB})
	
	set(MINGW_INCLUDE ${MINGW_PATH}/include)
	
	if(NOT EXISTS ${MINGW_INCLUDE})
		message(FATAL_ERROR "MinGW include directory does not exist: ${MINGW_INCLUDE}")
	endif()
	
	# Add general MinGW headers to compiler include path.
	include_directories(${MINGW_INCLUDE})
	
	if(HAVE_MINGW64)
		set(MINGW64_LIB ${MINGW_ARCH_PATH}/lib)
		
		if(NOT EXISTS ${MINGW64_LIB})
			message(FATAL_ERROR "MinGW 64-Bit libraries directory does not exist: ${MINGW64_LIB}")
		endif()
	
		# Add 64-Bit libraries to linker path.
		link_directories(${MINGW64_LIB})
		
		set(MINGW64_INCLUDE ${MINGW_ARCH_PATH}/include)
		
		if(NOT EXISTS ${MINGW64_INCLUDE})
			message(FATAL_ERROR "MinGW 64-Bit include directory does not exist: ${MINGW64_INCLUDE}")
		endif()
		
		set(MINGW64_INCLUDE_SDL ${MINGW64_INCLUDE}/c++)
		
		if(NOT EXISTS ${MINGW64_INCLUDE_SDL})
			message(FATAL_ERROR "MinGW 64-Bit SDL include directory does not exist: ${MINGW64_INCLUDE_SDL}")
		endif()

		# Add 64-Bit MinGW headers to compiler include path.
		include_directories(${MINGW64_INCLUDE})
		#include_directories(${MINGW64_INCLUDE_SDL})
	endif()
	
	# =============== GHOSTSCRIPT CHECKS ===============
	
	# Check for Ghostscript.
	set(GS_BIN "${GS_PATH}/bin")
	
	if(EXISTS "${GS_BIN}")
		set(HAVE_GS_BIN ON)
	else()
		set(HAVE_GS_BIN OFF)
	endif()

	# =============== LIBRARY AND LINKER ===============
	
	# Tweak CMake into using Unix-style library names.
	set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".dll")
	
	if(NOT HAVE_MINGW64)
		list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	endif()
	
	# Directory containing the current locale (translations).
    set(PACKAGE_LOCALE_DIR "${CMAKE_INSTALL_PREFIX}/share/locale")
    set(SHARE_INSTALL "share" CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
else(WIN32)
    # TODO: check and change this to correct value:
    if(NOT PACKAGE_LOCALE_DIR)
	set(PACKAGE_LOCALE_DIR "${CMAKE_INSTALL_PREFIX}/share/locale") # packagers might overwrite this
    endif(NOT PACKAGE_LOCALE_DIR)

    if(NOT SHARE_INSTALL)
	set(SHARE_INSTALL "share" CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
    endif(NOT SHARE_INSTALL)
    mark_as_advanced(SHARE_INSTALL)
endif(WIN32)
