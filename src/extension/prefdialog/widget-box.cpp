// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Box widget for extensions
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "widget-box.h"

#include <gtkmm/box.h>

#include "xml/node.h"
#include "extension/extension.h"

namespace Inkscape {
namespace Extension {


WidgetBox::WidgetBox(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxWidget(xml, ext)
{
    // Decide orientation based on tagname (hbox vs. vbox)
    const char *tagname = xml->name();
    if (!strncmp(tagname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
        tagname += strlen(INKSCAPE_EXTENSION_NS);
    }
    if (!strcmp(tagname, "hbox")) {
        _orientation = HORIZONTAL;
    } else if (!strcmp(tagname, "vbox")) {
        _orientation = VERTICAL;
    } else {
        g_assert_not_reached();
    }

    // Read XML tree of box and parse child widgets
    if (xml) {
        Inkscape::XML::Node *child_repr = xml->firstChild();
        while (child_repr) {
            const char *chname = child_repr->name();
            if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
                chname += strlen(INKSCAPE_EXTENSION_NS);
            }
            if (chname[0] == '_') { // allow leading underscore in tag names for backwards-compatibility
                chname++;
            }

            if (InxWidget::is_valid_widget_name(chname)) {
                InxWidget *widget = InxWidget::make(child_repr, _extension);
                if (widget) {
                    _children.push_back(widget);
                }
            } else if (child_repr->type() == XML::ELEMENT_NODE) {
                g_warning("Invalid child element ('%s') in box widget in extension '%s'.",
                          chname, _extension->get_id());
            } else if (child_repr->type() != XML::COMMENT_NODE){
                g_warning("Invalid child element found in box widget in extension '%s'.", _extension->get_id());
            }

            child_repr = child_repr->next();
        }
    }
}

Gtk::Widget *WidgetBox::get_widget(sigc::signal<void> *changeSignal)
{
    if (_hidden) {
        return nullptr;
    }

    Gtk::Orientation orientation;
    if (_orientation == HORIZONTAL) {
        orientation = Gtk::ORIENTATION_HORIZONTAL;
    } else {
        orientation = Gtk::ORIENTATION_VERTICAL;
    }

    Gtk::Box *box = Gtk::manage(new Gtk::Box(orientation));
    // box->set_border_width(GUI_BOX_MARGIN); // leave at zero for now, so box is purely for layouting (not grouping)
                                              // revisit this later, possibly implementing GtkFrame or similar
    box->set_spacing(GUI_BOX_SPACING);

    if (_orientation == HORIZONTAL) {
        box->set_vexpand(false);
    } else {
        box->set_hexpand(false);
    }

    // add child widgets onto page (if any)
    for (auto child : _children) {
        Gtk::Widget *child_widget = child->get_widget(changeSignal);
        if (child_widget) {
            int indent = child->get_indent();
            child_widget->set_margin_start(indent * GUI_INDENTATION);
            box->pack_start(*child_widget, false, true, 0); // fill=true does not have an effect here, but allows the
                                                            // child to choose to expand by setting hexpand/vexpand

            const char *tooltip = child->get_tooltip();
            if (tooltip) {
                child_widget->set_tooltip_text(tooltip);
            }
        }
    }

    box->show();

    return dynamic_cast<Gtk::Widget *>(box);
}

}  /* namespace Extension */
}  /* namespace Inkscape */
