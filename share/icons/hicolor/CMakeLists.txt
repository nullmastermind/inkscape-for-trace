set(THEME hicolor)

set(PIXMAP_SIZES "scalable")
set(CONTENT "actions")

foreach(pixmap_size ${PIXMAP_SIZES})
  foreach(content ${CONTENT})
    FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.svg)
    install(FILES ${PIXMAP_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/icons/${THEME}/${pixmap_size}/${content})
  endforeach(content)
endforeach(pixmap_size)

install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t ${CMAKE_CURRENT_SOURCE_DIR})")
install(FILES icon-theme.cache DESTINATION ${SHARE_INSTALL}/icons/${THEME})
