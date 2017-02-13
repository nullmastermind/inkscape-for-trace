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
{
    // printf("Building Description\n");
    const char * defaultval = NULL;
    if (xml->firstChild() != NULL) {
        defaultval = xml->firstChild()->content();
    }

    if (defaultval != NULL) {
        _value = g_strdup(defaultval);
    }

    _context = xml->attribute("msgctxt");

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

    Glib::ustring newguitext;

    if (_context != NULL) {
        newguitext = g_dpgettext2(NULL, _context, _value);
    } else {
        newguitext = _(_value);
    }

    Gtk::Label * label = Gtk::manage(new Gtk::Label());
    if (_mode == HEADER) {
        label->set_markup(Glib::ustring("<b>") + Glib::Markup::escape_text(newguitext) + Glib::ustring("</b>"));
        label->set_margin_top(5);
        label->set_margin_bottom(5);
    } else {
        label->set_text(newguitext);
    }
    label->set_line_wrap();
    //label->set_xalign(0); // requires gtkmm 3.16
    label->set_alignment(Gtk::ALIGN_START);

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
