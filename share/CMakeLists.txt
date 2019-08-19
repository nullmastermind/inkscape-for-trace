# SPDX-License-Identifier: GPL-2.0-or-later
add_subdirectory(attributes)
add_subdirectory(branding)
add_subdirectory(examples)
add_subdirectory(filters)
add_subdirectory(fonts)
add_subdirectory(icons)
add_subdirectory(keys)
add_subdirectory(markers)
add_subdirectory(paint)
add_subdirectory(palettes)
add_subdirectory(screens)
add_subdirectory(symbols)
add_subdirectory(templates)
add_subdirectory(pixmaps)
add_subdirectory(tutorials)
add_subdirectory(ui)

install(DIRECTORY extensions DESTINATION ${INKSCAPE_SHARE_INSTALL}
	PATTERN ".git*" EXCLUDE
	PATTERN "tests" EXCLUDE
	PATTERN "*.pyc" EXCLUDE
	)
