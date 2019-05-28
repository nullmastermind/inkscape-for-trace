// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Spray aux toolbar
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
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2015 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spray-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "inkscape.h"

#include "ui/icon-names.h"
#include "ui/simple-pref-pusher.h"

#include "ui/dialog/clonetiler.h"
#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/panel-dialog.h"

#include "ui/widget/spin-button-tool-item.h"

// Disabled in 0.91 because of Bug #1274831 (crash, spraying an object 
// with the mode: spray object in single path)
// Please enable again when working on 1.0
#define ENABLE_SPRAY_MODE_SINGLE_PATH

Inkscape::UI::Dialog::CloneTiler *get_clone_tiler_panel(SPDesktop *desktop)
{
    if (Inkscape::UI::Dialog::PanelDialogBase *panel_dialog =
        dynamic_cast<Inkscape::UI::Dialog::PanelDialogBase *>(desktop->_dlg_mgr->getDialog("CloneTiler"))) {
        try {
            Inkscape::UI::Dialog::CloneTiler &clone_tiler =
                dynamic_cast<Inkscape::UI::Dialog::CloneTiler &>(panel_dialog->getPanel());
            return &clone_tiler;
        } catch (std::exception &e) { }
    }

    return nullptr;
}

namespace Inkscape {
namespace UI {
namespace Toolbar {
SprayToolbar::SprayToolbar(SPDesktop *desktop) :
    Toolbar(desktop)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* Mode */
    {
        add_label(_("Mode:"));

        Gtk::RadioToolButton::Group mode_group;

        auto copy_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Spray with copies")));
        copy_mode_btn->set_tooltip_text(_("Spray copies of the initial selection"));
        copy_mode_btn->set_icon_name(INKSCAPE_ICON("spray-mode-copy"));
        _mode_buttons.push_back(copy_mode_btn);

        auto clone_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Spray with clones")));
        clone_mode_btn->set_tooltip_text(_("Spray clones of the initial selection"));
        clone_mode_btn->set_icon_name(INKSCAPE_ICON("spray-mode-clone"));
        _mode_buttons.push_back(clone_mode_btn);

#ifdef ENABLE_SPRAY_MODE_SINGLE_PATH
        auto union_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Spray single path")));
        union_mode_btn->set_tooltip_text(_("Spray objects in a single path"));
        union_mode_btn->set_icon_name(INKSCAPE_ICON("spray-mode-union"));
        _mode_buttons.push_back(union_mode_btn);
#endif

        auto eraser_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Delete sprayed items")));
        eraser_mode_btn->set_tooltip_text(_("Delete sprayed items from selection"));
        eraser_mode_btn->set_icon_name(INKSCAPE_ICON("draw-eraser"));
        _mode_buttons.push_back(eraser_mode_btn);

        int btn_idx = 0;
        for (auto btn : _mode_buttons) {
            btn->set_sensitive(true);
            add(*btn);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::mode_changed), btn_idx++));
        }
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        /* Width */
        std::vector<Glib::ustring> labels = {_("(narrow spray)"), "", "", "", _("(default)"), "", "", "", "", _("(broad spray)")};
        std::vector<double>        values = {                  1,  3,  5, 10,             15, 20, 30, 50, 75,                100};
        auto width_val = prefs->getDouble("/tools/spray/width", 15);
        _width_adj = Gtk::Adjustment::create(width_val, 1, 100, 1.0, 10.0);
        auto width_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-width", _("Width:"), _width_adj, 1, 0));
        width_item->set_tooltip_text(_("The width of the spray area (relative to the visible canvas area)"));
        width_item->set_custom_numeric_menu_data(values, labels);
        width_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _width_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::width_value_changed));
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*width_item);
        width_item->set_sensitive(true);
    }
    
    /* Use Pressure Width button */
    {
        auto pressure_item = add_toggle_button(_("Pressure"),
                                               _("Use the pressure of the input device to alter the width of spray area"));
        pressure_item->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
        _usepressurewidth_pusher.reset(new UI::SimplePrefPusher(pressure_item, "/tools/spray/usepressurewidth"));
        pressure_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                           pressure_item,
                                                           "/tools/spray/usepressurewidth"));
    }

    {   /* Population */
        std::vector<Glib::ustring> labels = {_("(low population)"), "", "", "", _("(default)"), "", _("(high population)")};
        std::vector<double>        values = {                    5, 20, 35, 50,             70, 85,                    100};
        auto population_val = prefs->getDouble("/tools/spray/population", 70);
        _population_adj = Gtk::Adjustment::create(population_val, 1, 100, 1.0, 10.0);
        _spray_population = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-population", _("Amount:"), _population_adj, 1, 0));
        _spray_population->set_tooltip_text(_("Adjusts the number of items sprayed per click"));
        _spray_population->set_custom_numeric_menu_data(values, labels);
        _spray_population->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _population_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::population_value_changed));
        //ege_adjustment_action_set_appearance( holder->_spray_population, TOOLBAR_SLIDER_HINT );
        add(*_spray_population);
        _spray_population->set_sensitive(true);
    }

    /* Use Pressure Population button */
    {
        auto pressure_population_item = add_toggle_button(_("Pressure"),
                                                          _("Use the pressure of the input device to alter the amount of sprayed objects"));
        pressure_population_item->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
        _usepressurepopulation_pusher.reset(new UI::SimplePrefPusher(pressure_population_item, "/tools/spray/usepressurepopulation"));
        pressure_population_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                                      pressure_population_item,
                                                                      "/tools/spray/usepressurepopulation"));
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {   /* Rotation */
        std::vector<Glib::ustring> labels = {_("(default)"), "", "", "", "", "", "", _("(high rotation variation)")};
        std::vector<double>        values = {             0, 10, 25, 35, 50, 60, 80,                            100};
        auto rotation_val = prefs->getDouble("/tools/spray/rotation_variation", 0);
        _rotation_adj = Gtk::Adjustment::create(rotation_val, 0, 100, 1.0, 10.0);
        _spray_rotation = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-rotation", _("Rotation:"), _rotation_adj, 1, 0));
        // xgettext:no-c-format
        _spray_rotation->set_tooltip_text(_("Variation of the rotation of the sprayed objects; 0% for the same rotation than the original object"));
        _spray_rotation->set_custom_numeric_menu_data(values, labels);
        _spray_rotation->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _rotation_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::rotation_value_changed));
        // ege_adjustment_action_set_appearance(holder->_spray_rotation, TOOLBAR_SLIDER_HINT );
        add(*_spray_rotation);
        _spray_rotation->set_sensitive();
    }

    {   /* Scale */
        std::vector<Glib::ustring> labels = {_("(default)"), "", "", "", "", "", "", _("(high scale variation)")};
        std::vector<double>        values = {             0, 10, 25, 35, 50, 60, 80,                         100};
        auto scale_val = prefs->getDouble("/tools/spray/scale_variation", 0);
        _scale_adj = Gtk::Adjustment::create(scale_val, 0, 100, 1.0, 10.0);
        _spray_scale = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-scale", C_("Spray tool", "Scale:"), _scale_adj, 1, 0));
        // xgettext:no-c-format
        _spray_scale->set_tooltip_text(_("Variation in the scale of the sprayed objects; 0% for the same scale than the original object"));
        _spray_scale->set_custom_numeric_menu_data(values, labels);
        _spray_scale->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _scale_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::scale_value_changed));
        // ege_adjustment_action_set_appearance( holder->_spray_scale, TOOLBAR_SLIDER_HINT );
        add(*_spray_scale);
        _spray_scale->set_sensitive(true);
    }

    /* Use Pressure Scale button */
    {
        _usepressurescale = add_toggle_button(_("Pressure"),
                                              _("Use the pressure of the input device to alter the scale of new items"));
        _usepressurescale->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
        _usepressurescale->set_active(prefs->getBool("/tools/spray/usepressurescale", false));
        _usepressurescale->signal_toggled().connect(sigc::mem_fun(*this, &SprayToolbar::toggle_pressure_scale));
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    {
        /* Standard_deviation */
        std::vector<Glib::ustring> labels = {_("(minimum scatter)"), "", "", "", "", "", _("(default)"), _("(maximum scatter)")};
        std::vector<double>        values = {                     1,  5, 10, 20, 30, 50,             70,                    100};
        auto sd_val = prefs->getDouble("/tools/spray/standard_deviation", 70);
        _sd_adj = Gtk::Adjustment::create(sd_val, 1, 100, 1.0, 10.0);
        auto sd_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-standard-deviation", C_("Spray tool", "Scatter:"), _sd_adj, 1, 0));
        sd_item->set_tooltip_text(_("Increase to scatter sprayed objects"));
        sd_item->set_custom_numeric_menu_data(values, labels);
        sd_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _sd_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::standard_deviation_value_changed));
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*sd_item);
        sd_item->set_sensitive(true);
    }

    {
        /* Mean */
        std::vector<Glib::ustring> labels = {_("(default)"), "", "", "", "", "", "", _("(maximum mean)")};
        std::vector<double>        values = {             0,  5, 10, 20, 30, 50, 70,                 100};
        auto mean_val = prefs->getDouble("/tools/spray/mean", 0);
        _mean_adj = Gtk::Adjustment::create(mean_val, 0, 100, 1.0, 10.0);
        auto mean_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-mean", _("Focus:"), _mean_adj, 1, 0));
        mean_item->set_tooltip_text(_("0 to spray a spot; increase to enlarge the ring radius"));
        mean_item->set_custom_numeric_menu_data(values, labels);
        mean_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _mean_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::mean_value_changed));
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*mean_item);
        mean_item->set_sensitive(true);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Over No Transparent */
    {
        _over_no_transparent = add_toggle_button(_("Apply over no transparent areas"),
                                                 _("Apply over no transparent areas"));
        _over_no_transparent->set_icon_name(INKSCAPE_ICON("object-visible"));
        _over_no_transparent->set_active(prefs->getBool("/tools/spray/over_no_transparent", true));
        _over_no_transparent->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                                  _over_no_transparent,
                                                                  "/tools/spray/over_no_transparent"));
    }

    /* Over Transparent */
    {
        _over_transparent = add_toggle_button(_("Apply over transparent areas"),
                                              _("Apply over transparent areas"));
        _over_transparent->set_icon_name(INKSCAPE_ICON("object-hidden"));
        _over_transparent->set_active(prefs->getBool("/tools/spray/over_transparent", true));
        _over_transparent->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                               _over_transparent,
                                                               "/tools/spray/over_transparent"));
    }

    /* Pick No Overlap */
    {
        _pick_no_overlap = add_toggle_button(_("No overlap between colors"),
                                             _("No overlap between colors"));
        _pick_no_overlap->set_icon_name(INKSCAPE_ICON("symbol-bigger"));
        _pick_no_overlap->set_active(prefs->getBool("/tools/spray/pick_no_overlap", false));
        _pick_no_overlap->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                              _pick_no_overlap,
                                                              "/tools/spray/pick_no_overlap"));
    }

    /* Overlap */
    {
        _no_overlap = add_toggle_button(_("Prevent overlapping objects"),
                                        _("Prevent overlapping objects"));
        _no_overlap->set_icon_name(INKSCAPE_ICON("distribute-randomize"));
        _no_overlap->set_active(prefs->getBool("/tools/spray/no_overlap", false));
        _no_overlap->signal_toggled().connect(sigc::mem_fun(*this, &SprayToolbar::toggle_no_overlap));
    }

    /* Offset */
    {
        std::vector<Glib::ustring> labels = {_("(minimum offset)"), "", "", "", _("(default)"),  "",  "", _("(maximum offset)")};
        std::vector<double>        values = {                    0, 25, 50, 75,            100, 150, 200,                  1000};
        auto offset_val = prefs->getDouble("/tools/spray/offset", 100);
        _offset_adj = Gtk::Adjustment::create(offset_val, 0, 1000, 1, 4);
        _offset = Gtk::manage(new UI::Widget::SpinButtonToolItem("spray-offset", _("Offset %:"), _offset_adj, 0, 0));
        _offset->set_tooltip_text(_("Increase to segregate objects more (value in percent)"));
        _offset->set_custom_numeric_menu_data(values, labels);
        _offset->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _offset_adj->signal_value_changed().connect(sigc::mem_fun(*this, &SprayToolbar::offset_value_changed));
        add(*_offset);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Picker */
    {
        _picker = add_toggle_button(_("Pick color from the drawing. You can use clonetiler trace dialog for advanced effects. In clone mode original fill or stroke colors must be unset."),
                                    _("Pick color from the drawing. You can use clonetiler trace dialog for advanced effects. In clone mode original fill or stroke colors must be unset."));
        _picker->set_icon_name(INKSCAPE_ICON("color-picker"));
        _picker->set_active(prefs->getBool("/tools/spray/picker", false));
        _picker->signal_toggled().connect(sigc::mem_fun(*this, &SprayToolbar::toggle_picker));
    }
    
    /* Pick Fill */
    {
        _pick_fill = add_toggle_button(_("Apply picked color to fill"),
                                       _("Apply picked color to fill"));
        _pick_fill->set_icon_name(INKSCAPE_ICON("paint-solid"));
        _pick_fill->set_active(prefs->getBool("/tools/spray/pick_fill", false));
        _pick_fill->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                        _pick_fill,
                                                        "/tools/spray/pick_fill"));
    }

    /* Pick Stroke */
    {
        _pick_stroke = add_toggle_button(_("Apply picked color to stroke"),
                                         _("Apply picked color to stroke"));
        _pick_stroke->set_icon_name(INKSCAPE_ICON("no-marker"));
        _pick_stroke->set_active(prefs->getBool("/tools/spray/pick_stroke", false));
        _pick_stroke->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                          _pick_stroke,
                                                          "/tools/spray/pick_stroke"));
    }

    /* Inverse Value Size */
    {
        _pick_inverse_value = add_toggle_button(_("Inverted pick value, retaining color in advanced trace mode"),
                                                _("Inverted pick value, retaining color in advanced trace mode"));
        _pick_inverse_value->set_icon_name(INKSCAPE_ICON("object-tweak-shrink"));
        _pick_inverse_value->set_active(prefs->getBool("/tools/spray/pick_inverse_value", false));
        _pick_inverse_value->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                                 _pick_inverse_value,
                                                                 "/tools/spray/pick_inverse_value"));
    }

    /* Pick from center */
    {
        _pick_center = add_toggle_button(_("Pick from center instead of average area."),
                                         _("Pick from center instead of average area."));
        _pick_center->set_icon_name(INKSCAPE_ICON("snap-bounding-box-center"));
        _pick_center->set_active(prefs->getBool("/tools/spray/pick_center", true));
        _pick_center->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &SprayToolbar::on_pref_toggled),
                                                          _pick_center,
                                                          "/tools/spray/pick_center"));
    }
    
    gint mode = prefs->getInt("/tools/spray/mode", 1);
    _mode_buttons[mode]->set_active();
    show_all();
    init();
}

GtkWidget *
SprayToolbar::create(SPDesktop *desktop)
{
   auto toolbar = new SprayToolbar(desktop);
   return GTK_WIDGET(toolbar->gobj());
}

void
SprayToolbar::width_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/width",
            _width_adj->get_value());
}

void
SprayToolbar::mean_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/mean",
            _mean_adj->get_value());
}

void
SprayToolbar::standard_deviation_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/standard_deviation",
            _sd_adj->get_value());
}

void
SprayToolbar::mode_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/spray/mode", mode);
    init();
}

void
SprayToolbar::init(){
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int mode = prefs->getInt("/tools/spray/mode", 0);

    bool show = true;
    if(mode == 3 || mode == 2){
        show = false;
    }
    _no_overlap->set_visible(show);
    _over_no_transparent->set_visible(show);
    _over_transparent->set_visible(show);
    _pick_no_overlap->set_visible(show);
    _pick_stroke->set_visible(show);
    _pick_fill->set_visible(show);
    _pick_inverse_value->set_visible(show);
    _pick_center->set_visible(show);
    _picker->set_visible(show);
    _offset->set_visible(show);
    _pick_fill->set_visible(show);
    _pick_stroke->set_visible(show);
    _pick_inverse_value->set_visible(show);
    _pick_center->set_visible(show);
    if(mode == 2){
        show = true;
    }
    _spray_rotation->set_visible(show);
    update_widgets();
}

void
SprayToolbar::population_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/population",
            _population_adj->get_value());
}

void
SprayToolbar::rotation_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/rotation_variation",
            _rotation_adj->get_value());
}

void
SprayToolbar::update_widgets()
{
    _offset_adj->set_value(100.0);

    bool no_overlap_is_active = _no_overlap->get_active() && _no_overlap->get_visible();
    _offset->set_visible(no_overlap_is_active);
    if (_usepressurescale->get_active()) {
        _scale_adj->set_value(0.0);
        _spray_scale->set_sensitive(false);
    } else {
        _spray_scale->set_sensitive(true);
    }

    bool picker_is_active = _picker->get_active() && _picker->get_visible();
    _pick_fill->set_visible(picker_is_active);
    _pick_stroke->set_visible(picker_is_active);
    _pick_inverse_value->set_visible(picker_is_active);
    _pick_center->set_visible(picker_is_active);
}

void
SprayToolbar::toggle_no_overlap()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _no_overlap->get_active();
    prefs->setBool("/tools/spray/no_overlap", active);
    update_widgets();
}

void
SprayToolbar::scale_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/scale_variation",
            _scale_adj->get_value());
}

void
SprayToolbar::offset_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/spray/offset",
            _offset_adj->get_value());
}

void
SprayToolbar::toggle_pressure_scale()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _usepressurescale->get_active();
    prefs->setBool("/tools/spray/usepressurescale", active);
    if(active){
        prefs->setDouble("/tools/spray/scale_variation", 0);
    }
    update_widgets();
}

void
SprayToolbar::toggle_picker()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _picker->get_active();
    prefs->setBool("/tools/spray/picker", active);
    if(active){
        prefs->setBool("/dialogs/clonetiler/dotrace", false);
        SPDesktop *dt = SP_ACTIVE_DESKTOP;
        if (Inkscape::UI::Dialog::CloneTiler *ct = get_clone_tiler_panel(dt)){
            dt->_dlg_mgr->showDialog("CloneTiler");
            ct->show_page_trace();
        }
    }
    update_widgets();
}

void
SprayToolbar::on_pref_toggled(Gtk::ToggleToolButton *btn,
                              const Glib::ustring&   path)
{
    auto prefs = Inkscape::Preferences::get();
    bool active = btn->get_active();
    prefs->setBool(path, active);
}

void
SprayToolbar::set_mode(int mode)
{
    _mode_buttons[mode]->set_active();
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
