# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.svg" "*.jpg" "*.png" "README")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/pixmaps)

install(FILES ../branding/inkscape.svg DESTINATION ${INKSCAPE_SHARE_INSTALL}/pixmaps)
