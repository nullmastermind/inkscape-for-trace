# - Try to find GNU readline
#
# Readline_FOUND - system has readline
# Readline_INCLUDE_DIRS - the readline include directories
# Readline_LIBRARIES - link these to use readline

find_path(Readline_ROOT_DIR
  NAMES include/readline/readline.h
)

find_path(Readline_INCLUDE_DIR
  NAMES readline/readline.h
  HINTS ${ReadlineROOT_DIR}/include
)

find_library(Readline_LIBRARY
  NAMES readline
  HINTS ${Readline_ROOT_DIR}/lib
)

if(Readline_INCLUDE_DIR AND Readline_LIBRARY)
  set (Readline_FOUND TRUE)
else()
  FIND_LIBRARY(Readline_LIBRARY NAMES readline)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline
    FOUND_VAR Readline_FOUND
    REQUIRED_VARS Readline_INCLUDE_DIR
    FAIL_MESSAGE "Failed to find GNU Readline")
endif()

mark_as_advanced(
  Readline_ROOT_DIR
  Readline_LIBRARY
  Readline_INCLUDE_DIR
)

