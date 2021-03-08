# SPDX-License-Identifier: GPL-2.0-or-later

# install application icons into system-wide hicolor icon theme "apps" folder
# it's also the fallback for all other themes that do not include a customized Inkscape icon.
set(PIXMAP_SIZES "16x16" "22x22" "24x24" "32x32" "48x48" "256x256" "scalable" "symbolic")
foreach(pixmap_size ${PIXMAP_SIZES})
  FILE(GLOB PIXMAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/*.png ${CMAKE_CURRENT_SOURCE_DIR}/${pixmap_size}/*.svg)
  install(FILES ${PIXMAP_FILES} DESTINATION ${SHARE_INSTALL}/icons/hicolor/${pixmap_size}/apps)
endforeach(pixmap_size)
