# SPDX-License-Identifier: GPL-2.0-or-later

file(GLOB _FILES "README" "*.svg" "*.jpg" "*.png")

filter_and_install_translated_content(_FILES ${INKSCAPE_SHARE_INSTALL}/tutorials)
install(FILES ${_FILES}
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/tutorials
    COMPONENT tutorials)
