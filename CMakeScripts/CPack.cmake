# This file reconfigures variables at CPack time.
#
# It is called by
# https://cmake.org/cmake/help/latest/module/CPack.html#variable:CPACK_PROJECT_CONFIG_FILE
#

#used to get cmake-time variables at cpack-time
set(INKSCAPE_VERSION_SUFFIX @INKSCAPE_VERSION_SUFFIX@)

if (CPACK_GENERATOR STREQUAL "WIX")
  
  # for Wix, version should be a.b.c.d with a,b,c between 0 and 255, and
  # 0<d<65536.
  # We are using the 4th number to store alpha, beta, or rc information

  string(FIND ${INKSCAPE_VERSION_SUFFIX} "alpha" ISALPHA)
  string(FIND ${INKSCAPE_VERSION_SUFFIX} "beta" ISBETA)
  string(FIND ${INKSCAPE_VERSION_SUFFIX} "rc" ISRC)
  string(REGEX REPLACE "[^0-9]" "" WIX_SUFFIX ${INKSCAPE_VERSION_SUFFIX} )

  if(ISALPHA GREATER -1)
    math(EXPR WIX_SUFFIX "${WIX_SUFFIX} + 10")
  elseif(ISBETA GREATER -1)
    math(EXPR WIX_SUFFIX "${WIX_SUFFIX} + 20")
  elseif(ISRC GREATER -1)
    math(EXPR WIX_SUFFIX "${WIX_SUFFIX} + 30")
  else()
    math(EXPR WIX_SUFFIX "${WIX_SUFFIX} + 100")
  endif()

  set(CPACK_PACKAGE_VERSION
    "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}.${WIX_SUFFIX}")
   message("WIX version: ${CPACK_PACKAGE_VERSION}")
endif()
