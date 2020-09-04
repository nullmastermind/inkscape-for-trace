message(STATUS "Creating build files in: ${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
  if(${CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT})
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/inkscape"
        CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
  endif()
  
  set(INKSCAPE_DATADIR "") # can be set via the environment variable INKSCAPE_DATADIR at runtime
else()
  if(NOT INKSCAPE_DATADIR)
    set(INKSCAPE_DATADIR "${CMAKE_INSTALL_PREFIX}/share")
  endif(NOT INKSCAPE_DATADIR)
endif()

if(NOT PACKAGE_LOCALE_DIR)
  set(PACKAGE_LOCALE_DIR "share/locale") # packagers might overwrite this
endif(NOT PACKAGE_LOCALE_DIR)

if(NOT SHARE_INSTALL)
  set(SHARE_INSTALL "share" CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
endif(NOT SHARE_INSTALL)
set(INKSCAPE_SHARE_INSTALL "${SHARE_INSTALL}/inkscape")
