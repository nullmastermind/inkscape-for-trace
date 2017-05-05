/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef linux  // does the dollar sign need escaping when passed as string parameter?
# define ESCAPE_DOLLAR_COMMANDLINE
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <glibmm/regex.h>

#include "xml/node.h"
#include "extension/extension.h"
#include "description.h"

namespace Inkscape {
namespace Extension {


/** \brief  Initialize the object, to do that, copy the data. */
ParamDescription::ParamDescription(const gchar * name,
                                   const gchar * text,
                                   const gchar * description,
                                   bool hidden,
                                   int indent,
                                   Inkscape::Extension::Extension * ext,
                                   Inkscape::XML::Node * xml,
                                   AppearanceMode mode)
    : Parameter(name, text, description, hidden, indent, ext)
    , _value(NULL)
    , _mode(mode)
{
    // construct the text content by concatenating all (non-empty) text nodes,
    // removing all other nodes (e.g. comment nodes) and replacing <extension:br> elements with "<br/>"
    Glib::ustring value;
    Inkscape::XML::Node * cur_child = xml->firstChild();
    while (cur_child != NULL) {
        if (cur_child->type() == XML::TEXT_NODE && cur_child->content() != NULL) {
            value += cur_child->content();
        } else if (cur_child->type() == XML::ELEMENT_NODE && !g_strcmp0(cur_child->name(), "extension:br")) {
            value += "<br/>";
        }
        cur_child = cur_child->next();
    }

    // if there is no text content we can return immediately (the description will be invisible)
    if (value == Glib::ustring("")) {
        return;
    }

    // do replacements in the source string to account for the attribute xml:space="preserve"
    // (those should match replacements potentially performed by xgettext to allow for proper translation)
    if (g_strcmp0(xml->attribute("xml:space"), "preserve") == 0) {
        // xgettext copies the source string verbatim in this case, so no changes needed
    } else {
        // remove all whitespace from start/end of string and replace intermediate whitespace with a single space
        value = Glib::Regex::create("^\\s+|\\s+$")->replace_literal(value, 0, "", (Glib::RegexMatchFlags)0);
        value = Glib::Regex::create("\\s+")->replace_literal(value, 0, " ", (Glib::RegexMatchFlags)0);
    }

    // translate if underscored version (_param) was used
    if (g_str_has_prefix(xml->name(), "extension:_")) {
        const gchar * context = xml->attribute("msgctxt");
        if (context != NULL) {
            value = g_dpgettext2(NULL, context, value.c_str());
        } else {
            value = _(value.c_str());
        }
    }

    // finally replace all remaining <br/> with a real newline character
    value = Glib::Regex::create("<br/>")->replace_literal(value, 0, "\n", (Glib::RegexMatchFlags)0);

    _value = g_strdup(value.c_str());

    return;
}

/** \brief  Create a label for the description */
Gtk::Widget *
ParamDescription::get_widget (SPDocument * /*doc*/, Inkscape::XML::Node * /*node*/, sigc::signal<void> * /*changeSignal*/)
{
    if (_hidden) {
        return NULL;
    }
    if (_value == NULL) {
        return NULL;
    }

    Glib::ustring newtext = _value;

    Gtk::Label * label = Gtk::manage(new Gtk::Label());
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
#if (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 16)
    label->set_xalign(0);
#else
    label->set_alignment(Gtk::ALIGN_START);
#endif

    // TODO: Ugly "fix" for gtk3 width/height calculation of labels.
    //   - If not applying any limits long labels will make the window grow horizontally until it uses up
    //     most of the available space (i.e. most of the screen area) which is ridicously wide
    //   - By using "set_default_size(0,0)" in prefidalog.cpp we tell the window to shrink as much as possible,
    //     however this can result in a much to narrow dialog instead and much unnecessary wrapping
    //   - Here we set a lower limit of GUI_MAX_LINE_LENGTH characters per line that long texts will always use
    //     This means texts can not shrink anymore (they can still grow, though) and it's also necessary
    //     to prevent https://bugzilla.gnome.org/show_bug.cgi?id=773572
    int len = newtext.length();
    label->set_width_chars(len > Parameter::GUI_MAX_LINE_LENGTH ? Parameter::GUI_MAX_LINE_LENGTH : len);

    label->show();

    Gtk::HBox * hbox = Gtk::manage(new Gtk::HBox());
    hbox->pack_start(*label, true, true);
    hbox->show();

    return hbox;
}

}  /* namespace Extension */
}  /* namespace Inkscape */
