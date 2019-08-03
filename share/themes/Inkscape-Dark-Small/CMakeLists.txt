# SPDX-License-Identifier: GPL-2.0-or-later
install(FILES "readme.md" DESTINATION ${INKSCAPE_SHARE_INSTALL}/themes/Inkscape-Dark-Small)
file(GLOB _CSS
      "gtk-3.0/*.css"
)
file(GLOB _ASSETS
      "gtk-3.0/assets/*.svg"
      "gtk-3.0/assets/*.jpg"
      "gtk-3.0/assets/*.png"
      "gtk-3.0/assets/*.gif"
)
install(FILES ${_CSS} DESTINATION ${INKSCAPE_SHARE_INSTALL}/themes/Inkscape-Dark-Small/gtk-3.0)
install(FILES ${_ASSETS} DESTINATION ${INKSCAPE_SHARE_INSTALL}/themes/Inkscape-Dark-Small/gtk-3.0/assets)


