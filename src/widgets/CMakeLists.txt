# SPDX-License-Identifier: GPL-2.0-or-later
add_subdirectory(gimp)

set(widgets_SRC
	desktop-widget.cpp
	ege-adjustment-action.cpp
	ege-paint-def.cpp
	fill-style.cpp
	gradient-image.cpp
	gradient-selector.cpp
	gradient-vector.cpp
	ink-action.cpp
	ink-comboboxentry-action.cpp
	ink-toggle-action.cpp
	ink-tool-menu-action.cpp
	paint-selector.cpp
	sp-attribute-widget.cpp
	sp-color-selector.cpp
	sp-xmlview-tree.cpp
	spinbutton-events.cpp
	spw-utilities.cpp
	stroke-marker-selector.cpp
	stroke-style.cpp
	swatch-selector.cpp
	toolbox.cpp

	# -------
	# Headers
	desktop-widget.h
	ege-adjustment-action.h
	ege-paint-def.h
	fill-n-stroke-factory.h
	fill-style.h
	gradient-image.h
	gradient-selector.h
	gradient-vector.h
	ink-action.h
	ink-comboboxentry-action.h
	ink-toggle-action.h
	ink-tool-menu-action.h
	paint-selector.h
	sp-attribute-widget.h
	sp-color-selector.h
	sp-xmlview-tree.h
	spinbutton-events.h
	spw-utilities.h
	stroke-marker-selector.h
	stroke-style.h
	swatch-selector.h
	toolbox.h
	widget-sizes.h
)

add_inkscape_source("${widgets_SRC}")
