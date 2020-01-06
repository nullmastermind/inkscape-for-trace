// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Parameters for extensions.
 *//*
 * Author:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"
#include "widget.h"
#include "widget-box.h"
#include "widget-image.h"
#include "widget-label.h"
#include "widget-separator.h"
#include "widget-spacer.h"

#include <algorithm>
#include <cstring>

#include <sigc++/sigc++.h>

#include "extension/extension.h"

#include "xml/node.h"


namespace Inkscape {
namespace Extension {

InxWidget *InxWidget::make(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *in_ext)
{
    InxWidget *widget = nullptr;

    const char *name = in_repr->name();
    if (!strncmp(name, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
        name += strlen(INKSCAPE_EXTENSION_NS);
    }
    if (name[0] == '_') { // allow leading underscore in tag names for backwards-compatibility
        name++;
    }

    // decide on widget type based on tag name
    // keep in sync with list of names supported in InxWidget::is_valid_widget_name() below
    if (!name) {
        // we can't create a widget without name
        g_warning("InxWidget without name in extension '%s'.", in_ext->get_id());
    } else if (!strcmp(name, "hbox") || !strcmp(name, "vbox")) {
        widget = new WidgetBox(in_repr, in_ext);
    } else if (!strcmp(name, "image")) {
        widget = new WidgetImage(in_repr, in_ext);
    } else if (!strcmp(name, "label")) {
        widget = new WidgetLabel(in_repr, in_ext);
    } else if (!strcmp(name, "separator")) {
        widget = new WidgetSeparator(in_repr, in_ext);
    } else if (!strcmp(name, "spacer")) {
        widget = new WidgetSpacer(in_repr, in_ext);
    } else if (!strcmp(name, "param")) {
        widget = InxParameter::make(in_repr, in_ext);
    } else {
        g_warning("Unknown widget name ('%s') in extension '%s'", name, in_ext->get_id());
    }

    // Note: widget could equal nullptr
    return widget;
}

bool InxWidget::is_valid_widget_name(const char *name)
{
    // keep in sync with names supported in InxWidget::make() above
    static const std::vector<std::string> valid_names =
        {"hbox", "vbox", "image", "label", "separator", "spacer", "param"};

    if (std::find(valid_names.begin(), valid_names.end(), name) != valid_names.end()) {
        return true;
    } else {
        return false;
    }
}


InxWidget::InxWidget(Inkscape::XML::Node *in_repr, Inkscape::Extension::Extension *ext)
    : _extension(ext)
{
    // translatable (optional)
    const char *translatable = in_repr->attribute("translatable");
    if (translatable) {
        if (!strcmp(translatable, "yes")) {
            _translatable = YES;
        } else if (!strcmp(translatable, "no"))  {
            _translatable = NO;
        } else {
            g_warning("Invalid value ('%s') for translatable attribute of widget '%s' in extension '%s'",
                      translatable, in_repr->name(), _extension->get_id());
        }
    }

    // context (optional)
    const char *context = in_repr->attribute("context");
    if (!context) {
        context = in_repr->attribute("msgctxt"); // backwards-compatibility with previous name
    }
    if (context) {
        _context = g_strdup(context);
    }

    // gui-hidden (optional)
    const char *gui_hidden = in_repr->attribute("gui-hidden");
    if (gui_hidden != nullptr) {
        if (strcmp(gui_hidden, "true") == 0) {
            _hidden = true;
        }
    }

    // indent (optional)
    const char *indent = in_repr->attribute("indent");
    if (indent != nullptr) {
        _indent = strtol(indent, nullptr, 0);
    }

    // appearance (optional, does not apply to all parameters)
    const char *appearance = in_repr->attribute("appearance");
    if (appearance) {
        _appearance = g_strdup(appearance);
    }
}

InxWidget::~InxWidget()
{
    for (auto child : _children) {
        delete child;
    }

    g_free(_context);
    _context = nullptr;

    g_free(_appearance);
    _appearance = nullptr;
}

Gtk::Widget *
InxWidget::get_widget(sigc::signal<void> * /*changeSignal*/)
{
    // if we end up here we're missing a definition of ::get_widget() in one of the subclasses
    g_critical("InxWidget::get_widget called from widget of type '%s' in extension '%s'",
               typeid(this).name(), _extension->get_id());
    g_assert_not_reached();
    return nullptr;
}

const char *InxWidget::get_translation(const char* msgid) {
    return _extension->get_translation(msgid, _context);
}

void InxWidget::get_widgets(std::vector<InxWidget *> &list)
{
    list.push_back(this);
    for (auto child : _children) {
        child->get_widgets(list);
    }
}

}  // namespace Extension
}  // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
