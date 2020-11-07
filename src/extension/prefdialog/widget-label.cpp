// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Description widget for extensions
 *//*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2005-2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "widget-label.h"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <glibmm/markup.h>
#include <glibmm/regex.h>

#include "xml/node.h"
#include "extension/extension.h"

namespace Inkscape {
namespace Extension {


WidgetLabel::WidgetLabel(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxWidget(xml, ext)
{
    // construct the text content by concatenating all (non-empty) text nodes,
    // removing all other nodes (e.g. comment nodes) and replacing <extension:br> elements with "<br/>"
    Inkscape::XML::Node * cur_child = xml->firstChild();
    while (cur_child != nullptr) {
        if (cur_child->type() == XML::NodeType::TEXT_NODE && cur_child->content() != nullptr) {
            _value += cur_child->content();
        } else if (cur_child->type() == XML::NodeType::ELEMENT_NODE && !g_strcmp0(cur_child->name(), "extension:br")) {
            _value += "<br/>";
        }
        cur_child = cur_child->next();
    }

    // do replacements in the source string to account for the attribute xml:space="preserve"
    // (those should match replacements potentially performed by xgettext to allow for proper translation)
    if (g_strcmp0(xml->attribute("xml:space"), "preserve") == 0) {
        // xgettext copies the source string verbatim in this case, so no changes needed
    } else {
        // remove all whitespace from start/end of string and replace intermediate whitespace with a single space
        _value = Glib::Regex::create("^\\s+|\\s+$")->replace_literal(_value, 0, "", (Glib::RegexMatchFlags)0);
        _value = Glib::Regex::create("\\s+")->replace_literal(_value, 0, " ", (Glib::RegexMatchFlags)0);
    }

    // translate value
    if (!_value.empty()) {
        if (_translatable != NO) { // translate unless explicitly marked untranslatable
            _value = get_translation(_value.c_str());
        }
    }

    // finally replace all remaining <br/> with a real newline character
    _value = Glib::Regex::create("<br/>")->replace_literal(_value, 0, "\n", (Glib::RegexMatchFlags)0);

    // parse appearance
    if (_appearance) {
        if (!strcmp(_appearance, "header")) {
            _mode = HEADER;
        } else if (!strcmp(_appearance, "url")) {
            _mode = URL;
        } else {
            g_warning("Invalid value ('%s') for appearance of label widget in extension '%s'",
                      _appearance, _extension->get_id());
        }
    }
}

/** \brief  Create a label for the description */
Gtk::Widget *WidgetLabel::get_widget(sigc::signal<void> * /*changeSignal*/)
{
    if (_hidden) {
        return nullptr;
    }

    Glib::ustring newtext = _value;

    Gtk::Label *label = Gtk::manage(new Gtk::Label());
    if (_mode == HEADER) {
        label->set_markup(Glib::ustring("<b>") + Glib::Markup::escape_text(newtext) + Glib::ustring("</b>"));
        label->set_margin_top(5);
        label->set_margin_bottom(5);
    } else if (_mode == URL) {
        Glib::ustring escaped_url = Glib::Markup::escape_text(newtext);
        label->set_markup(Glib::ustring::compose("<a href='%1'>%1</a>", escaped_url));
    } else {
        label->set_text(newtext);
    }
    label->set_line_wrap();
    label->set_xalign(0);

    // TODO: Ugly "fix" for gtk3 width/height calculation of labels.
    //   - If not applying any limits long labels will make the window grow horizontally until it uses up
    //     most of the available space (i.e. most of the screen area) which is ridiculously wide.
    //   - By using "set_default_size(0,0)" in prefidalog.cpp we tell the window to shrink as much as possible,
    //     however this can result in a much too narrow dialog instead and a lot of unnecessary wrapping.
    //   - Here we set a lower limit of GUI_MAX_LINE_LENGTH characters per line that long texts will always use.
    //     This means texts can not shrink anymore (they can still grow, though) and it's also necessary
    //     to prevent https://bugzilla.gnome.org/show_bug.cgi?id=773572
    int len = newtext.length();
    label->set_width_chars(len > GUI_MAX_LINE_LENGTH ? GUI_MAX_LINE_LENGTH : len);

    label->show();

    Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    hbox->pack_start(*label, true, true);
    hbox->show();

    return hbox;
}

}  /* namespace Extension */
}  /* namespace Inkscape */
