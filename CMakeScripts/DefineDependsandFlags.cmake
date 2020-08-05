set(INKSCAPE_LIBS "")
set(INKSCAPE_INCS "")
set(INKSCAPE_INCS_SYS "")
set(INKSCAPE_CXX_FLAGS "")
set(INKSCAPE_CXX_FLAGS_DEBUG "")

list(APPEND INKSCAPE_INCS ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src

    # generated includes
    ${CMAKE_BINARY_DIR}/include
)

# NDEBUG implies G_DISABLE_ASSERT
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER} MATCHES "-DNDEBUG")
    list(APPEND INKSCAPE_CXX_FLAGS "-DG_DISABLE_ASSERT")
endif()

# AddressSanitizer
# Clang's AddressSanitizer can detect more memory errors and is more powerful
# than compiling with _FORTIFY_SOURCE but has a performance impact (approx. 2x
# slower), so it's not suitable for release builds.
if(WITH_ASAN)
    list(APPEND INKSCAPE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    list(APPEND INKSCAPE_LIBS "-fsanitize=address")
    list(APPEND INKSCAPE_LIBS "-Wno-mismatched-tags")
    list(APPEND INKSCAPE_LIBS "-Wno-switch")
else()
    # Undefine first, to suppress 'warning: "_FORTIFY_SOURCE" redefined'
    list(APPEND INKSCAPE_CXX_FLAGS "-U_FORTIFY_SOURCE")
    list(APPEND INKSCAPE_CXX_FLAGS "-D_FORTIFY_SOURCE=2")
endif()

# Errors for common mistakes
list(APPEND INKSCAPE_CXX_FLAGS "-fstack-protector-strong")
list(APPEND INKSCAPE_CXX_FLAGS "-Werror=format")                # e.g.: printf("%s", std::string("foo"))
list(APPEND INKSCAPE_CXX_FLAGS "-Werror=format-security")       # e.g.: printf(variable);
list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-Og")                     # -Og for _FORTIFY_SOURCE. One could add -Weffc++ here to see approx. 6000 warnings
list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-Wcomment")
list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-Wunused-function")
list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-Wunused-variable")
list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-D_GLIBCXX_ASSERTIONS")
if (CMAKE_COMPILER_IS_GNUCC)
    list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-fexceptions -grecord-gcc-switches -fasynchronous-unwind-tables")
    if(CXX_COMPILER_VERSION VERSION_GREATER 8.0)
        list(APPEND INKSCAPE_CXX_FLAGS_DEBUG "-fstack-clash-protection -fcf-protection")
    endif()
endif()

# Define the flags for profiling if desired:
if(WITH_PROFILING)
    set(BUILD_SHARED_LIBS off)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
#include <cstdint>
std::atomic<uint64_t> x (0);
int main() {
  uint64_t i = x.load(std::memory_order_relaxed);
  return 0;
}
"
LIBATOMIC_NOT_NEEDED)
IF (NOT LIBATOMIC_NOT_NEEDED)
    message(STATUS "  Adding -latomic to the libs.")
    list(APPEND INKSCAPE_LIBS "-latomic")
ENDIF()


# ----------------------------------------------------------------------------
# Helper macros
# ----------------------------------------------------------------------------

# Turns linker arguments like "-framework Foo" into "-Wl,-framework,Foo" to
# make them safe for appending to INKSCAPE_LIBS
macro(sanitize_ldflags_for_libs ldflags_var)
    # matches dash-argument followed by non-dash-argument
    string(REGEX REPLACE "(^|;)(-[^;]*);([^-])" "\\1-Wl,\\2,\\3" ${ldflags_var} "${${ldflags_var}}")
endmacro()


# ----------------------------------------------------------------------------
# Files we include
# ----------------------------------------------------------------------------
if(WIN32)
    # Set the link and include directories
    get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

    list(APPEND INKSCAPE_LIBS "-lmscms")

    list(APPEND INKSCAPE_CXX_FLAGS "-mms-bitfields")
    list(APPEND INKSCAPE_CXX_FLAGS "-mwindows")
    list(APPEND INKSCAPE_CXX_FLAGS "-mthreads")

    list(APPEND INKSCAPE_LIBS "-lgomp")
    list(APPEND INKSCAPE_LIBS "-lwinpthread")

    if(HAVE_MINGW64)
        list(APPEND INKSCAPE_CXX_FLAGS "-m64")
    else()
        list(APPEND INKSCAPE_CXX_FLAGS "-m32")
    endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(INKSCAPE_DEP REQUIRED
                  harfbuzz
                  pangocairo
                  pangoft2
                  fontconfig
                  gsl
                  gmodule-2.0
                  libsoup-2.4>=2.42
                  #double-conversion
                  bdw-gc #boehm-demers-weiser gc
                  lcms2)

# remove this line and uncomment the doiuble-conversion above when double-conversion.pc file gets shipped on all platforms we support
find_package(DoubleConversion REQUIRED)  # lib2geom dependency

sanitize_ldflags_for_libs(INKSCAPE_DEP_LDFLAGS)
list(APPEND INKSCAPE_LIBS ${INKSCAPE_DEP_LDFLAGS})
list(APPEND INKSCAPE_INCS_SYS ${INKSCAPE_DEP_INCLUDE_DIRS})

add_definitions(${INKSCAPE_DEP_CFLAGS_OTHER})

if(WITH_JEMALLOC)
    find_package(JeMalloc)
    if (JEMALLOC_FOUND)
        list(APPEND INKSCAPE_LIBS ${JEMALLOC_LIBRARIES})
    else()
        set(WITH_JEMALLOC OFF)
    endif()
endif()

find_package(Iconv REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${Iconv_INCLUDE_DIRS})
list(APPEND INKSCAPE_LIBS ${Iconv_LIBRARIES})

find_package(Intl REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${Intl_INCLUDE_DIRS})
list(APPEND INKSCAPE_LIBS ${Intl_LIBRARIES})
add_definitions(${Intl_DEFINITIONS})

# Check for system-wide version of 2geom and fallback to internal copy if not found
if(NOT WITH_INTERNAL_2GEOM)
    pkg_check_modules(2Geom QUIET IMPORTED_TARGET GLOBAL 2geom>=1.0.0)
    if(2Geom_FOUND)
        add_library(2Geom::2geom ALIAS PkgConfig::2Geom)
    else()
        set(WITH_INTERNAL_2GEOM ON CACHE BOOL "Prefer internal copy of lib2geom" FORCE)
        message(STATUS "lib2geom not found, using internal copy in src/3rdparty/2geom")
    endif()
endif()
if(WITH_INTERNAL_2GEOM)
  set(2Geom_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/3rdparty/2geom/include)
endif()


if(ENABLE_POPPLER)
    find_package(PopplerCairo)
    if(POPPLER_FOUND)
        set(HAVE_POPPLER ON)
        if(ENABLE_POPPLER_CAIRO)
            if(POPPLER_CAIRO_FOUND AND POPPLER_GLIB_FOUND)
                set(HAVE_POPPLER_CAIRO ON)
            endif()
        endif()
    else()
        set(ENABLE_POPPLER_CAIRO OFF)
    endif()
else()
    set(HAVE_POPPLER OFF)
    set(ENABLE_POPPLER_CAIRO OFF)
endif()

list(APPEND INKSCAPE_INCS_SYS ${POPPLER_INCLUDE_DIRS})
list(APPEND INKSCAPE_LIBS     ${POPPLER_LIBRARIES})
add_definitions(${POPPLER_DEFINITIONS})

if(WITH_LIBWPG)
    pkg_check_modules(LIBWPG libwpg-0.3 librevenge-0.0 librevenge-stream-0.0)
    if(LIBWPG_FOUND)
        sanitize_ldflags_for_libs(LIBWPG_LDFLAGS)
        list(APPEND INKSCAPE_INCS_SYS ${LIBWPG_INCLUDE_DIRS})
        list(APPEND INKSCAPE_LIBS     ${LIBWPG_LDFLAGS})
        add_definitions(${LIBWPG_DEFINITIONS})
    else()
        set(WITH_LIBWPG OFF)
    endif()
endif()

if(WITH_LIBVISIO)
    pkg_check_modules(LIBVISIO libvisio-0.1 librevenge-0.0 librevenge-stream-0.0)
    if(LIBVISIO_FOUND)
        sanitize_ldflags_for_libs(LIBVISIO_LDFLAGS)
        list(APPEND INKSCAPE_INCS_SYS ${LIBVISIO_INCLUDE_DIRS})
        list(APPEND INKSCAPE_LIBS     ${LIBVISIO_LDFLAGS})
        add_definitions(${LIBVISIO_DEFINITIONS})
    else()
        set(WITH_LIBVISIO OFF)
    endif()
endif()

if(WITH_LIBCDR)
    pkg_check_modules(LIBCDR libcdr-0.1 librevenge-0.0 librevenge-stream-0.0)
    if(LIBCDR_FOUND)
        sanitize_ldflags_for_libs(LIBCDR_LDFLAGS)
        list(APPEND INKSCAPE_INCS_SYS ${LIBCDR_INCLUDE_DIRS})
        list(APPEND INKSCAPE_LIBS     ${LIBCDR_LDFLAGS})
        add_definitions(${LIBCDR_DEFINITIONS})
    else()
        set(WITH_LIBCDR OFF)
    endif()
endif()

FIND_PACKAGE(JPEG)
IF(JPEG_FOUND)
    list(APPEND INKSCAPE_INCS_SYS ${JPEG_INCLUDE_DIR})
    list(APPEND INKSCAPE_LIBS ${JPEG_LIBRARIES})
    set(HAVE_JPEG ON)
ENDIF()

find_package(PNG REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${PNG_PNG_INCLUDE_DIR})
list(APPEND INKSCAPE_LIBS ${PNG_LIBRARY})

find_package(Potrace REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${POTRACE_INCLUDE_DIRS})
list(APPEND INKSCAPE_LIBS ${POTRACE_LIBRARIES})


if(WITH_DBUS)
    pkg_check_modules(DBUS dbus-1 dbus-glib-1)
    if(DBUS_FOUND)
        sanitize_ldflags_for_libs(DBUS_LDFLAGS)
        list(APPEND INKSCAPE_LIBS ${DBUS_LDFLAGS})
        list(APPEND INKSCAPE_INCS_SYS ${DBUS_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/src/extension/dbus/)
        add_definitions(${DBUS_CFLAGS_OTHER})
    else()
        set(WITH_DBUS OFF)
    endif()
endif()

if(WITH_SVG2)
    add_definitions(-DWITH_MESH -DWITH_CSSBLEND -DWITH_CSSCOMPOSITE -DWITH_SVG2)
else()
    add_definitions(-UWITH_MESH -UWITH_CSSBLEND -UWITH_CSSCOMPOSITE -UWITH_SVG2)
endif()

if(APPLE)
  # gtk+-3.0.pc should have targets=quartz (or not targets=x11)
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.4")
    pkg_get_variable(GTK3_TARGETS gtk+-3.0 targets)
  endif()
  if(NOT("${GTK3_TARGETS}" MATCHES "x11"))
    pkg_check_modules(MacIntegration REQUIRED gtk-mac-integration-gtk3)
    list(APPEND INKSCAPE_INCS_SYS ${MacIntegration_INCLUDE_DIRS})
    sanitize_ldflags_for_libs(MacIntegration_LDFLAGS)
    list(APPEND INKSCAPE_LIBS ${MacIntegration_LDFLAGS})
  endif()
endif()

# ----------------------------------------------------------------------------
# CMake's builtin
# ----------------------------------------------------------------------------

# Include dependencies:

pkg_check_modules(
    GTK3
    REQUIRED
    gtkmm-3.0>=3.24
    gdkmm-3.0>=3.24
    gtk+-3.0>=3.24
    gdk-3.0>=3.24
    )
list(APPEND INKSCAPE_CXX_FLAGS ${GTK3_CFLAGS_OTHER})
list(APPEND INKSCAPE_INCS_SYS ${GTK3_INCLUDE_DIRS})
list(APPEND INKSCAPE_LIBS ${GTK3_LIBRARIES})
link_directories(${GTK3_LIBRARY_DIRS})

if(WITH_GSPELL)
    pkg_check_modules(GSPELL gspell-1)
    if("${GSPELL_FOUND}")
        message(STATUS "Using gspell")
        list(APPEND INKSCAPE_INCS_SYS ${GSPELL_INCLUDE_DIRS})
        sanitize_ldflags_for_libs(GSPELL_LDFLAGS)
        list(APPEND INKSCAPE_LIBS ${GSPELL_LDFLAGS})
    else()
        set(WITH_GSPELL OFF)
    endif()
endif()

find_package(Boost 1.19.0 REQUIRED COMPONENTS filesystem)

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    list(APPEND INKSCAPE_LIBS "-lstdc++fs")
endif()

list(APPEND INKSCAPE_INCS_SYS ${Boost_INCLUDE_DIRS})
# list(APPEND INKSCAPE_LIBS ${Boost_LIBRARIES})

#find_package(OpenSSL)
#list(APPEND INKSCAPE_INCS_SYS ${OPENSSL_INCLUDE_DIR})
#list(APPEND INKSCAPE_LIBS ${OPENSSL_LIBRARIES})

find_package(LibXslt REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${LIBXSLT_INCLUDE_DIR})
list(APPEND INKSCAPE_LIBS ${LIBXSLT_LIBRARIES})
add_definitions(${LIBXSLT_DEFINITIONS})

find_package(LibXml2 REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${LIBXML2_INCLUDE_DIR})
list(APPEND INKSCAPE_LIBS ${LIBXML2_LIBRARIES})
add_definitions(${LIBXML2_DEFINITIONS})

if(WITH_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        list(APPEND INKSCAPE_CXX_FLAGS  ${OpenMP_CXX_FLAGS})
        if(APPLE)
            list(APPEND INKSCAPE_LIBS "-lomp")
        endif()
        mark_as_advanced(OpenMP_C_FLAGS)
        mark_as_advanced(OpenMP_CXX_FLAGS)
        # '-fopenmp' is in OpenMP_C_FLAGS, OpenMP_CXX_FLAGS and implies '-lgomp'
        # uncomment explicit linking below if still needed:
        set(HAVE_OPENMP ON)
        #list(APPEND INKSCAPE_LIBS "-lgomp")  # FIXME
    else()
        set(HAVE_OPENMP OFF)
        set(WITH_OPENMP OFF)
    endif()
endif()

find_package(ZLIB REQUIRED)
list(APPEND INKSCAPE_INCS_SYS ${ZLIB_INCLUDE_DIRS})
list(APPEND INKSCAPE_LIBS ${ZLIB_LIBRARIES})

if(WITH_GNU_READLINE)
  pkg_check_modules(Readline readline)
  if(Readline_FOUND)
    message(STATUS "Found GNU Readline: ${Readline_LIBRARY}")
    list(APPEND INKSCAPE_INCS_SYS ${Readline_INCLUDE_DIRS})
    list(APPEND INKSCAPE_LIBS ${Readline_LDFLAGS})
  else()
    message(STATUS "Did not find GNU Readline")
    set(WITH_GNU_READLINE OFF)
  endif()
endif()

if(WITH_IMAGE_MAGICK)
    # we want "<" but pkg_check_modules only offers "<=" for some reason; let's hope nobody actually has 7.0.0
    pkg_check_modules(MAGICK ImageMagick++<=7)
    if(MAGICK_FOUND)
        set(WITH_GRAPHICS_MAGICK OFF)  # prefer ImageMagick for now and disable GraphicsMagick if found
    else()
        set(WITH_IMAGE_MAGICK OFF)
    endif()
endif()
if(WITH_GRAPHICS_MAGICK)
    pkg_check_modules(MAGICK GraphicsMagick++)
    if(NOT MAGICK_FOUND)
        set(WITH_GRAPHICS_MAGICK OFF)
    endif()
endif()
if(MAGICK_FOUND)
    sanitize_ldflags_for_libs(MAGICK_LDFLAGS)
    list(APPEND INKSCAPE_LIBS ${MAGICK_LDFLAGS})
    add_definitions(${MAGICK_CFLAGS_OTHER})
    list(APPEND INKSCAPE_INCS_SYS ${MAGICK_INCLUDE_DIRS})

    set(WITH_MAGICK ON) # enable 'Extensions > Raster'
endif()

set(ENABLE_NLS OFF)
if(WITH_NLS)
    find_package(Gettext)
    if(GETTEXT_FOUND)
        message(STATUS "Found gettext + msgfmt to convert language files. Translation enabled")
        set(ENABLE_NLS ON)
    else(GETTEXT_FOUND)
        message(STATUS "Cannot find gettext + msgfmt to convert language file. Translation won't be enabled")
        set(WITH_NLS OFF)
    endif(GETTEXT_FOUND)
    find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
    if(GETTEXT_XGETTEXT_EXECUTABLE)
        message(STATUS "Found xgettext. inkscape.pot will be re-created if missing.")
    else()
        message(STATUS "Did not find xgetttext. inkscape.pot can't be re-created.")
    endif()
endif(WITH_NLS)

pkg_check_modules(SIGC++ REQUIRED sigc++-2.0 )
sanitize_ldflags_for_libs(SIGC++_LDFLAGS)
list(APPEND INKSCAPE_LIBS ${SIGC++_LDFLAGS})
list(APPEND INKSCAPE_CXX_FLAGS ${SIGC++_CFLAGS_OTHER})

# Some linkers, like gold, don't find symbols recursively. So we have to link against X11 explicitly
find_package(X11)
if(X11_FOUND)
    list(APPEND INKSCAPE_INCS_SYS ${X11_INCLUDE_DIRS})
    list(APPEND INKSCAPE_LIBS ${X11_LIBRARIES})
endif(X11_FOUND)

# end Dependencies



# Set include directories and CXX flags
# (INKSCAPE_LIBS are set as target_link_libraries for inkscape_base in src/CMakeLists.txt)

foreach(flag ${INKSCAPE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
endforeach()
foreach(flag ${INKSCAPE_CXX_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${flag}")
endforeach()

# Add color output to ninja
if ("${CMAKE_GENERATOR}" MATCHES "Ninja")
    add_compile_options (-fdiagnostics-color)
endif ()

list(REMOVE_DUPLICATES INKSCAPE_LIBS)
list(REMOVE_DUPLICATES INKSCAPE_INCS_SYS)

include_directories(${INKSCAPE_INCS})
include_directories(SYSTEM ${INKSCAPE_INCS_SYS})

include(${CMAKE_CURRENT_LIST_DIR}/ConfigChecks.cmake) # TODO: Check if this needs to be "hidden" here

unset(INKSCAPE_INCS)
unset(INKSCAPE_INCS_SYS)
unset(INKSCAPE_CXX_FLAGS)
unset(INKSCAPE_CXX_FLAGS_DEBUG)
