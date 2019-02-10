# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/templates.h
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES} > ${CMAKE_CURRENT_SOURCE_DIR}/templates.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES}
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/templates.h PROPERTIES GENERATED TRUE)
add_custom_target(templates_h ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/templates.h)

install(FILES ${_FILES} "README" DESTINATION ${INKSCAPE_SHARE_INSTALL}/templates)


# create localized versions of default.svg
file(GLOB POFILES ${CMAKE_SOURCE_DIR}/po/*.po)
  foreach(pofile ${POFILES})
  string(REGEX REPLACE "(.+(\\\\|/))+" "${CMAKE_BINARY_DIR}/po/" pofile ${pofile})
  string(REGEX REPLACE "\\.po$" ".gmo" pofile ${pofile})
  list(APPEND GMOFILES "${pofile}")
endforeach(pofile)

add_custom_command(
  OUTPUT default_templates.timestamp
  COMMAND ./create_default_templates.py ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/default.svg ${CMAKE_CURRENT_SOURCE_DIR}/create_default_templates.py ${GMOFILES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating localized default templates"
)
add_custom_target(default_templates ALL DEPENDS default_templates.timestamp)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DESTINATION ${INKSCAPE_SHARE_INSTALL} 
  FILES_MATCHING PATTERN "*.svg" PATTERN "CMakeFiles" EXCLUDE )
