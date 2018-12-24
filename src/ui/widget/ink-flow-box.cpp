// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Notebook page widget.
 *
 * Author:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "preferences.h"
#include "ui/widget/ink-flow-box.h"
#include "ui/icon-loader.h"
#include <gtkmm/adjustment.h>

namespace Inkscape {
namespace UI {
namespace Widget {

InkFlowBox::InkFlowBox(const gchar * name)
{
    set_name(name);
    //_flowbox.set_homogeneous();
    this->pack_start(_controller, false, false, 0);
    this->pack_start(_flowbox, true, true, 0);
    _flowbox.set_activate_on_single_click(true);
    Gtk::ToggleButton *tbutton = new Gtk::ToggleButton("", false);
    tbutton->set_always_show_image(true);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    tbutton->set_active(prefs->getBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock/"), true));
    Glib::ustring iconname = "object-unlocked";
    if(tbutton->get_active()) {
        iconname = "object-locked";
    }
    tbutton->set_image(*sp_get_icon_image(iconname, Gtk::ICON_SIZE_MENU));
    tbutton->signal_toggled().connect(sigc::bind< Gtk::ToggleButton* >(sigc::mem_fun (*this, &InkFlowBox::on_global_toggle), tbutton) );
    _controller.pack_start(*tbutton);
    tbutton->show();
    showing = 0;
    sensitive = true;
}

InkFlowBox::~InkFlowBox() {}

Glib::ustring
InkFlowBox::getPrefsPath(gint pos) {
    return Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/index_") + std::to_string(pos);
}

bool 
InkFlowBox::on_filter(Gtk::FlowBoxChild* child) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if(prefs->getBool(getPrefsPath(child->get_index()), true)) {
        showing ++;
        return true;
    }
    return false;
}

void 
InkFlowBox::on_toggle(gint pos, Gtk::ToggleButton *tbutton) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool global = prefs->getBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock"), true);
    if (global && sensitive) {
        sensitive = false;
        bool active = true;
        for (auto child:tbutton->get_parent()->get_children()) {
            if (tbutton != child) {
                dynamic_cast<Gtk::ToggleButton *>(child)->set_active(active);
                active = false;
            }
        }
        prefs->setBool(getPrefsPath(pos), true);
        tbutton->set_active(true);
        sensitive = true;
    } else {
        prefs->setBool(getPrefsPath(pos), tbutton->get_active());
    }
    showing = 0;
    _flowbox.set_filter_func(sigc::mem_fun (*this, &InkFlowBox::on_filter));
    _flowbox.set_max_children_per_line(showing);
}

void 
InkFlowBox::on_global_toggle(Gtk::ToggleButton *tbutton) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock"), tbutton->get_active());
    sensitive = true;
    if (tbutton->get_active()) {
        sensitive = false;
        bool active = true;
        for (auto child:tbutton->get_parent()->get_children()) {
            if (tbutton != child) {
                dynamic_cast<Gtk::ToggleButton *>(child)->set_active(active);
                active = false;
            }
        }
    }
    Glib::ustring iconname = "object-unlocked";
    if(tbutton->get_active()) {
        iconname = "object-locked";
    }
    tbutton->set_image(*sp_get_icon_image(iconname, Gtk::ICON_SIZE_MENU));
    sensitive = true;
}

void 
InkFlowBox::insert(Gtk::Widget *widget, Glib::ustring label, gint pos, bool active, int minwidth){
    Gtk::ToggleButton *tbutton = new Gtk::ToggleButton(label, true);
    tbutton->set_active(active);
    tbutton->signal_toggled().connect(sigc::bind<gint, Gtk::ToggleButton* >(sigc::mem_fun (*this, &InkFlowBox::on_toggle),pos, tbutton) );
    _controller.pack_start(*tbutton);
    tbutton->show();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(getPrefsPath(pos), active);
    widget->set_size_request(minwidth,-1);
    _flowbox.insert(*widget, pos);
    showing = 0;
    _flowbox.set_filter_func(sigc::mem_fun (*this, &InkFlowBox::on_filter) );
    _flowbox.set_max_children_per_line(showing);
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
