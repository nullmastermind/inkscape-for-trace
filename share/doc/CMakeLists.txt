# SPDX-License-Identifier: GPL-2.0-or-later

# "human-readable" LICENSE file for about dialog
install(FILES
    LICENSE
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/doc)

# actual LICENSE files from LICENSES subdirectory
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/LICENSES/
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/doc)

# additional copyright related files from root dir
install(FILES
    ${CMAKE_SOURCE_DIR}/AUTHORS
    ${CMAKE_SOURCE_DIR}/TRANSLATORS
    ${CMAKE_SOURCE_DIR}/COPYING
    DESTINATION ${INKSCAPE_SHARE_INSTALL}/doc)
