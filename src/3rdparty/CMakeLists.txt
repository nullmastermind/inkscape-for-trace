add_subdirectory(libuemf)
add_subdirectory(libcroco)
add_subdirectory(libdepixelize)
add_subdirectory(adaptagrams)
add_subdirectory(autotrace)

if(WITH_INTERNAL_2GEOM)
    set(2GEOM_BUILD_SHARED ${BUILD_SHARED_LIBS} CACHE BOOL "")
    add_subdirectory(2geom)
endif()
