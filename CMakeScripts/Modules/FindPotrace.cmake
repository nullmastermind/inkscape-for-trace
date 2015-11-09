#  POTRACE_FOUND - system has Potrace
#  POTRACE_INCLUDE_DIR - the Potrace include directory
#  POTRACE_LIBRARIES - The libraries needed to use Potrace

FIND_PATH(POTRACE_INCLUDE_DIR potracelib.h
   /usr/include
   /usr/local/include
)

FIND_LIBRARY(POTRACE_LIBRARY NAMES  potrace libpotrace
   PATHS
   /usr/lib
   /usr/local/lib
)

if (POTRACE_INCLUDE_DIR AND POTRACE_LIBRARY)
   set(POTRACE_FOUND TRUE)
   set(POTRACE_LIBRARIES ${POTRACE_LIBRARY})
else (POTRACE_INCLUDE_DIR AND POTRACE_LIBRARY)
   set(POTRACE_FOUND FALSE)
endif (POTRACE_INCLUDE_DIR AND POTRACE_LIBRARY)

if (POTRACE_FOUND)
   if (NOT Potrace_FIND_QUIETLY)
      message(STATUS "Found potrace: ${POTRACE_LIBRARIES}")
   endif (NOT Potrace_FIND_QUIETLY)
else (POTRACE_FOUND)
   if (NOT Potrace_FIND_QUIETLY)

 message(STATUS "don't find potrace")

   endif (NOT Potrace_FIND_QUIETLY)
endif (POTRACE_FOUND)

MARK_AS_ADVANCED(POTRACE_INCLUDE_DIR POTRACE_LIBRARIES POTRACE_LIBRARY)

