// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Erasor aux toolbar
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

#include <glibmm/i18n.h>

#include "eraser-toolbar.h"

#include <array>

#include "desktop.h"
#include "document-undo.h"
#include "widgets/ege-adjustment-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/icon-names.h"
#include "ui/tools/eraser-tool.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;

//########################
//##       Eraser       ##
//########################


namespace Inkscape {
namespace UI {
namespace Toolbar {
EraserToolbar::~EraserToolbar()
{
    if(_pressure_pusher) delete _pressure_pusher;
}

GtkWidget *
EraserToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new EraserToolbar(desktop);
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gint eraser_mode = ERASER_MODE_DELETE;

    {
        InkSelectOneActionColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("Delete");
        row[columns.col_tooltip  ] = _("Delete objects touched by eraser");
        row[columns.col_icon     ] = INKSCAPE_ICON("draw-eraser-delete-objects");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Cut");
        row[columns.col_tooltip  ] = _("Cut out from paths and shapes");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-difference");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Clip");
        row[columns.col_tooltip  ] = _("Clip from objects");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-intersection");
        row[columns.col_sensitive] = true;

        toolbar->_eraser_mode_action =
            InkSelectOneAction::create( "EraserModeAction",  // Name
                                        _("Mode"),           // Label
                                        "",                  // Tooltip
                                        "Not Used",          // Icon
                                        store );             // Tree store
        toolbar->_eraser_mode_action->use_radio( true );
        toolbar->_eraser_mode_action->use_group_label( true );
        eraser_mode = prefs->getInt("/tools/eraser/mode", ERASER_MODE_CLIP); // Used at end
        toolbar->_eraser_mode_action->set_active( eraser_mode );

        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_eraser_mode_action->gobj() ));

        toolbar->_eraser_mode_action->signal_changed().connect(sigc::mem_fun(*toolbar, &EraserToolbar::mode_changed));
    }


    /* Width */
    {
        gchar const* labels[] = {_("(no width)"),_("(hairline)"), nullptr, nullptr, nullptr, _("(default)"), nullptr, nullptr, nullptr, nullptr, _("(broad stroke)")};
        gdouble values[] = {0, 1, 3, 5, 10, 15, 20, 30, 50, 75, 100};
        toolbar->_width = create_adjustment_action( "EraserWidthAction",
                                                    _("Pen Width"), _("Width:"),
                                                    _("The width of the eraser pen (relative to the visible canvas area)"),
                                                    "/tools/eraser/width", 15,
                                                    GTK_WIDGET(desktop->canvas),
                                                    nullptr, // dataKludge
                                                    TRUE, "altx-eraser",
                                                    0, 100, 1.0, 10.0,
                                                    labels, values, G_N_ELEMENTS(labels),
                                                    nullptr, // callback
                                                    nullptr /*unit tracker*/, 1, 0);
        toolbar->_width_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_width));
        toolbar->_width_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &EraserToolbar::width_value_changed));
        ege_adjustment_action_set_appearance( toolbar->_width, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_width) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_width), TRUE );
    }


    /* Use Pressure button */
    {
        toolbar->_usepressure = ink_toggle_action_new( "EraserPressureAction",
                                                       _("Eraser Pressure"),
                                                       _("Use the pressure of the input device to alter the width of the pen"),
                                                       INKSCAPE_ICON("draw-use-pressure"),
                                                       GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( toolbar->_usepressure ) );
        toolbar->_pressure_pusher = new PrefPusher(GTK_TOGGLE_ACTION(toolbar->_usepressure), "/tools/eraser/usepressure", update_presets_list, (gpointer)toolbar);
    }


    /* Thinning */
    {
        gchar const* labels[] = {_("(speed blows up stroke)"), nullptr, nullptr, _("(slight widening)"), _("(constant width)"), _("(slight thinning, default)"), nullptr, nullptr, _("(speed deflates stroke)")};
        gdouble values[] = {-100, -40, -20, -10, 0, 10, 20, 40, 100};
        toolbar->_thinning = create_adjustment_action( "EraserThinningAction",
                                                       _("Eraser Stroke Thinning"), _("Thinning:"),
                                                       _("How much velocity thins the stroke (> 0 makes fast strokes thinner, < 0 makes them broader, 0 makes width independent of velocity)"),
                                                       "/tools/eraser/thinning", 10,
                                                       GTK_WIDGET(desktop->canvas),
                                                       nullptr, // dataKludge
                                                       FALSE, nullptr,
                                                       -100, 100, 1, 10.0,
                                                       labels, values, G_N_ELEMENTS(labels),
                                                       nullptr, // callback
                                                       nullptr /*unit tracker*/, 1, 0);
        toolbar->_thinning_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_thinning));
        toolbar->_thinning_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &EraserToolbar::velthin_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_thinning) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_thinning), TRUE );
    }


    /* Cap Rounding */
    {
            gchar const* labels[] = {_("(blunt caps, default)"), _("(slightly bulging)"), nullptr, nullptr, _("(approximately round)"), _("(long protruding caps)")};
        gdouble values[] = {0, 0.3, 0.5, 1.0, 1.4, 5.0};
        // TRANSLATORS: "cap" means "end" (both start and finish) here
        toolbar->_cap_rounding = create_adjustment_action( "EraserCapRoundingAction",
                                                           _("Eraser Cap rounding"), _("Caps:"),
                                                           _("Increase to make caps at the ends of strokes protrude more (0 = no caps, 1 = round caps)"),
                                                           "/tools/eraser/cap_rounding", 0.0,
                                                           GTK_WIDGET(desktop->canvas),
                                                           nullptr, // dataKludge
                                                           FALSE, nullptr,
                                                           0.0, 5.0, 0.01, 0.1,
                                                           labels, values, G_N_ELEMENTS(labels),
                                                           nullptr, // callback
                                                           nullptr /*unit tracker*/, 0.01, 2 );
        toolbar->_cap_rounding_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_cap_rounding));
        toolbar->_cap_rounding_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &EraserToolbar::cap_rounding_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_cap_rounding) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_cap_rounding), TRUE );
    }


    /* Tremor */
    {
        gchar const* labels[] = {_("(smooth line)"), _("(slight tremor)"), _("(noticeable tremor)"), nullptr, nullptr, _("(maximum tremor)")};
        gdouble values[] = {0, 10, 20, 40, 60, 100};
        toolbar->_tremor = create_adjustment_action( "EraserTremorAction",
                                                     _("EraserStroke Tremor"), _("Tremor:"),
                                                     _("Increase to make strokes rugged and trembling"),
                                                     "/tools/eraser/tremor", 0.0,
                                                     GTK_WIDGET(desktop->canvas),
                                                     nullptr, // dataKludge
                                                     FALSE, nullptr,
                                                     0.0, 100, 1, 10.0,
                                                     labels, values, G_N_ELEMENTS(labels),
                                                     nullptr, // callback
                                                     nullptr /*unit tracker*/, 1, 0);
        toolbar->_tremor_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_tremor));
        toolbar->_tremor_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &EraserToolbar::tremor_value_changed));

        ege_adjustment_action_set_appearance( toolbar->_tremor, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_tremor) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_tremor), TRUE );
    }

    /* Mass */
    {
        gchar const* labels[] = {_("(no inertia)"), _("(slight smoothing, default)"), _("(noticeable lagging)"), nullptr, nullptr, _("(maximum inertia)")};
        gdouble values[] = {0.0, 2, 10, 20, 50, 100};
        toolbar->_mass = create_adjustment_action( "EraserMassAction",
                                                   _("Eraser Mass"), _("Mass:"),
                                                   _("Increase to make the eraser drag behind, as if slowed by inertia"),
                                                   "/tools/eraser/mass", 10.0,
                                                   GTK_WIDGET(desktop->canvas),
                                                   nullptr, // dataKludge
                                                   FALSE, nullptr,
                                                   0.0, 100, 1, 10.0,
                                                   labels, values, G_N_ELEMENTS(labels),
                                                   nullptr, // callback
                                                   nullptr /*unit tracker*/, 1, 0);
        toolbar->_mass_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_mass));
        toolbar->_mass_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &EraserToolbar::mass_value_changed));
        ege_adjustment_action_set_appearance( toolbar->_mass, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_mass) );
        gtk_action_set_sensitive( GTK_ACTION(toolbar->_mass), TRUE );
    }

    /* Overlap */
    {
        toolbar->_split = ink_toggle_action_new( "EraserBreakAppart",
                                                 _("Break apart cut items"),
                                                 _("Break apart cut items"),
                                                 INKSCAPE_ICON("distribute-randomize"),
                                                 secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(toolbar->_split), prefs->getBool("/tools/eraser/break_apart", false) );
        g_signal_connect_after( G_OBJECT(toolbar->_split), "toggled", G_CALLBACK(toggle_break_apart), (gpointer)toolbar);
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_split) );
    }

    toolbar->set_eraser_mode_visibility(eraser_mode);

    return GTK_WIDGET(toolbar->gobj());
}

void
EraserToolbar::mode_changed(int mode)
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt( "/tools/eraser/mode", mode );
    }

    set_eraser_mode_visibility(mode);

    // only take action if run by the attr_changed listener
    if (!_freeze) {
        // in turn, prevent listener from responding
        _freeze = true;

        /*
        if ( eraser_mode != ERASER_MODE_DELETE ) {
        } else {
        }
        */
        // TODO finish implementation

        _freeze =  false;
    }
}

void
EraserToolbar::set_eraser_mode_visibility(const guint eraser_mode)
{
    gtk_action_set_visible( GTK_ACTION(_split), (eraser_mode == ERASER_MODE_CUT));

    const gboolean visibility = (eraser_mode != ERASER_MODE_DELETE);

    const std::array<GtkAction *, 6> arr = {GTK_ACTION(_cap_rounding),
                                            GTK_ACTION(_mass),
                                            GTK_ACTION(_thinning),
                                            GTK_ACTION(_tremor),
                                            GTK_ACTION(_usepressure),
                                            GTK_ACTION(_width)};
    for (auto act : arr) {
        gtk_action_set_visible( act, visibility );
    }
}

void
EraserToolbar::width_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/width", _width_adj->get_value() );
}

void
EraserToolbar::mass_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/mass", _mass_adj->get_value() );
}

void
EraserToolbar::velthin_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/eraser/thinning", _thinning_adj->get_value() );
}

void
EraserToolbar::cap_rounding_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/cap_rounding", _cap_rounding_adj->get_value() );
}

void
EraserToolbar::tremor_value_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/eraser/tremor", _tremor_adj->get_value() );
}

// A dummy function for PrefPusher.
// The code was calling the update_presets_list function in the calligraphy tool
// which was immediately returning. TODO: Investigate this further.
void
EraserToolbar::update_presets_list(gpointer data)
{
    return;
}

void
EraserToolbar::toggle_break_apart(GtkToggleAction *act,
                                  gpointer         data)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setBool("/tools/eraser/break_apart", active);
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
