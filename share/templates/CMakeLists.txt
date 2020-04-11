# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB _FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.svg")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/templates.h
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES} > ${CMAKE_CURRENT_SOURCE_DIR}/templates.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/i18n.py ${_FILES}
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/templates.h PROPERTIES GENERATED TRUE)
add_custom_target(templates_h ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/templates.h)

install(FILES ${_FILES} "README" DESTINATION ${INKSCAPE_SHARE_INSTALL}/templates)


# create localized versions of default.svg
if(ENABLE_NLS)
  file(GLOB POFILES ${CMAKE_SOURCE_DIR}/po/*.po)
  foreach(pofile ${POFILES})
    string(REGEX REPLACE "(.+(\\\\|/))+" "${CMAKE_BINARY_DIR}/po/" pofile ${pofile})
    string(REGEX REPLACE "\\.po$" ".gmo" pofile ${pofile})
    list(APPEND GMOFILES "${pofile}")
  endforeach(pofile)

  add_custom_command(
    OUTPUT default_templates.timestamp
    COMMAND python3 ./create_default_templates.py ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/default.svg ${CMAKE_CURRENT_SOURCE_DIR}/create_default_templates.py ${GMOFILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating localized default templates"
  )
  add_custom_target(default_templates ALL DEPENDS default_templates.timestamp)

  get_inkscape_languages()
  foreach(language_code ${INKSCAPE_LANGUAGE_CODES})
    string(MAKE_C_IDENTIFIER "${language_code}" language_code_escaped)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/default.${language_code}.svg
      DESTINATION ${INKSCAPE_SHARE_INSTALL}/templates
      COMPONENT translations.${language_code_escaped}
      OPTIONAL)
  endforeach()
else()
  message(STATUS "Generation of localized default templates will be skipped (building without NLS)")
endif()
