set(THEME nextgen)
install(FILES "index.theme" DESTINATION ${INKSCAPE_SHARE_INSTALL}/icons/${THEME})

set(PIXMAP_SIZES "scalable" "symbolic")
set(CONTENT "actions")

foreach(pixmap_size ${PIXMAP_SIZES})
  foreach(content ${CONTENT})
    FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.svg)
    install(FILES ${PIXMAP_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/icons/${THEME}/${pixmap_size}/${content})
  endforeach(content)
endforeach(pixmap_size)

if(WIN32)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache \${CMAKE_INSTALL_PREFIX}/${INKSCAPE_SHARE_INSTALL}/icons/${THEME})")
endif()
