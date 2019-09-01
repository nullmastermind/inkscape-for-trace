// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_EXTENSION_INTERNAL_FILTER_FILTER_H
#define INKSCAPE_EXTENSION_INTERNAL_FILTER_FILTER_H

/*
 * Copyright (C) 2008 Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "extension/implementation/implementation.h"

namespace Inkscape {

namespace XML {
	struct Document;
}

namespace Extension {

class Effect;
class Extension;

namespace Internal {
namespace Filter {

class Filter : public Inkscape::Extension::Implementation::Implementation {
protected:
	gchar const * _filter;
	virtual gchar const * get_filter_text (Inkscape::Extension::Extension * ext);

private:
	Inkscape::XML::Document * get_filter (Inkscape::Extension::Extension * ext);
	void merge_filters (Inkscape::XML::Node * to, Inkscape::XML::Node * from, Inkscape::XML::Document * doc, gchar const * srcGraphic = nullptr, gchar const * srcGraphicAlpha = nullptr);

public:
	Filter();
	Filter(gchar const * filter);
	~Filter() override;

	bool load(Inkscape::Extension::Extension *module) override;
	Inkscape::Extension::Implementation::ImplementationDocumentCache * newDocCache (Inkscape::Extension::Extension * ext, Inkscape::UI::View::View * doc) override;
	void effect(Inkscape::Extension::Effect *module, Inkscape::UI::View::View *document, Inkscape::Extension::Implementation::ImplementationDocumentCache * docCache) override;

	static void filter_init(gchar const * id, gchar const * name, gchar const * submenu, gchar const * tip, gchar const * filter);
	static void filters_all();

	/* File loader related */
	static void filters_all_files();
	static void filters_load_node(Inkscape::XML::Node *node, gchar * menuname);

};

}; /* namespace Filter */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */

#endif // INKSCAPE_EXTENSION_INTERNAL_FILTER_FILTER_H
