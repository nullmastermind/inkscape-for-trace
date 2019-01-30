# process and install .po files
# SPDX-License-Identifier: GPL-2.0-or-later
file(GLOB LANGUAGES *.po)
foreach(language ${LANGUAGES})
    string(REGEX REPLACE "(.+(\\\\|/))+" "" language ${language})
    string(REGEX REPLACE "\\.po$" "" language ${language})
    set(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${language}.po)
    set(gmofile ${CMAKE_CURRENT_BINARY_DIR}/${language}.gmo)
    GETTEXT_PROCESS_PO_FILES(${language} ALL PO_FILES ${pofile})
    install(FILES ${gmofile} DESTINATION "${PACKAGE_LOCALE_DIR}/${language}/LC_MESSAGES/" RENAME ${CMAKE_PROJECT_NAME}.mo)
endforeach(language)


if(UNIX)
    # translate org.inkscape.Inkscape.desktop
    if("${GETTEXT_VERSION_STRING}" VERSION_GREATER "0.19")
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop
            DEPENDS ${LANGUAGES}
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --desktop --template ${CMAKE_SOURCE_DIR}/org.inkscape.Inkscape.desktop.template -d ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop.template.in --keyword=Name --keyword=GenericName --keyword=X-GNOME-FullName --keyword=Comment --keyword=Keywords
            COMMAND ${CMAKE_COMMAND} -DINKSCAPE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DINKSCAPE_BINARY_DIR=${CMAKE_BINARY_DIR} -DENABLE_BINRELOC=${ENABLE_BINRELOC} -P ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-desktop.cmake
        )
    else()
        message("Old gettext version, not translating org.inkscape.Inkscape.desktop")
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop
            COMMAND cp ${CMAKE_SOURCE_DIR}/org.inkscape.Inkscape.desktop.template ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop.template.in
            COMMAND ${CMAKE_COMMAND} -DINKSCAPE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DINKSCAPE_BINARY_DIR=${CMAKE_BINARY_DIR} -DENABLE_BINRELOC=${ENABLE_BINRELOC} -P ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-desktop.cmake
        )
    endif()
    add_custom_target(inkscape_desktop ALL DEPENDS ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop)


    # translate org.inkscape.Inkscape.appdata.xml
    if("${GETTEXT_VERSION_STRING}" VERSION_GREATER "0.19")
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
            DEPENDS ${LANGUAGES}
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --xml --template ${CMAKE_SOURCE_DIR}/org.inkscape.Inkscape.appdata.xml.in -d ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
        )
    else()
        message("Old gettext version, not translating org.inkscape.Inkscape.appdata.xml")
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
            COMMAND cp ${CMAKE_SOURCE_DIR}/org.inkscape.Inkscape.appdata.xml.in ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
        )
    endif()
    add_custom_target(inkscape_appdata ALL DEPENDS ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml)
    if(GETTEXT_MSGMERGE_EXECUTABLE)
        SET(_potFile ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.pot)
        add_custom_target(inkscape_pot ALL DEPENDS ${_potFile})
        set_source_files_properties(${_potFile} PROPERTIES GENERATED TRUE)
        add_dependencies(inkscape_pot filters_svg_h)
        add_dependencies(inkscape_pot palettes_h)
        add_dependencies(inkscape_pot patterns_svg_h)
        add_dependencies(inkscape_pot symbols_h)
        add_dependencies(inkscape_pot templates_h)
        ADD_CUSTOM_COMMAND(OUTPUT ${_potFile}
            COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}    -L Desktop --from-code=UTF-8 -ktranslatable -k_ -kN_ -o ${_potFile} ${CMAKE_CURRENT_SOURCE_DIR}/../org.inkscape.Inkscape.appdata.xml.in
            COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -L Desktop --from-code=UTF-8 -ktranslatable -k_ -kN_ -o ${_potFile} ${CMAKE_CURRENT_SOURCE_DIR}/../org.inkscape.Inkscape.desktop.in
            COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -L Glade   --from-code=UTF-8 -ktranslatable -k_ -kN_ -o ${_potFile} -f ${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.xml.in
            COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -L Python  --from-code=UTF-8 -ktranslatable -k_ -kN_ -o ${_potFile} -f ${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.py.in
            COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j            --from-code=UTF-8 -ktranslatable -k_ -kN_ -o ${_potFile} -f ${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in
            COMMENT "Extract translatable messages to ${_potFile}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        )
    endif()
endif()
