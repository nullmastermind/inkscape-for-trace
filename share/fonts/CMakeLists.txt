# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*.ttf" "README")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/fonts)
