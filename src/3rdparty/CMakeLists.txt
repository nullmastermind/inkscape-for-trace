add_subdirectory(libuemf)
add_subdirectory(libcroco)
add_subdirectory(libdepixelize)
add_subdirectory(adaptagrams)
add_subdirectory(autotrace)

if(NOT 2Geom_FOUND)
    add_subdirectory(2geom)
endif()
