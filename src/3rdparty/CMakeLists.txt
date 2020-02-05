#include(ExternalProject)
include(FetchContent)
add_subdirectory(libuemf)
add_subdirectory(libcroco)
add_subdirectory(libdepixelize)
add_subdirectory(adaptagrams)
add_subdirectory(autotrace)

# FetchContent can be replaced with git submodule
# git submodule add https://gitlab.com/inkscape/lib2geom.git
FetchContent_Declare(
    2geom
    GIT_REPOSITORY https://gitlab.com/sizmailov/lib2geom.git  # fixme: use inkscape repo
    GIT_TAG        be-friendly-to-super-project               # fixme: use dedicated branch
)

FetchContent_MakeAvailable(2geom)