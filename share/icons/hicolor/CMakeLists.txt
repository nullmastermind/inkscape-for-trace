set(THEME hicolor)

set(PIXMAP_SIZES "scalable")
set(CONTENT "actions")

foreach(pixmap_size ${PIXMAP_SIZES})
  foreach(content ${CONTENT})
    FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.svg)
    install(FILES ${PIXMAP_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/icons/${THEME}/${pixmap_size}/${content})
  endforeach(content)
endforeach(pixmap_size)

if(0)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t ${INKSCAPE_SHARE_INSTALL}/icons/${THEME})")
    install(FILES ${INKSCAPE_SHARE_INSTALL}/icons/${THEME}/icon-theme.cache DESTINATION ${SHARE_INSTALL}/icons/${THEME})
endif()
