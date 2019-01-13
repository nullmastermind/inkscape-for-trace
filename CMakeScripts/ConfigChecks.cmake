include(${CMAKE_CURRENT_LIST_DIR}/ConfigPaths.cmake)

#---------------
# From here on:
# Set all HAVE_XXX variables, to correctly set all defines in config.h
#SET(CMAKE_REQUIRED_INCLUDES ${INK_INCLUDES})
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckStructHasMember)
# usage: CHECK_INCLUDE_FILES (<header> <RESULT_VARIABLE> )
# usage: CHECK_FUNCTION_EXISTS (<function name> <RESULT_VARIABLE> )
# usage: CHECK_STRUCT_HAS_MEMBER (<struct> <member> <header> <RESULT_VARIABLE>)

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${INKSCAPE_LIBS})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${INKSCAPE_INCS_SYS})

CHECK_INCLUDE_FILES(cairo-pdf.h HAVE_CAIRO_PDF)
CHECK_INCLUDE_FILES(ieeefp.h HAVE_IEEEFP_H)
CHECK_FUNCTION_EXISTS(mallinfo HAVE_MALLINFO)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_STRUCT_HAS_MEMBER(fordblks mallinfo malloc.h HAVE_STRUCT_MALLINFO_FORDBLKS)
CHECK_STRUCT_HAS_MEMBER(fsmblks mallinfo malloc.h HAVE_STRUCT_MALLINFO_FSMBLKS)
CHECK_STRUCT_HAS_MEMBER(hblkhd mallinfo malloc.h HAVE_STRUCT_MALLINFO_HBLKHD)
CHECK_STRUCT_HAS_MEMBER(uordblks mallinfo malloc.h HAVE_STRUCT_MALLINFO_UORDBLKS)
CHECK_STRUCT_HAS_MEMBER(usmblks mallinfo malloc.h HAVE_STRUCT_MALLINFO_USMBLKS)

# Enable pango defines, necessary for compilation on Win32, how about Linux?
# yes but needs to be done a better way
if(HAVE_CAIRO_PDF)
    set(PANGO_ENABLE_ENGINE TRUE)
endif()

# Create the configuration files config.h in the binary root dir
configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
add_definitions(-DHAVE_CONFIG_H)
