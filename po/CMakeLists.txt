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
	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop
		DEPENDS ${LANGUAGES}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --desktop --template ${CMAKE_SOURCE_DIR}/org.inkscape.Inkscape.desktop.template -d ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop.template.in --keyword=Name --keyword=GenericName --keyword=X-GNOME-FullName --keyword=Comment --keyword=Keywords
		COMMAND ${CMAKE_COMMAND} -DINKSCAPE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DINKSCAPE_BINARY_DIR=${CMAKE_BINARY_DIR} -DENABLE_BINRELOC=${ENABLE_BINRELOC} -P ${CMAKE_SOURCE_DIR}/CMakeScripts/inkscape-desktop.cmake
		)
	add_custom_target(inkscape_desktop ALL DEPENDS ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.desktop)

	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
		DEPENDS ${LANGUAGES}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --xml --template ${CMAKE_SOURCE_DIR}/org.inkscape.Inkscape.appdata.xml.in -d ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml
		)
	add_custom_target(inkscape_appdata ALL DEPENDS ${CMAKE_BINARY_DIR}/org.inkscape.Inkscape.appdata.xml)
	
	
	
	# update inkscape.pot
	SET(_potFile ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.pot)
	add_custom_command(OUTPUT ${_potFile}
		#COMMAND sh -c "${INTLTOOL-UPDATE} --pot --gettext-package=inkscape"
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}    -L Glade --from-code=UTF-8 -ktranslatable -k_ -kN_ -o ${_potFile} -f POTFILES.ui.in
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -L Python -f POTFILES.py.in -o ${_potFile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -f POTFILES.inx.in --its its/inx.its -o ${_potFile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -C -f POTFILES.src.in -o ${_potFile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -L AppData ../org.inkscape.Inkscape.appdata.xml.in -o ${_potFile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j -L Desktop ../org.inkscape.Inkscape.desktop.template -o ${_potFile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j --its its/menus.its ../share/ui/menus.xml -o ${_potFile}
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE} -j --its its/units.its ../share/ui/units.xml -o ${_potFile}
		COMMENT "Extract translatable messages to ${_potFile}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
	set_source_files_properties(${_potFile} PROPERTIES GENERATED TRUE)

	add_custom_target(inkscape_pot ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/inkscape.pot)
	add_dependencies(inkscape_pot filters_svg_h)
	add_dependencies(inkscape_pot palettes_h)
	add_dependencies(inkscape_pot patterns_svg_h)
	add_dependencies(inkscape_pot symbols_h)
	add_dependencies(inkscape_pot templates_h)
endif()
