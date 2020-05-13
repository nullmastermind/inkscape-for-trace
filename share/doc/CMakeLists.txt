# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES "*")
list(FILTER _FILES EXCLUDE REGEX "CMakeLists\\.txt")
install(FILES ${_FILES} DESTINATION ${INKSCAPE_SHARE_INSTALL}/doc)

