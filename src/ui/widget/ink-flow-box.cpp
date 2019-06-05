// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkflow-box widget.
 * This widget allow pack widgets in a flowbox with a controller to show-hide
 *
 * Author:
 *   Jabier Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2018 Jabier Arraiza
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "preferences.h"
#include "ui/icon-loader.h"
#include "ui/widget/ink-flow-box.h"
#include <gtkmm/adjustment.h>

namespace Inkscape {
namespace UI {
namespace Widget {

InkFlowBox::InkFlowBox(const gchar *name)
{
    set_name(name);
    this->pack_start(_controller, false, false, 0);
    this->pack_start(_flowbox, true, true, 0);
    _flowbox.set_activate_on_single_click(true);
    Gtk::ToggleButton *tbutton = new Gtk::ToggleButton("", false);
    tbutton->set_always_show_image(true);
    _flowbox.set_selection_mode(Gtk::SelectionMode::SELECTION_NONE);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock"), false);
    tbutton->set_active(prefs->getBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock"), true));
    Glib::ustring iconname = "object-unlocked";
    if (tbutton->get_active()) {
        iconname = "object-locked";
    }
    tbutton->set_image(*sp_get_icon_image(iconname, Gtk::ICON_SIZE_MENU));
    tbutton->signal_toggled().connect(
        sigc::bind<Gtk::ToggleButton *>(sigc::mem_fun(*this, &InkFlowBox::on_global_toggle), tbutton));
    _controller.pack_start(*tbutton);
    tbutton->hide();
    tbutton->set_no_show_all(true);
    showing = 0;
    sensitive = true;
}

InkFlowBox::~InkFlowBox() = default;

Glib::ustring InkFlowBox::getPrefsPath(gint pos)
{
    return Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/index_") + std::to_string(pos);
}

bool InkFlowBox::on_filter(Gtk::FlowBoxChild *child)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool(getPrefsPath(child->get_index()), true)) {
        showing++;
        return true;
    }
    return false;
}

void InkFlowBox::on_toggle(gint pos, Gtk::ToggleButton *tbutton)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool global = prefs->getBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock"), true);
    if (global && sensitive) {
        sensitive = false;
        bool active = true;
        for (auto child : tbutton->get_parent()->get_children()) {
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
    _flowbox.set_filter_func(sigc::mem_fun(*this, &InkFlowBox::on_filter));
    _flowbox.set_max_children_per_line(showing);
}

void InkFlowBox::on_global_toggle(Gtk::ToggleButton *tbutton)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(Glib::ustring("/dialogs/") + get_name() + Glib::ustring("/flowbox/lock"), tbutton->get_active());
    sensitive = true;
    if (tbutton->get_active()) {
        sensitive = false;
        bool active = true;
        for (auto child : tbutton->get_parent()->get_children()) {
            if (tbutton != child) {
                dynamic_cast<Gtk::ToggleButton *>(child)->set_active(active);
                active = false;
            }
        }
    }
    Glib::ustring iconname = "object-unlocked";
    if (tbutton->get_active()) {
        iconname = "object-locked";
    }
    tbutton->set_image(*sp_get_icon_image(iconname, Gtk::ICON_SIZE_MENU));
    sensitive = true;
}

void InkFlowBox::insert(Gtk::Widget *widget, Glib::ustring label, gint pos, bool active, int minwidth)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Gtk::ToggleButton *tbutton = new Gtk::ToggleButton(label, true);
    tbutton->set_active(prefs->getBool(getPrefsPath(pos), active));
    tbutton->signal_toggled().connect(
        sigc::bind<gint, Gtk::ToggleButton *>(sigc::mem_fun(*this, &InkFlowBox::on_toggle), pos, tbutton));
    _controller.pack_start(*tbutton);
    tbutton->show();
    prefs->setBool(getPrefsPath(pos), prefs->getBool(getPrefsPath(pos), active));
    widget->set_size_request(minwidth, -1);
    _flowbox.insert(*widget, pos);
    showing = 0;
    _flowbox.set_filter_func(sigc::mem_fun(*this, &InkFlowBox::on_filter));
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
