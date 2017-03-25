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
                                   const gchar * guitext,
                                   const gchar * desc,
                                   const Parameter::_scope_t scope,
                                   bool gui_hidden,
                                   const gchar * gui_tip,
                                   int indent,
                                   Inkscape::Extension::Extension * ext,
                                   Inkscape::XML::Node * xml,
                                   AppearanceMode mode)
    : Parameter(name, guitext, desc, scope, gui_hidden, gui_tip, indent, ext)
    , _value(NULL)
    , _mode(mode)
    , _preserve_whitespace(false)
{
    Glib::ustring defaultval;
    Inkscape::XML::Node * cur_child = xml->firstChild();
    while (cur_child != NULL) {
        if (cur_child->type() == XML::TEXT_NODE && cur_child->content() != NULL) {
            defaultval += cur_child->content();
        } else if (cur_child->type() == XML::ELEMENT_NODE && !g_strcmp0(cur_child->name(), "extension:br")) {
            defaultval += "<br/>";
        }
        cur_child = cur_child->next();
    }

    if (defaultval != Glib::ustring("")) {
        _value = g_strdup(defaultval.c_str());
    }

    _context = xml->attribute("msgctxt");

    if (g_strcmp0(xml->attribute("xml:space"), "preserve") == 0) {
        _preserve_whitespace = true;
    }

    return;
}

/** \brief  Create a label for the description */
Gtk::Widget *
ParamDescription::get_widget (SPDocument * /*doc*/, Inkscape::XML::Node * /*node*/, sigc::signal<void> * /*changeSignal*/)
{
    if (_gui_hidden) {
        return NULL;
    }
    if (_value == NULL) {
        return NULL;
    }

    Glib::ustring newguitext = _value;

    // do replacements in the source string matching those performed by xgettext to allow for proper translation
    if (_preserve_whitespace) {
        // xgettext copies the source string verbatim in this case, so no changes needed
    } else {
        // remove all whitespace from start/end of string and replace intermediate whitespace with a single space
        newguitext = Glib::Regex::create("^\\s+|\\s+$")->replace_literal(newguitext, 0, "", (Glib::RegexMatchFlags)0);
        newguitext = Glib::Regex::create("\\s+")->replace_literal(newguitext, 0, " ", (Glib::RegexMatchFlags)0);
    }

    // translate
    if (_context != NULL) {
        newguitext = g_dpgettext2(NULL, _context, newguitext.c_str());
    } else {
        newguitext = _(newguitext.c_str());
    }

    // finally replace all remaining <br/> with a real newline character
    newguitext = Glib::Regex::create("<br/>")->replace_literal(newguitext, 0, "\n", (Glib::RegexMatchFlags)0);

    Gtk::Label * label = Gtk::manage(new Gtk::Label());
    if (_mode == HEADER) {
        label->set_markup(Glib::ustring("<b>") + Glib::Markup::escape_text(newguitext) + Glib::ustring("</b>"));
        label->set_margin_top(5);
        label->set_margin_bottom(5);
    } else if (_mode == URL) {
        Glib::ustring escaped_url = Glib::Markup::escape_text(newguitext);
        label->set_markup(Glib::ustring::compose("<a href='%1'>%1</a>", escaped_url));
    } else {
        label->set_text(newguitext);
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
    int len = newguitext.length();
    label->set_width_chars(len > Parameter::GUI_MAX_LINE_LENGTH ? Parameter::GUI_MAX_LINE_LENGTH : len);

    label->show();

    Gtk::HBox * hbox = Gtk::manage(new Gtk::HBox());
    hbox->pack_start(*label, true, true);
    hbox->show();

    return hbox;
}

}  /* namespace Extension */
}  /* namespace Inkscape */
