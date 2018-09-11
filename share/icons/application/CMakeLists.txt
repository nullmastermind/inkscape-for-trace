# SPDX-License-Identifier: GPL-2.0-or-later
set(PIXMAP_SIZES "16x16" "22x22" "24x24" "32x32" "48x48" "256x256")
set(THEME hicolor)
foreach(pixmap_size ${PIXMAP_SIZES})
  FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/*.svg)
  install(FILES ${PIXMAP_FILES} DESTINATION ${SHARE_INSTALL}/icons/${THEME}/${pixmap_size}/apps)
endforeach(pixmap_size)

set(PIXMAP_SIZES "scalable" "symbolic")
set(THEME hicolor)
set(CONTENT "apps")

foreach(pixmap_size ${PIXMAP_SIZES})
  foreach(content ${CONTENT})
    FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.svg)
    install(FILES ${PIXMAP_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/icons/${THEME}/${pixmap_size}/${content})
  endforeach(content)
endforeach(pixmap_size)


set(PIXMAP_SIZES "scalable")
set(THEME Tango)
set(CONTENT "apps")

foreach(pixmap_size ${PIXMAP_SIZES})
  foreach(content ${CONTENT})
    FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/${content}/*.svg)
    install(FILES ${PIXMAP_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/icons/${THEME}/${pixmap_size}/${content})
  endforeach(content)
endforeach(pixmap_size)
