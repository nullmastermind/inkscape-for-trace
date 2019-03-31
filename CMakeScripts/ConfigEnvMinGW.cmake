# -----------------------------------------------------------------------------
# Set the paths in this file if you want to build Inkscape from a shell other than the
# Windows built-in command line (i.e. MSYS) or IDEs such as CodeLite. These variables
# will be used as default if no environment variables are set.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# MinGW Configuration
# -----------------------------------------------------------------------------
message(STATUS "Configuring MinGW environment:")

if("$ENV{MSYSTEM_CHOST}" STREQUAL "")
  message(WARNING "  Could not detect MinGW build environment. We recommend building with MSYS2. Proceed at your own risk!")
else()
  message(STATUS "  Detected MinGW environment provided by MSYS2")
  set(MINGW_PATH $ENV{MINGW_PREFIX})
endif()

# -----------------------------------------------------------------------------
# MINGW CHECKS
# -----------------------------------------------------------------------------

# Try to determine the MinGW processor architecture.
if(EXISTS "${MINGW_PATH}/i686-w64-mingw32")
  set(HAVE_MINGW64 OFF)
  set(MINGW_ARCH i686-w64-mingw32)
elseif(EXISTS "${MINGW_PATH}/x86_64-w64-mingw32")
  set(HAVE_MINGW64 ON)
  set(MINGW_ARCH x86_64-w64-mingw32)
else()
  message(FATAL_ERROR "Unable to determine MinGW processor architecture. Are you using an unsupported MinGW version?")
endif()

# Path to processor architecture specific binaries and libs.
set(MINGW_ARCH_PATH "${MINGW_PATH}/${MINGW_ARCH}")

set(MINGW_BIN "${MINGW_PATH}/bin")

if(NOT EXISTS ${MINGW_BIN})
  message(FATAL_ERROR "MinGW binary directory does not exist: ${MINGW_BIN}")
endif()

# Eliminate error messages when not having mingw in the environment path variable.
list(APPEND CMAKE_PROGRAM_PATH  ${MINGW_BIN})

set(MINGW_LIB "${MINGW_PATH}/lib")

if(NOT EXISTS ${MINGW_LIB})
  message(FATAL_ERROR "MinGW library directory does not exist: ${MINGW_LIB}")
endif()

# Add MinGW libraries to linker path.
link_directories(${MINGW_LIB})

set(MINGW_INCLUDE "${MINGW_PATH}/include")

if(NOT EXISTS ${MINGW_INCLUDE})
  message(FATAL_ERROR "MinGW include directory does not exist: ${MINGW_INCLUDE}")
endif()

# Add MinGW headers to compiler include path.
include_directories(SYSTEM ${MINGW_INCLUDE})

set(MINGW_ARCH_LIB "${MINGW_ARCH_PATH}/lib")

if(NOT EXISTS ${MINGW_ARCH_LIB})
  message(FATAL_ERROR "MinGW-w64 toolchain libraries directory does not exist: ${MINGW_ARCH_LIB}")
endif()

# Add MinGW toolchain libraries to linker path.
link_directories(${MINGW_ARCH_LIB})

set(MINGW_ARCH_INCLUDE "${MINGW_ARCH_PATH}/include")

if(NOT EXISTS ${MINGW_ARCH_INCLUDE})
  message(FATAL_ERROR "MinGW-w64 toolchain include directory does not exist: ${MINGW_ARCH_INCLUDE}")
endif()

# Add MinGW toolchain headers to compiler include path.
include_directories(${MINGW_ARCH_INCLUDE})

# -----------------------------------------------------------------------------
# MSYS CHECKS
# -----------------------------------------------------------------------------

# Somehow the MSYS variable does not work as documented..
if("${CMAKE_GENERATOR}" STREQUAL "MSYS Makefiles")
  set(HAVE_MSYS ON)
else()
  set(HAVE_MSYS OFF)
endif()

# Set the path to the 'ar' utility for the MSYS shell as it fails to detect it properly.
if(HAVE_MSYS)
  message(STATUS "Configuring MSYS environment:")

  if(NOT EXISTS ${CMAKE_AR})
	set(MINGW_AR ${MINGW_BIN}/ar.exe)

	if(EXISTS ${MINGW_AR})
		set(CMAKE_AR ${MINGW_AR} CACHE FILEPATH "Archive Utility")
	else()
		message(FATAL_ERROR "ar.exe not found.")
	endif()

	message(STATUS "  Setting path to ar.exe: ${CMAKE_AR}")
  endif()
endif()

# -----------------------------------------------------------------------------
# LIBRARY AND LINKER SETTINGS
# -----------------------------------------------------------------------------

# Tweak CMake into using Unix-style library names.
set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".dll")
