# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.xml")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/keys)

# install a duplicate of inkscape.xml as default.xml
#   default.xml is the default keymap that will be used by inkscape after installation
#   replace this with a keymap of your own to override the Inkscape default shortcuts
install(FILES inkscape.xml
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/keys
    RENAME default.xml)
