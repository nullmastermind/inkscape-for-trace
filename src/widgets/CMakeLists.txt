# SPDX-License-Identifier: GPL-2.0-or-later

set(widgets_SRC
	desktop-widget.cpp
	ege-paint-def.cpp
	ink-action.cpp
	sp-attribute-widget.cpp
	sp-xmlview-tree.cpp
	spw-utilities.cpp
	toolbox.cpp

	# -------
	# Headers
	desktop-widget.h
	ege-paint-def.h
	ink-action.h
	sp-attribute-widget.h
	sp-xmlview-tree.h
	spw-utilities.h
	toolbox.h
	widget-sizes.h
)

add_inkscape_source("${widgets_SRC}")
