// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Tweak aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "tweak-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"

#include "ui/icon-names.h"
#include "ui/tools/tweak-tool.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/spin-button-tool-item.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {
TweakToolbar::TweakToolbar(SPDesktop *desktop)
        : Toolbar(desktop)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* Width */
    {
        std::vector<Glib::ustring> labels = {_("(pinch tweak)"), "", "", "", _("(default)"), "", "", "", "", _("(broad tweak)")};
        std::vector<double>        values = {                 1,  3,  5, 10,             15, 20, 30, 50, 75,                100};

        auto width_val = prefs->getDouble("/tools/tweak/width", 15);
        _width_adj = Gtk::Adjustment::create(width_val * 100, 1, 100, 1.0, 10.0);
        _width_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("tweak-width", _("Width:"), _width_adj, 0.01, 0));
        _width_item->set_tooltip_text(_("The width of the tweak area (relative to the visible canvas area)"));
        _width_item->set_custom_numeric_menu_data(values, labels);
        _width_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _width_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TweakToolbar::width_value_changed));
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*_width_item);
        _width_item->set_sensitive(true);
    }

    // Force
    {
        std::vector<Glib::ustring> labels = {_("(minimum force)"), "", "", _("(default)"), "", "", "", _("(maximum force)")};
        std::vector<double>        values = {                   1,  5, 10,             20, 30, 50, 70,                  100};
        auto force_val = prefs->getDouble("/tools/tweak/force", 20);
        _force_adj = Gtk::Adjustment::create(force_val * 100, 1, 100, 1.0, 10.0);
        _force_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("tweak-force", _("Force:"), _force_adj, 0.01, 0));
        _force_item->set_tooltip_text(_("The force of the tweak action"));
        _force_item->set_custom_numeric_menu_data(values, labels);
        _force_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _force_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TweakToolbar::force_value_changed));
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*_force_item);
        _force_item->set_sensitive(true);
    }

    /* Use Pressure button */
    {
        _pressure_item = add_toggle_button(_("Pressure"),
                                           _("Use the pressure of the input device to alter the force of tweak action"));
        _pressure_item->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
        _pressure_item->signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::pressure_state_changed));
        _pressure_item->set_active(prefs->getBool("/tools/tweak/usepressure", true));
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Mode */
    {
        add_label(_("Mode:"));
        Gtk::RadioToolButton::Group mode_group;

        auto mode_move_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Move mode")));
        mode_move_btn->set_tooltip_text(_("Move objects in any direction"));
        mode_move_btn->set_icon_name(INKSCAPE_ICON("object-tweak-push"));
        _mode_buttons.push_back(mode_move_btn);

        auto mode_inout_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Move in/out mode")));
        mode_inout_btn->set_tooltip_text(_("Move objects towards cursor; with Shift from cursor"));
        mode_inout_btn->set_icon_name(INKSCAPE_ICON("object-tweak-attract"));
        _mode_buttons.push_back(mode_inout_btn);

        auto mode_jitter_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Move jitter mode")));
        mode_jitter_btn->set_tooltip_text(_("Move objects in random directions"));
        mode_jitter_btn->set_icon_name(INKSCAPE_ICON("object-tweak-randomize"));
        _mode_buttons.push_back(mode_jitter_btn);

        auto mode_scale_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Scale mode")));
        mode_scale_btn->set_tooltip_text(_("Shrink objects, with Shift enlarge"));
        mode_scale_btn->set_icon_name(INKSCAPE_ICON("object-tweak-shrink"));
        _mode_buttons.push_back(mode_scale_btn);

        auto mode_rotate_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Rotate mode")));
        mode_rotate_btn->set_tooltip_text(_("Rotate objects, with Shift counterclockwise"));
        mode_rotate_btn->set_icon_name(INKSCAPE_ICON("object-tweak-rotate"));
        _mode_buttons.push_back(mode_rotate_btn);

        auto mode_dupdel_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Duplicate/delete mode")));
        mode_dupdel_btn->set_tooltip_text(_("Duplicate objects, with Shift delete"));
        mode_dupdel_btn->set_icon_name(INKSCAPE_ICON("object-tweak-duplicate"));
        _mode_buttons.push_back(mode_dupdel_btn);

        auto mode_push_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Push mode")));
        mode_push_btn->set_tooltip_text(_("Push parts of paths in any direction"));
        mode_push_btn->set_icon_name(INKSCAPE_ICON("path-tweak-push"));
        _mode_buttons.push_back(mode_push_btn);

        auto mode_shrinkgrow_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Shrink/grow mode")));
        mode_shrinkgrow_btn->set_tooltip_text(_("Shrink (inset) parts of paths; with Shift grow (outset)"));
        mode_shrinkgrow_btn->set_icon_name(INKSCAPE_ICON("path-tweak-shrink"));
        _mode_buttons.push_back(mode_shrinkgrow_btn);

        auto mode_attrep_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Attract/repel mode")));
        mode_attrep_btn->set_tooltip_text(_("Attract parts of paths towards cursor; with Shift from cursor"));
        mode_attrep_btn->set_icon_name(INKSCAPE_ICON("path-tweak-attract"));
        _mode_buttons.push_back(mode_attrep_btn);

        auto mode_roughen_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Roughen mode")));
        mode_roughen_btn->set_tooltip_text(_("Roughen parts of paths"));
        mode_roughen_btn->set_icon_name(INKSCAPE_ICON("path-tweak-roughen"));
        _mode_buttons.push_back(mode_roughen_btn);

        auto mode_colpaint_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Color paint mode")));
        mode_colpaint_btn->set_tooltip_text(_("Paint the tool's color upon selected objects"));
        mode_colpaint_btn->set_icon_name(INKSCAPE_ICON("object-tweak-paint"));
        _mode_buttons.push_back(mode_colpaint_btn);

        auto mode_coljitter_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Color jitter mode")));
        mode_coljitter_btn->set_tooltip_text(_("Jitter the colors of selected objects"));
        mode_coljitter_btn->set_icon_name(INKSCAPE_ICON("object-tweak-jitter-color"));
        _mode_buttons.push_back(mode_coljitter_btn);

        auto mode_blur_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Blur mode")));
        mode_blur_btn->set_tooltip_text(_("Blur selected objects more; with Shift, blur less"));
        mode_blur_btn->set_icon_name(INKSCAPE_ICON("object-tweak-blur"));
        _mode_buttons.push_back(mode_blur_btn);

        int btn_idx = 0;

        for (auto btn : _mode_buttons) {
            btn->set_sensitive();
            add(*btn);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &TweakToolbar::mode_changed), btn_idx++));
        }
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    guint mode = prefs->getInt("/tools/tweak/mode", 0);

    /* Fidelity */
    {
        std::vector<Glib::ustring> labels = {_("(rough, simplified)"), "", "", _("(default)"), "", "", _("(fine, but many nodes)")};
        std::vector<double>        values = {                      10, 25, 35,             50, 60, 80,                         100};

        auto fidelity_val = prefs->getDouble("/tools/tweak/fidelity", 50);
        _fidelity_adj = Gtk::Adjustment::create(fidelity_val * 100, 1, 100, 1.0, 10.0);
        _fidelity_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("tweak-fidelity", _("Fidelity:"), _fidelity_adj, 0.01, 0));
        _fidelity_item->set_tooltip_text(_("Low fidelity simplifies paths; high fidelity preserves path features but may generate a lot of new nodes"));
        _fidelity_item->set_custom_numeric_menu_data(values, labels);
        _fidelity_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _fidelity_adj->signal_value_changed().connect(sigc::mem_fun(*this, &TweakToolbar::fidelity_value_changed));
        add(*_fidelity_item);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        _channels_label = Gtk::manage(new UI::Widget::LabelToolItem(_("Channels:")));
        _channels_label->set_use_markup(true);
        add(*_channels_label);
    }

    {
        //TRANSLATORS:  "H" here stands for hue
        _doh_item = add_toggle_button(C_("Hue", "H"),
                                      _("In color mode, act on object's hue"));
        _doh_item->signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_doh));
        _doh_item->set_active(prefs->getBool("/tools/tweak/doh", true));
    }
    {
        //TRANSLATORS:  "S" here stands for saturation
        _dos_item = add_toggle_button(C_("Saturation", "S"),
                                      _("In color mode, act on object's saturation"));
        _dos_item->signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_dos));
        _dos_item->set_active(prefs->getBool("/tools/tweak/dos", true));
    }
    {
        //TRANSLATORS:  "S" here stands for saturation
        _dol_item = add_toggle_button(C_("Lightness", "L"),
                                      _("In color mode, act on object's lightness"));
        _dol_item->signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_dol));
        _dol_item->set_active(prefs->getBool("/tools/tweak/dol", true));
    }
    {
        //TRANSLATORS:  "O" here stands for opacity
        _doo_item = add_toggle_button(C_("Opacity", "O"),
                                      _("In color mode, act on object's opacity"));
        _doo_item->signal_toggled().connect(sigc::mem_fun(*this, &TweakToolbar::toggle_doo));
        _doo_item->set_active(prefs->getBool("/tools/tweak/doo", true));
    }

    _mode_buttons[mode]->set_active();
    show_all();

    // Elements must be hidden after show_all() is called
    if (mode == Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT || mode == Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER) {
        _fidelity_item->set_visible(false);
    } else {
        _channels_label->set_visible(false);
        _doh_item->set_visible(false);
        _dos_item->set_visible(false);
        _dol_item->set_visible(false);
        _doo_item->set_visible(false);
    }
}

void
TweakToolbar::set_mode(int mode)
{
    _mode_buttons[mode]->set_active();
}

GtkWidget *
TweakToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new TweakToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
TweakToolbar::width_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/tweak/width",
            _width_adj->get_value() * 0.01 );
}

void
TweakToolbar::force_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/tweak/force",
            _force_adj->get_value() * 0.01 );
}

void
TweakToolbar::mode_changed(int mode)
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/tweak/mode", mode);

    bool flag = ((mode == Inkscape::UI::Tools::TWEAK_MODE_COLORPAINT) ||
                 (mode == Inkscape::UI::Tools::TWEAK_MODE_COLORJITTER));

    _doh_item->set_visible(flag);
    _dos_item->set_visible(flag);
    _dol_item->set_visible(flag);
    _doo_item->set_visible(flag);
    _channels_label->set_visible(flag);

    if (_fidelity_item) {
        _fidelity_item->set_visible(!flag);
    }
}

void
TweakToolbar::fidelity_value_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/tweak/fidelity",
            _fidelity_adj->get_value() * 0.01 );
}

void
TweakToolbar::pressure_state_changed()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/usepressure", _pressure_item->get_active());
}

void
TweakToolbar::toggle_doh() {
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/doh", _doh_item->get_active());
}

void
TweakToolbar::toggle_dos() {
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/dos", _dos_item->get_active());
}

void
TweakToolbar::toggle_dol() {
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/dol", _dol_item->get_active());
}

void
TweakToolbar::toggle_doo() {
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/tweak/doo", _doo_item->get_active());
}

}
}
}

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
