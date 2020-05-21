# SPDX-License-Identifier: GPL-2.0-or-later

# process and install .po files
file(GLOB LANGUAGES *.po)
foreach(language ${LANGUAGES})
	string(REGEX REPLACE "(.+(\\\\|/))+" "" language ${language})
	string(REGEX REPLACE "\\.po$" "" language ${language})
    string(MAKE_C_IDENTIFIER "${language}" language_code_escaped)
	set(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${language}.po)
	set(gmofile ${CMAKE_CURRENT_BINARY_DIR}/${language}.gmo)
	GETTEXT_PROCESS_PO_FILES(${language} ALL PO_FILES ${pofile})
	install(FILES ${gmofile}
        DESTINATION "${PACKAGE_LOCALE_DIR}/${language}/LC_MESSAGES/"
        RENAME ${CMAKE_PROJECT_NAME}.mo
        COMPONENT translations.${language_code_escaped})
endforeach(language)


# update inkscape.pot
set(_potFile ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.pot)
set(xgettext_options -cTRANSLATORS -j --keyword=_ --keyword=N_ --keyword=Q_:1g --keyword=C_:1c,2 --keyword=NC_:1c,2 --msgid-bugs-address=inkscape-translator@lists.inkscape.org --from-code=UTF-8 -ktranslatable -o ${_potFile})

# Due to a bug in old xgettext versions, we cannot use '--its='.
# Instead, we use GETTEXTDATADIR=. to refer to the po/its/*.its files.
# This requires a .loc file for each .its file. See po/its/units.loc for an example.
# See https://gitlab.com/inkscape/inkscape/-/issues/271 for details.
# This workaround is necessary for Debian 9, so roughly until ca 2021.
set(its_workaround ${CMAKE_COMMAND} -E env GETTEXTDATADIR=.)

add_custom_target(inkscape_pot BYPRODUCTS ${_potFile}
    # make sure inkscape.pot is re-created from scratch
    COMMAND rm -f ${_potFile} && touch ${_potFile}

    # extract strings from source files into inkscape.pot
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} -C -f POTFILES.src.in
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} -L Glade -f POTFILES.ui.in
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} -L Python -f POTFILES.py.in
    # the following uses po/its/inx.{loc, its}
    COMMAND ${its_workaround} ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} -f POTFILES.inx.in --from-code=UTF-8
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} -L AppData ../org.inkscape.Inkscape.appdata.xml.in
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} -L Desktop ../org.inkscape.Inkscape.desktop.template
    # the following uses po/its/menus.{loc, its}
    COMMAND ${its_workaround} ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} ../share/ui/menus.xml
    # the following uses po/its/units.{loc, its}
    COMMAND ${its_workaround} ${GETTEXT_XGETTEXT_EXECUTABLE} ${xgettext_options} ../share/ui/units.xml

    COMMENT "Extract translatable messages to ${_potFile}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(inkscape_pot filters_svg_h)
add_dependencies(inkscape_pot palettes_h)
add_dependencies(inkscape_pot patterns_svg_h)
add_dependencies(inkscape_pot symbols_h)
add_dependencies(inkscape_pot templates_h)

