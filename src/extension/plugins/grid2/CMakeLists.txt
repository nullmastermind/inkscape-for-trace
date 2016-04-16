set(grid_PART_SRCS grid.cpp)                                                                      

include_directories( ${CMAKE_BINARY_DIR}/src )

add_library(grid2 SHARED ${grid_PART_SRCS})
                                                                                                              
target_link_libraries(grid2 inkscape_base)                                                  
                                                                                                              
#install(TARGETS kritaartivity DESTINATION ${PLUGIN_INSTALL_DIR})                                              
                                                                                                              
########### install files ###############                                                                     
                                                                                                              
#install(FILES artivity.rc DESTINATION ${DATA_INSTALL_DIR}/kritaplugins)                                       
#install(FILES kritaartivity.desktop DESTINATION ${SERVICES_INSTALL_DIR})                                      
                                                                                                              
#include(CPack)   
