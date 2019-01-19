// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Selector aux toolbar
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2003-2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include <2geom/rect.h>

#include "select-toolbar.h"

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "widgets/ink-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"
#include "inkscape.h"
#include "message-stack.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "display/sp-canvas.h"

#include "helper/action-context.h"
#include "helper/action.h"

#include "object/sp-item-transform.h"
#include "object/sp-namedview.h"

#include "ui/icon-names.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/widget-sizes.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::DocumentUndo;
using Inkscape::Util::unit_table;



// toggle button callbacks and updaters

static void toggle_stroke( GtkToggleAction* act, gpointer data )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setBool("/options/transform/stroke", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>stroke width</b> is <b>scaled</b> when objects are scaled."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>stroke width</b> is <b>not scaled</b> when objects are scaled."));
    }
}

static void toggle_corners( GtkToggleAction* act, gpointer data)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setBool("/options/transform/rectcorners", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>rounded rectangle corners</b> are <b>scaled</b> when rectangles are scaled."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>rounded rectangle corners</b> are <b>not scaled</b> when rectangles are scaled."));
    }
}

static void toggle_gradient( GtkToggleAction *act, gpointer data )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setBool("/options/transform/gradient", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>gradients</b> are <b>transformed</b> along with their objects when those are transformed (moved, scaled, rotated, or skewed)."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>gradients</b> remain <b>fixed</b> when objects are transformed (moved, scaled, rotated, or skewed)."));
    }
}

static void toggle_pattern( GtkToggleAction* act, gpointer data )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setInt("/options/transform/pattern", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>patterns</b> are <b>transformed</b> along with their objects when those are transformed (moved, scaled, rotated, or skewed)."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Now <b>patterns</b> remain <b>fixed</b> when objects are transformed (moved, scaled, rotated, or skewed)."));
    }
}

static void toggle_lock( GtkToggleAction *act, gpointer /*data*/ ) {
    gboolean active = gtk_toggle_action_get_active( act );
    if ( active ) {
        g_object_set( G_OBJECT(act), "iconId", INKSCAPE_ICON("object-locked"), NULL );
    } else {
        g_object_set( G_OBJECT(act), "iconId", INKSCAPE_ICON("object-unlocked"), NULL );
    }
}

static void trigger_sp_action( GtkAction* /*act*/, gpointer user_data )
{
    SPAction* targetAction = SP_ACTION(user_data);
    if ( targetAction ) {
        sp_action_perform( targetAction, nullptr );
    }
}

static GtkAction* create_action_for_verb( Inkscape::Verb* verb, Inkscape::UI::View::View* view, GtkIconSize size )
{
    GtkAction* act = nullptr;

    SPAction* targetAction = verb->get_action(Inkscape::ActionContext(view));
    InkAction* inky = ink_action_new( verb->get_id(), verb->get_name(), verb->get_tip(), verb->get_image(), size  );
    act = GTK_ACTION(inky);

    g_signal_connect( G_OBJECT(inky), "activate", G_CALLBACK(trigger_sp_action), targetAction );

    return act;
}

namespace Inkscape {
namespace UI {
namespace Toolbar {

SelectToolbar::SelectToolbar(SPDesktop *desktop) :
    Toolbar(desktop),
    _context_actions(new std::vector<GtkAction*>()),
    _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR)),
    _update(false)
{}

SelectToolbar::~SelectToolbar()
{
    delete _tracker;
}

GtkWidget *
SelectToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto holder = new SelectToolbar(desktop);

    Inkscape::UI::View::View *view = desktop;
    GtkIconSize secondarySize = Inkscape::UI::ToolboxFactory::prefToSize("/toolbox/secondary", 1);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    GtkAction* act = nullptr;

    holder->_selection_actions = mainActions; // temporary

    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_EDIT_SELECT_ALL), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_EDIT_SELECT_ALL_IN_ALL_LAYERS), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_EDIT_DESELECT), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );

    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_OBJECT_ROTATE_90_CCW), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_OBJECT_ROTATE_90_CW), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_OBJECT_FLIP_HORIZONTAL), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_OBJECT_FLIP_VERTICAL), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );

    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_SELECTION_TO_BACK), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_SELECTION_LOWER), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_SELECTION_RAISE), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );
    act = create_action_for_verb( Inkscape::Verb::get(SP_VERB_SELECTION_TO_FRONT), view, secondarySize );
    gtk_action_group_add_action( holder->_selection_actions, act );
    holder->_context_actions->push_back( act );

    // Create the parent widget for x y w h tracker.
    // The vb frame holds all other widgets and is used to set sensitivity depending on selection state.
    auto vb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vb), FALSE);
    gtk_widget_show(vb);

    // Create the units menu.
    holder->_tracker->addUnit(unit_table.getUnit("%"));
    holder->_tracker->setActiveUnit( desktop->getNamedView()->display_units );

    EgeAdjustmentAction* eact = nullptr;

    // four spinbuttons

    eact = create_adjustment_action(
            "XAction",                            /* name */ 
            C_("Select toolbar", "X position"),   /* label */ 
            C_("Select toolbar", "X:"),           /* shortLabel */ 
            C_("Select toolbar", "Horizontal coordinate of selection"), /* tooltip */ 
            "/tools/select/X",                    /* path */ 
            0.0,                                  /* def(default) */ 
            GTK_WIDGET(desktop->canvas),          /* focusTarget */ 
            nullptr,                              /* dataKludge */
            TRUE, "altx",                         /* altx, altx_mark */ 
            -1e6, 1e6, SPIN_STEP, SPIN_PAGE_STEP, /* lower, upper, step, page */ 
            nullptr, nullptr, 0,                  /* descrLabels, descrValues, descrCount */
            nullptr,                              /* callback */
            holder->_tracker,                     /* unit_tracker */
            SPIN_STEP, 3, 1);                     /* climb, digits, factor */

    holder->_adj_x = Glib::wrap(GTK_ADJUSTMENT(ege_adjustment_action_get_adjustment(eact)));
    holder->_adj_x->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &SelectToolbar::any_value_changed), holder->_adj_x));
    gtk_action_group_add_action( holder->_selection_actions, GTK_ACTION(eact) );
    holder->_context_actions->push_back( GTK_ACTION(eact) );

    eact = create_adjustment_action(
            "YAction",                            /* name */
            C_("Select toolbar", "Y position"),   /* label */
            C_("Select toolbar", "Y:"),           /* shortLabel */
            C_("Select toolbar", "Vertical coordinate of selection"), /* tooltip */
            "/tools/select/Y",                    /* path */
            0.0,                                  /* def(default) */
            GTK_WIDGET(desktop->canvas),          /* focusTarget */
            nullptr,                              /* dataKludge */
            TRUE, "altx",                         /* altx, altx_mark */
            -1e6, 1e6, SPIN_STEP, SPIN_PAGE_STEP, /* lower, upper, step, page */
            nullptr, nullptr, 0,                              /* descrLabels, descrValues, descrCount */
            nullptr,                              /* callback */
            holder->_tracker,                     /* unit_tracker */
            SPIN_STEP, 3, 1);                     /* climb, digits, factor */              

    holder->_adj_y = Glib::wrap(GTK_ADJUSTMENT(ege_adjustment_action_get_adjustment(eact)));
    holder->_adj_y->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &SelectToolbar::any_value_changed), holder->_adj_y));
    gtk_action_group_add_action( holder->_selection_actions, GTK_ACTION(eact) );
    holder->_context_actions->push_back( GTK_ACTION(eact) );

    eact = create_adjustment_action(
            "WidthAction",                        /* name */
            C_("Select toolbar", "Width"),        /* label */
            C_("Select toolbar", "W:"),           /* shortLabel */
            C_("Select toolbar", "Width of selection"), /* tooltip */
            "/tools/select/width",                /* path */                      
            0.0,                                  /* def(default) */
            GTK_WIDGET(desktop->canvas),          /* focusTarget */
            nullptr,                              /* dataKludge */
            TRUE, "altx",                         /* altx, altx_mark */
            0.0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,  /* lower, upper, step, page */
            nullptr, nullptr, 0,                              /* descrLabels, descrValues, descrCount */
            nullptr,                              /* callback */
            holder->_tracker,                     /* unit_tracker */
            SPIN_STEP, 3, 1);                     /* climb, digits, factor */

    holder->_adj_w = Glib::wrap(GTK_ADJUSTMENT(ege_adjustment_action_get_adjustment(eact)));
    holder->_adj_w->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &SelectToolbar::any_value_changed), holder->_adj_w));
    gtk_action_group_add_action( holder->_selection_actions, GTK_ACTION(eact) );
    holder->_context_actions->push_back( GTK_ACTION(eact) );

    // lock toggle
    {
    holder->_lock = GTK_TOGGLE_ACTION(ink_toggle_action_new( "LockAction",
                                                            _("Lock width and height"),
                                                            _("When locked, change both width and height by the same proportion"),
                                                            INKSCAPE_ICON("object-unlocked"),
                                                            GTK_ICON_SIZE_MENU ));
    gtk_action_set_short_label( GTK_ACTION(holder->_lock), "Lock" );
    g_signal_connect_after( G_OBJECT(holder->_lock), "toggled", G_CALLBACK(toggle_lock), desktop) ;
    gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_lock) );
    }

    eact = create_adjustment_action(
            "HeightAction",                       /* name */
            C_("Select toolbar", "Height"),       /* label */
            C_("Select toolbar", "H:"),           /* shortLabel */
            C_("Select toolbar", "Height of selection"), /* tooltip */
            "/tools/select/height",               /* path */                      
            0.0,                                  /* def(default) */
            GTK_WIDGET(desktop->canvas),          /* focusTarget */
            nullptr,                              /* dataKludge */
            TRUE, "altx",                         /* altx, altx_mark */
            0.0, 1e6, SPIN_STEP, SPIN_PAGE_STEP,  /* lower, upper, step, page */
            nullptr, nullptr, 0,                              /* descrLabels, descrValues, descrCount */
            nullptr,                              /* callback */
            holder->_tracker,                     /* unit_tracker */
            SPIN_STEP, 3, 1);                     /* climb, digits, factor */


    holder->_adj_h = Glib::wrap(GTK_ADJUSTMENT(ege_adjustment_action_get_adjustment(eact)));
    holder->_adj_h->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &SelectToolbar::any_value_changed), holder->_adj_h));
    gtk_action_group_add_action( holder->_selection_actions, GTK_ACTION(eact) );
    holder->_context_actions->push_back( GTK_ACTION(eact) );

    // Add the units menu.
    {
        InkSelectOneAction* act = holder->_tracker->createAction( "UnitsAction", _("Units"), ("") );
        gtk_action_group_add_action( holder->_selection_actions, act->gobj() );
    }

    // Force update when selection changes.
    INKSCAPE.signal_selection_modified.connect(sigc::mem_fun(*holder, &SelectToolbar::on_inkscape_selection_modified));
    INKSCAPE.signal_selection_changed.connect(sigc::mem_fun(*holder, &SelectToolbar::on_inkscape_selection_changed));

    // Update now.
    holder->layout_widget_update(SP_ACTIVE_DESKTOP ? SP_ACTIVE_DESKTOP->getSelection() : nullptr);

    for (auto & contextAction : *holder->_context_actions) {
        if ( gtk_action_is_sensitive(contextAction) ) {
            gtk_action_set_sensitive( contextAction, FALSE );
        }
    }

    // Insert vb into the toolbar.
    GtkToolItem *vb_toolitem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(vb_toolitem), vb);
    holder->insert(*Glib::wrap(vb_toolitem), -1);

    // "Transform with object" buttons
    {

    InkToggleAction* itact = ink_toggle_action_new( "transform_stroke",
                                                    _("Scale stroke width"),
                                                    _("When scaling objects, scale the stroke width by the same proportion"),
                                                    "transform-affect-stroke",
                                                    secondarySize );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(itact), prefs->getBool("/options/transform/stroke", true) );
    g_signal_connect_after( G_OBJECT(itact), "toggled", G_CALLBACK(toggle_stroke), desktop) ;
    gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
    }

    {
    InkToggleAction* itact = ink_toggle_action_new( "transform_corners",
                                                    _("Scale rounded corners"),
                                                    _("When scaling rectangles, scale the radii of rounded corners"),
                                                    "transform-affect-rounded-corners",
                                                    secondarySize );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(itact), prefs->getBool("/options/transform/rectcorners", true) );
    g_signal_connect_after( G_OBJECT(itact), "toggled", G_CALLBACK(toggle_corners), desktop) ;
    gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
    }

    {
    InkToggleAction* itact = ink_toggle_action_new( "transform_gradient",
                                                    _("Move gradients"),
                                                    _("Move gradients (in fill or stroke) along with the objects"),
                                                    "transform-affect-gradient",
                                                    secondarySize );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(itact), prefs->getBool("/options/transform/gradient", true) );
    g_signal_connect_after( G_OBJECT(itact), "toggled", G_CALLBACK(toggle_gradient), desktop) ;
    gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
    }

    {
    InkToggleAction* itact = ink_toggle_action_new( "transform_pattern",
                                                    _("Move patterns"),
                                                    _("Move patterns (in fill or stroke) along with the objects"),
                                                    "transform-affect-pattern",
                                                    secondarySize );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(itact), prefs->getBool("/options/transform/pattern", true) );
    g_signal_connect_after( G_OBJECT(itact), "toggled", G_CALLBACK(toggle_pattern), desktop) ;
    gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
    }

    return GTK_WIDGET(holder->gobj());
}

void
SelectToolbar::any_value_changed(Glib::RefPtr<Gtk::Adjustment>& adj)
{
    if (_update) {
        return;
    }

    if ( !_tracker || _tracker->isUpdating() ) {
        /*
         * When only units are being changed, don't treat changes
         * to adjuster values as object changes.
         */
        return;
    }
    _update = true;

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Inkscape::Selection *selection = desktop->getSelection();
    SPDocument *document = desktop->getDocument();

    document->ensureUpToDate ();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Geom::OptRect bbox_vis = selection->visualBounds();
    Geom::OptRect bbox_geom = selection->geometricBounds();

    int prefs_bbox = prefs->getInt("/tools/bounding_box");
    SPItem::BBoxType bbox_type = (prefs_bbox == 0)?
        SPItem::VISUAL_BBOX : SPItem::GEOMETRIC_BBOX;
    Geom::OptRect bbox_user = selection->bounds(bbox_type);

    if ( !bbox_user ) {
        _update = false;
        return;
    }

    gdouble x0 = 0;
    gdouble y0 = 0;
    gdouble x1 = 0;
    gdouble y1 = 0;
    gdouble xrel = 0;
    gdouble yrel = 0;
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    if (unit->type == Inkscape::Util::UNIT_TYPE_LINEAR) {
        x0 = Quantity::convert(_adj_x->get_value(), unit, "px");
        y0 = Quantity::convert(_adj_y->get_value(), unit, "px");
        x1 = x0 + Quantity::convert(_adj_w->get_value(), unit, "px");
        xrel = Quantity::convert(_adj_w->get_value(), unit, "px") / bbox_user->dimensions()[Geom::X];
        y1 = y0 + Quantity::convert(_adj_h->get_value(), unit, "px");;
        yrel = Quantity::convert(_adj_h->get_value(), unit, "px") / bbox_user->dimensions()[Geom::Y];
    } else {
        double const x0_propn = _adj_x->get_value() / 100 / unit->factor;
        x0 = bbox_user->min()[Geom::X] * x0_propn;
        double const y0_propn = _adj_y->get_value() / 100 / unit->factor;
        y0 = y0_propn * bbox_user->min()[Geom::Y];
        xrel = _adj_w->get_value() / (100 / unit->factor);
        x1 = x0 + xrel * bbox_user->dimensions()[Geom::X];
        yrel = _adj_h->get_value() / (100 / unit->factor);
        y1 = y0 + yrel * bbox_user->dimensions()[Geom::Y];
    }

    // Keep proportions if lock is on
    if ( gtk_toggle_action_get_active(_lock) ) {
        if (adj == _adj_h) {
            x1 = x0 + yrel * bbox_user->dimensions()[Geom::X];
        } else if (adj == _adj_w) {
            y1 = y0 + xrel * bbox_user->dimensions()[Geom::Y];
        }
    }

    // scales and moves, in px
    double mh = fabs(x0 - bbox_user->min()[Geom::X]);
    double sh = fabs(x1 - bbox_user->max()[Geom::X]);
    double mv = fabs(y0 - bbox_user->min()[Geom::Y]);
    double sv = fabs(y1 - bbox_user->max()[Geom::Y]);

    // unless the unit is %, convert the scales and moves to the unit
    if (unit->type == Inkscape::Util::UNIT_TYPE_LINEAR) {
        mh = Quantity::convert(mh, "px", unit);
        sh = Quantity::convert(sh, "px", unit);
        mv = Quantity::convert(mv, "px", unit);
        sv = Quantity::convert(sv, "px", unit);
    }

    // do the action only if one of the scales/moves is greater than half the last significant
    // digit in the spinbox (currently spinboxes have 3 fractional digits, so that makes 0.0005). If
    // the value was changed by the user, the difference will be at least that much; otherwise it's
    // just rounding difference between the spinbox value and actual value, so no action is
    // performed
    char const * const actionkey = ( mh > 5e-4 ? "selector:toolbar:move:horizontal" :
                                     sh > 5e-4 ? "selector:toolbar:scale:horizontal" :
                                     mv > 5e-4 ? "selector:toolbar:move:vertical" :
                                     sv > 5e-4 ? "selector:toolbar:scale:vertical" : nullptr );

    if (actionkey != nullptr) {

        // FIXME: fix for GTK breakage, see comment in SelectedStyle::on_opacity_changed
        desktop->getCanvas()->forceFullRedrawAfterInterruptions(0);

        bool transform_stroke = prefs->getBool("/options/transform/stroke", true);
        bool preserve = prefs->getBool("/options/preservetransform/value", false);

        Geom::Affine scaler;
        if (bbox_type == SPItem::VISUAL_BBOX) {
            scaler = get_scale_transform_for_variable_stroke (*bbox_vis, *bbox_geom, transform_stroke, preserve, x0, y0, x1, y1);
        } else {
            // 1) We could have use the newer get_scale_transform_for_variable_stroke() here, but to avoid regressions
            // we'll just use the old get_scale_transform_for_uniform_stroke() for now.
            // 2) get_scale_transform_for_uniform_stroke() is intended for visual bounding boxes, not geometrical ones!
            // we'll trick it into using a geometric bounding box though, by setting the stroke width to zero
            scaler = get_scale_transform_for_uniform_stroke (*bbox_geom, 0, 0, false, false, x0, y0, x1, y1);
        }

        selection->applyAffine(scaler);
        DocumentUndo::maybeDone(document, actionkey, SP_VERB_CONTEXT_SELECT,
                                _("Transform by toolbar"));

        // resume interruptibility
        desktop->getCanvas()->endForcedFullRedraws();
    }

    _update = false;
}

void
SelectToolbar::layout_widget_update(Inkscape::Selection *sel)
{
    if (_update) {
        return;
    }

    _update = true;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    using Geom::X;
    using Geom::Y;
    if ( sel && !sel->isEmpty() ) {
        int prefs_bbox = prefs->getInt("/tools/bounding_box", 0);
        SPItem::BBoxType bbox_type = (prefs_bbox ==0)?
            SPItem::VISUAL_BBOX : SPItem::GEOMETRIC_BBOX;
        Geom::OptRect const bbox(sel->bounds(bbox_type));
        if ( bbox ) {
            Unit const *unit = _tracker->getActiveUnit();
            g_return_if_fail(unit != nullptr);

            struct { char const *key; double val; } const keyval[] = {
                { "X", bbox->min()[X] },
                { "Y", bbox->min()[Y] },
                { "width", bbox->dimensions()[X] },
                { "height", bbox->dimensions()[Y] }
            };

            if (unit->type == Inkscape::Util::UNIT_TYPE_DIMENSIONLESS) {
                double const val = unit->factor * 100;
                _adj_x->set_value(val);
                _adj_y->set_value(val);
                _adj_w->set_value(val);
                _adj_h->set_value(val);
                _tracker->setFullVal( _adj_x->gobj(), keyval[0].val );
                _tracker->setFullVal( _adj_y->gobj(), keyval[1].val );
                _tracker->setFullVal( _adj_w->gobj(), keyval[2].val );
                _tracker->setFullVal( _adj_h->gobj(), keyval[3].val );
            } else {
                _adj_x->set_value(Quantity::convert(keyval[0].val, "px", unit));
                _adj_y->set_value(Quantity::convert(keyval[1].val, "px", unit));
                _adj_w->set_value(Quantity::convert(keyval[2].val, "px", unit));
                _adj_h->set_value(Quantity::convert(keyval[3].val, "px", unit));
            }
        }
    }

    _update = false;
}

void
SelectToolbar::on_inkscape_selection_modified(Inkscape::Selection *selection, guint flags)
{
    if ((_desktop->getSelection() == selection) // only respond to changes in our desktop
        && (flags & (SP_OBJECT_MODIFIED_FLAG        |
                     SP_OBJECT_PARENT_MODIFIED_FLAG |
                     SP_OBJECT_CHILD_MODIFIED_FLAG   )))
    {
        layout_widget_update(selection);
    }
}

void
SelectToolbar::on_inkscape_selection_changed(Inkscape::Selection *selection)
{
    if (_desktop->getSelection() == selection) { // only respond to changes in our desktop
        gboolean setActive = (selection && !selection->isEmpty());
        if ( _context_actions ) {
            for (auto & contextAction : *_context_actions) {
                if ( setActive != gtk_action_is_sensitive(contextAction) ) {
                    gtk_action_set_sensitive( contextAction, setActive );
                }
            }
        }

        layout_widget_update(selection);
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
