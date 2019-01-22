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


#include "desktop.h"
#include "inkscape.h"
#include "widgets/ege-adjustment-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"
#include "ui/dialog/clonetiler.h"
#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/panel-dialog.h"
#include "ui/pref-pusher.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/icon-names.h"

#include <glibmm/i18n.h>

using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;

// Disabled in 0.91 because of Bug #1274831 (crash, spraying an object 
// with the mode: spray object in single path)
// Please enable again when working on 1.0
#define ENABLE_SPRAY_MODE_SINGLE_PATH

//########################
//##       Spray        ##
//########################

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
{}

GtkWidget *
SprayToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto holder = new SprayToolbar(desktop);

    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    {
        /* Width */
        gchar const* labels[] = {_("(narrow spray)"), nullptr, nullptr, nullptr, _("(default)"), nullptr, nullptr, nullptr, nullptr, _("(broad spray)")};
        gdouble values[] = {1, 3, 5, 10, 15, 20, 30, 50, 75, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "SprayWidthAction",
                                                              _("Width"), _("Width:"), _("The width of the spray area (relative to the visible canvas area)"),
                                                              "/tools/spray/width", 15,
                                                              true, "altx-spray",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_width_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_width_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::width_value_changed));
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), true );
    }
    
    /* Use Pressure Width button */
    {
        InkToggleAction* act = ink_toggle_action_new( "SprayPressureWidthAction",
                                                      _("Pressure"),
                                                      _("Use the pressure of the input device to alter the width of spray area"),
                                                      INKSCAPE_ICON("draw-use-pressure"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
        holder->_usepressurewidth_pusher.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/spray/usepressurewidth"));
    }
    
    {
        /* Mean */
        gchar const* labels[] = {_("(default)"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, _("(maximum mean)")};
        gdouble values[] = {0, 5, 10, 20, 30, 50, 70, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "SprayMeanAction",
                                                              _("Focus"), _("Focus:"), _("0 to spray a spot; increase to enlarge the ring radius"),
                                                              "/tools/spray/mean", 0,
                                                              true, "spray-mean",
                                                              0, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_mean_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_mean_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::mean_value_changed));
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), true );
    }

    {
        /* Standard_deviation */
        gchar const* labels[] = {_("(minimum scatter)"), nullptr, nullptr, nullptr, nullptr, nullptr, _("(default)"), _("(maximum scatter)")};
        gdouble values[] = {1, 5, 10, 20, 30, 50, 70, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "SprayStandard_deviationAction",
                                                              C_("Spray tool", "Scatter"), C_("Spray tool", "Scatter:"), _("Increase to scatter sprayed objects"),
                                                              "/tools/spray/standard_deviation", 70,
                                                              true, "spray-standard_deviation",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));
        holder->_sd_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        holder->_sd_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::standard_deviation_value_changed));
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        gtk_action_set_sensitive( GTK_ACTION(eact), true );
    }

    /* Mode */
    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Spray with copies");
        row[columns.col_tooltip  ] = _("Spray copies of the initial selection");
        row[columns.col_icon     ] = INKSCAPE_ICON("spray-mode-copy");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] =   _("Spray with clones");
        row[columns.col_tooltip  ] =   _("Spray clones of the initial selection");
        row[columns.col_icon     ] =   INKSCAPE_ICON("spray-mode-clone");
        row[columns.col_sensitive] =  true;

#ifdef ENABLE_SPRAY_MODE_SINGLE_PATH
        row = *(store->append());
        row[columns.col_label    ] = _("Spray single path");
        row[columns.col_tooltip  ] = _("Spray objects in a single path");
        row[columns.col_icon     ] = INKSCAPE_ICON("spray-mode-union");
        row[columns.col_sensitive] = true;
#endif

        row = *(store->append());
        row[columns.col_label    ] = _("Delete sprayed items");
        row[columns.col_tooltip  ] = _("Delete sprayed items from selection");
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-eraser");
        row[columns.col_sensitive] = true;

        holder->_spray_tool_mode =
            InkSelectOneAction::create( "SprayModeAction",   // Name
                                        _("Mode"),           // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store

        holder->_spray_tool_mode->use_radio( true );
        holder->_spray_tool_mode->use_group_label( true );
        gint mode = prefs->getInt("/tools/spray/mode", 1);
        holder->_spray_tool_mode->set_active( mode );
        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_spray_tool_mode->gobj() ));
        holder->_spray_tool_mode->signal_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::mode_changed));
    }

    {   /* Population */
        gchar const* labels[] = {_("(low population)"), nullptr, nullptr, nullptr, _("(default)"), nullptr, _("(high population)")};
        gdouble values[] = {5, 20, 35, 50, 70, 85, 100};
        holder->_spray_population = create_adjustment_action( "SprayPopulationAction",
                                                              _("Amount"), _("Amount:"),
                                                              _("Adjusts the number of items sprayed per click"),
                                                              "/tools/spray/population", 70,
                                                              true, "spray-population",
                                                              1, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(holder->_spray_population, GTK_WIDGET(desktop->canvas));
        holder->_population_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_spray_population));
        holder->_population_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::population_value_changed));
        ege_adjustment_action_set_appearance( holder->_spray_population, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_spray_population) );
        gtk_action_set_sensitive( GTK_ACTION(holder->_spray_population), true );
    }

    /* Use Pressure Population button */
    {
        InkToggleAction* act = ink_toggle_action_new( "SprayPressurePopulationAction",
                                                      _("Pressure"),
                                                      _("Use the pressure of the input device to alter the amount of sprayed objects"),
                                                      INKSCAPE_ICON("draw-use-pressure"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
        holder->_usepressurepopulation_pusher.reset(new PrefPusher(GTK_TOGGLE_ACTION(act), "/tools/spray/usepressurepopulation"));
    }

    {   /* Rotation */
        gchar const* labels[] = {_("(default)"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, _("(high rotation variation)")};
        gdouble values[] = {0, 10, 25, 35, 50, 60, 80, 100};
        holder->_spray_rotation = create_adjustment_action( "SprayRotationAction",
                                                            _("Rotation"), _("Rotation:"),
                                                            // xgettext:no-c-format
                                                            _("Variation of the rotation of the sprayed objects; 0% for the same rotation than the original object"),
                                                            "/tools/spray/rotation_variation", 0,
                                                            true, "spray-rotation",
                                                            0, 100, 1.0, 10.0,
                                                            labels, values, G_N_ELEMENTS(labels),
                                                            nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(holder->_spray_rotation, GTK_WIDGET(desktop->canvas));
        holder->_rotation_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_spray_rotation));
        holder->_rotation_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::rotation_value_changed));
        ege_adjustment_action_set_appearance(holder->_spray_rotation, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_spray_rotation) );
        gtk_action_set_sensitive( GTK_ACTION(holder->_spray_rotation), true );
    }

    {   /* Scale */
        gchar const* labels[] = {_("(default)"), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, _("(high scale variation)")};
        gdouble values[] = {0, 10, 25, 35, 50, 60, 80, 100};
        holder->_spray_scale = create_adjustment_action( "SprayScaleAction",
                                                              C_("Spray tool", "Scale"), C_("Spray tool", "Scale:"),
                                                              // xgettext:no-c-format
                                                              _("Variation in the scale of the sprayed objects; 0% for the same scale than the original object"),
                                                              "/tools/spray/scale_variation", 0,
                                                              true, "spray-scale",
                                                              0, 100, 1.0, 10.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr /*unit tracker*/, 1, 0 );
        ege_adjustment_action_set_focuswidget(holder->_spray_scale, GTK_WIDGET(desktop->canvas));
        holder->_scale_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_spray_scale));
        holder->_scale_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::scale_value_changed));
        ege_adjustment_action_set_appearance( holder->_spray_scale, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_spray_scale) );
        gtk_action_set_sensitive( GTK_ACTION(holder->_spray_scale), true );
    }

    /* Use Pressure Scale button */
    {
        holder->_usepressurescale = ink_toggle_action_new( "SprayPressureScaleAction",
                                                           _("Pressure"),
                                                           _("Use the pressure of the input device to alter the scale of new items"),
                                                           INKSCAPE_ICON("draw-use-pressure"),
                                                           GTK_ICON_SIZE_MENU);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(holder->_usepressurescale), prefs->getBool("/tools/spray/usepressurescale", false));
        g_signal_connect(G_OBJECT(holder->_usepressurescale), "toggled", G_CALLBACK(&SprayToolbar::toggle_pressure_scale), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_usepressurescale) );
    }

    /* Picker */
    {
        holder->_picker = ink_toggle_action_new( "SprayPickColorAction",
                                                      _("Pick color from the drawing. You can use clonetiler trace dialog for advanced effects. In clone mode original fill or stroke colors must be unset."),
                                                      _("Pick color from the drawing. You can use clonetiler trace dialog for advanced effects. In clone mode original fill or stroke colors must be unset."),
                                                      INKSCAPE_ICON("color-picker"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_picker), prefs->getBool("/tools/spray/picker", false) );
        g_signal_connect(G_OBJECT(holder->_picker), "toggled", G_CALLBACK(&SprayToolbar::toggle_picker), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_picker) );
    }
    
    /* Pick from center */
    {
        holder->_pick_center = ink_toggle_action_new( "SprayPickCenterAction",
                                                      _("Pick from center instead average area."),
                                                      _("Pick from center instead average area."),
                                                      INKSCAPE_ICON("snap-bounding-box-center"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_pick_center), prefs->getBool("/tools/spray/pick_center", true) );
        g_signal_connect(G_OBJECT(holder->_pick_center), "toggled", G_CALLBACK(&SprayToolbar::toggle_pick_center), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_pick_center) );
    }

    /* Inverse Value Size */
    {
        holder->_pick_inverse_value = ink_toggle_action_new( "SprayPickInverseValueAction",
                                                      _("Inverted pick value, retaining color in advanced trace mode"),
                                                      _("Inverted pick value, retaining color in advanced trace mode"),
                                                      INKSCAPE_ICON("object-tweak-shrink"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_pick_inverse_value), prefs->getBool("/tools/spray/pick_inverse_value", false) );
        g_signal_connect(G_OBJECT(holder->_pick_inverse_value), "toggled", G_CALLBACK(&SprayToolbar::toggle_pick_inverse_value), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_pick_inverse_value) );
    }

    /* Pick Fill */
    {
        holder->_pick_fill = ink_toggle_action_new( "SprayPickFillAction",
                                                      _("Apply picked color to fill"),
                                                      _("Apply picked color to fill"),
                                                      INKSCAPE_ICON("paint-solid"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_pick_fill), prefs->getBool("/tools/spray/pick_fill", false) );
        g_signal_connect(G_OBJECT(holder->_pick_fill), "toggled", G_CALLBACK(&SprayToolbar::toggle_pick_fill), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_pick_fill) );
    }

    /* Pick Stroke */
    {
        holder->_pick_stroke = ink_toggle_action_new( "SprayPickStrokeAction",
                                                      _("Apply picked color to stroke"),
                                                      _("Apply picked color to stroke"),
                                                      INKSCAPE_ICON("no-marker"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_pick_stroke), prefs->getBool("/tools/spray/pick_stroke", false) );
        g_signal_connect(G_OBJECT(holder->_pick_stroke), "toggled", G_CALLBACK(&SprayToolbar::toggle_pick_stroke), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_pick_stroke) );
    }

    /* Pick No Overlap */
    {
        holder->_pick_no_overlap = ink_toggle_action_new( "SprayPickNoOverlapAction",
                                                      _("No overlap between colors"),
                                                      _("No overlap between colors"),
                                                      INKSCAPE_ICON("symbol-bigger"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_pick_no_overlap), prefs->getBool("/tools/spray/pick_no_overlap", false) );
        g_signal_connect(G_OBJECT(holder->_pick_no_overlap), "toggled", G_CALLBACK(&SprayToolbar::toggle_pick_no_overlap), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_pick_no_overlap) );
    }

    /* Over Transparent */
    {
        holder->_over_transparent = ink_toggle_action_new( "SprayOverTransparentAction",
                                                      _("Apply over transparent areas"),
                                                      _("Apply over transparent areas"),
                                                      INKSCAPE_ICON("object-hidden"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_over_transparent), prefs->getBool("/tools/spray/over_transparent", true) );
        g_signal_connect(G_OBJECT(holder->_over_transparent), "toggled", G_CALLBACK(&SprayToolbar::toggle_over_transparent), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_over_transparent) );
    }

    /* Over No Transparent */
    {
        holder->_over_no_transparent = ink_toggle_action_new( "SprayOverNoTransparentAction",
                                                      _("Apply over no transparent areas"),
                                                      _("Apply over no transparent areas"),
                                                      INKSCAPE_ICON("object-visible"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_over_no_transparent), prefs->getBool("/tools/spray/over_no_transparent", true) );
        g_signal_connect(G_OBJECT(holder->_over_no_transparent), "toggled", G_CALLBACK(&SprayToolbar::toggle_over_no_transparent), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_over_no_transparent) );
    }

    /* Overlap */
    {
        holder->_no_overlap = ink_toggle_action_new( "SprayNoOverlapAction",
                                                      _("Prevent overlapping objects"),
                                                      _("Prevent overlapping objects"),
                                                      INKSCAPE_ICON("distribute-randomize"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_no_overlap), prefs->getBool("/tools/spray/no_overlap", false) );
        g_signal_connect(G_OBJECT(holder->_no_overlap), "toggled", G_CALLBACK(&SprayToolbar::toggle_no_overlap), holder);
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_no_overlap) );
    }
    
    /* Offset */
    {
        gchar const* labels[] = {_("(minimum offset)"), nullptr, nullptr, nullptr, _("(default)"), nullptr, nullptr, _("(maximum offset)")};
        gdouble values[] = {0, 25, 50, 75, 100, 150, 200, 1000};
        holder->_offset = create_adjustment_action( "SprayToolOffsetAction",
                                         _("Offset %"), _("Offset %:"),
                                         _("Increase to segregate objects more (value in percent)"),
                                         "/tools/spray/offset", 100,
                                         FALSE, nullptr,
                                         0, 1000, 1, 4,
                                         labels, values, G_N_ELEMENTS(labels),
                                         nullptr, 0 , 0);
        ege_adjustment_action_set_focuswidget(holder->_offset, GTK_WIDGET(desktop->canvas));
        holder->_offset_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_offset));
        holder->_offset_adj->signal_value_changed().connect(sigc::mem_fun(*holder, &SprayToolbar::offset_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_offset) );
    }
    holder->init();

    return GTK_WIDGET(holder->gobj());
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
    gtk_action_set_visible( GTK_ACTION(_no_overlap), show );
    gtk_action_set_visible( GTK_ACTION(_over_no_transparent), show );
    gtk_action_set_visible( GTK_ACTION(_over_transparent), show );
    gtk_action_set_visible( GTK_ACTION(_pick_no_overlap), show );
    gtk_action_set_visible( GTK_ACTION(_pick_stroke), show );
    gtk_action_set_visible( GTK_ACTION(_pick_fill), show );
    gtk_action_set_visible( GTK_ACTION(_pick_inverse_value), show );
    gtk_action_set_visible( GTK_ACTION(_pick_center), show );
    gtk_action_set_visible( GTK_ACTION(_picker), show );
    gtk_action_set_visible( GTK_ACTION(_offset), show );
    gtk_action_set_visible( GTK_ACTION(_pick_fill), show );
    gtk_action_set_visible( GTK_ACTION(_pick_stroke), show );
    gtk_action_set_visible( GTK_ACTION(_pick_inverse_value), show );
    gtk_action_set_visible( GTK_ACTION(_pick_center), show );
    if(mode == 2){
        show = true;
    }
    gtk_action_set_visible( GTK_ACTION(_spray_rotation), show );
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

    if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(_no_overlap)) &&
        gtk_action_get_visible(GTK_ACTION(_no_overlap)))
    {
        gtk_action_set_visible( GTK_ACTION(_offset), true );
    } else {
        gtk_action_set_visible( GTK_ACTION(_offset), false );
    }
    if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(_usepressurescale))) {
        _scale_adj->set_value(0.0);
        gtk_action_set_sensitive( GTK_ACTION(_spray_scale), false );
    } else {
        gtk_action_set_sensitive( GTK_ACTION(_spray_scale), true );
    }
    if(gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(_picker)) &&
       gtk_action_get_visible(GTK_ACTION(_picker))){
        gtk_action_set_visible( GTK_ACTION(_pick_fill), true );
        gtk_action_set_visible( GTK_ACTION(_pick_stroke), true );
        gtk_action_set_visible( GTK_ACTION(_pick_inverse_value), true );
        gtk_action_set_visible( GTK_ACTION(_pick_center), true );
    } else {
        gtk_action_set_visible( GTK_ACTION(_pick_fill), false );
        gtk_action_set_visible( GTK_ACTION(_pick_stroke), false );
        gtk_action_set_visible( GTK_ACTION(_pick_inverse_value), false );
        gtk_action_set_visible( GTK_ACTION(_pick_center), false );
    }
}

void
SprayToolbar::toggle_no_overlap(GtkToggleAction *toggleaction,
                                gpointer         user_data)
{
    auto toolbar = reinterpret_cast<SprayToolbar *>(user_data);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/no_overlap", active);
    toolbar->update_widgets();
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
SprayToolbar::toggle_pressure_scale(GtkToggleAction *toggleaction,
                                    gpointer         user_data)
{
    auto toolbar = reinterpret_cast<SprayToolbar *>(user_data);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/usepressurescale", active);
    if(active){
        prefs->setDouble("/tools/spray/scale_variation", 0);
    }
    toolbar->update_widgets();
}

void
SprayToolbar::toggle_over_no_transparent(GtkToggleAction *toggleaction,
                                         gpointer         /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/over_no_transparent", active);
}

void
SprayToolbar::toggle_over_transparent(GtkToggleAction *toggleaction,
                                      gpointer         /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/over_transparent", active);
}

void
SprayToolbar::toggle_picker(GtkToggleAction *toggleaction,
                            gpointer         user_data)
{
    auto toolbar = reinterpret_cast<SprayToolbar *>(user_data);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/picker", active);
    if(active){
        prefs->setBool("/dialogs/clonetiler/dotrace", false);
        SPDesktop *dt = SP_ACTIVE_DESKTOP;
        if (Inkscape::UI::Dialog::CloneTiler *ct = get_clone_tiler_panel(dt)){
            dt->_dlg_mgr->showDialog("CloneTiler");
            ct->show_page_trace();
        }
    }
    toolbar->update_widgets();
}

void
SprayToolbar::toggle_pick_center(GtkToggleAction *toggleaction,
                                 gpointer         /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/pick_center", active);
}

void
SprayToolbar::toggle_pick_fill(GtkToggleAction *toggleaction,
                               gpointer         /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/pick_fill", active);
}

void
SprayToolbar::toggle_pick_stroke(GtkToggleAction *toggleaction,
                                 gpointer         /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/pick_stroke", active);
}

void
SprayToolbar::toggle_pick_no_overlap(GtkToggleAction *toggleaction,
                                     gpointer        /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/pick_no_overlap", active);
}

void
SprayToolbar::toggle_pick_inverse_value(GtkToggleAction *toggleaction,
                                        gpointer         /* user_data */)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(toggleaction);
    prefs->setBool("/tools/spray/pick_inverse_value", active);
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
