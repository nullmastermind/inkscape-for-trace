# -----------------------------------------------------------------------------
# Set platform defaults (originally copied from darktable)
# -----------------------------------------------------------------------------
if(WIN32)
  message("-- Windows build detected, setting default features")

  include(CMakeScripts/ConfigEnvMinGW.cmake)

  set(CMAKE_C_COMPILER "${MINGW_BIN}/gcc.exe")
  set(CMAKE_CXX_COMPILER "${MINGW_BIN}/g++.exe")

  # Setup Windows resource files compiler.
  set(CMAKE_RC_COMPILER "${MINGW_BIN}/windres.exe")
  set(CMAKE_RC_COMPILER_INIT windres)
  enable_language(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>")

  # These options are required for having i18n support on Windows.
  option(ENABLE_NLS "Compile with Native Language Support (using gettext)" ON)

  # Does not compile on Windows with these options.
  option(ENABLE_BINRELOC "Compile with binary relocation support" OFF)
  option(WITH_JEMALLOC "Compile with JEMALLOC support" OFF)
endif()

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
endif()
