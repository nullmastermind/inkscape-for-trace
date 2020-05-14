// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2008 Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "filter.h"

#include "io/sys.h"
#include "io/resource.h"
#include "io/stream/inkscapestream.h"

/* Directory includes */
#include "path-prefix.h"
#include "inkscape.h"

/* Extension */
#include "extension/extension.h"
#include "extension/system.h"

/* System includes */
#include <glibmm/i18n.h>
#include <glibmm/fileutils.h>

using namespace Inkscape::IO::Resource;

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Filter {

void
filters_load_file (Glib::ustring filename, gchar * menuname)
{
    Inkscape::XML::Document *doc = sp_repr_read_file(filename.c_str(), INKSCAPE_EXTENSION_URI);
	if (doc == nullptr) {
		g_warning("File (%s) is not parseable as XML.  Ignored.", filename.c_str());
		return;
	}

	Inkscape::XML::Node * root = doc->root();
	if (strcmp(root->name(), "svg:svg")) {
		Inkscape::GC::release(doc);
		g_warning("File (%s) is not SVG.  Ignored.", filename.c_str());
		return;
	}

	for (Inkscape::XML::Node * child = root->firstChild();
			child != nullptr; child = child->next()) {
		if (!strcmp(child->name(), "svg:defs")) {
			for (Inkscape::XML::Node * defs = child->firstChild();
					defs != nullptr; defs = defs->next()) {
				if (!strcmp(defs->name(), "svg:filter")) {
                                    Filter::filters_load_node(defs, menuname);
				} // oh!  a filter
			} //defs
		} // is defs
	} // children of root

	Inkscape::GC::release(doc);
	return;
}

void Filter::filters_all_files()
{
    for(auto &filename: get_filenames(USER, FILTERS, {".svg"})) {
        filters_load_file(filename, _("Personal"));
    }
    for(auto &filename: get_filenames(SYSTEM, FILTERS, {".svg"})) {
        filters_load_file(filename, _("Bundled"));
    }
}


#include "extension/internal/clear-n_.h"

class mywriter : public Inkscape::IO::BasicWriter {
	Glib::ustring _str;
public:
	void close() override;
	void flush() override;
	void put (char ch) override;
	gchar const * c_str () { return _str.c_str(); }
};

void mywriter::close () { return; }
void mywriter::flush () { return; }
void mywriter::put (char ch) { _str += ch; }


void
Filter::filters_load_node (Inkscape::XML::Node *node, gchar * menuname)
{
	gchar const * label = node->attribute("inkscape:label");
	gchar const * menu = node->attribute("inkscape:menu");
	gchar const * menu_tooltip = node->attribute("inkscape:menu-tooltip");
	gchar const * id = node->attribute("id");

	if (label == nullptr) {
		label = id;
	}

    // clang-format off
	gchar * xml_str = g_strdup_printf(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>%s</name>\n"
            "<id>org.inkscape.effect.filter.%s</id>\n"
            "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                    "<submenu name=\"" N_("Filters") "\">\n"
                        "<submenu name=\"%s\"/>\n"
                    "</submenu>\n"
                "</effects-menu>\n"
                "<menu-tip>%s</menu-tip>\n"
            "</effect>\n"
        "</inkscape-extension>\n", label, id, menu? menu : menuname, menu_tooltip? menu_tooltip : label);
    // clang-format on

	// FIXME: Bad hack: since we pull out a single filter node out of SVG file and
	// serialize it, it loses the namespace declarations from the root, so we must provide
	// one right here for our inkscape attributes
	node->setAttribute("xmlns:inkscape", SP_INKSCAPE_NS_URI);

	mywriter writer;
	sp_repr_write_stream(node, writer, 0, FALSE, g_quark_from_static_string("svg"), 0, 0);

    Inkscape::Extension::build_from_mem(xml_str, new Filter(g_strdup(writer.c_str())));
	g_free(xml_str);
    return;
}

}; /* namespace Filter */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */

