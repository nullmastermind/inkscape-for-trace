# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.xml" "*.rc" "*.css" "*.ui" "*.glade")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/ui)

file(GLOB _RESOURCES "resources/*.png" "resources/*.svg")
install(FILES ${_RESOURCES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/ui/resources)
