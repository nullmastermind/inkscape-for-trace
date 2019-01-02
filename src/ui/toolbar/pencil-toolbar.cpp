// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Pencil aux toolbar
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

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "pencil-toolbar.h"

#include "desktop.h"
#include "selection.h"

#include "widgets/ink-action.h"
#include "widgets/ink-toggle-action.h"
#include "widgets/toolbox.h"

#include "live_effects/lpe-bspline.h"
#include "live_effects/lpe-powerstroke.h"
#include "live_effects/lpe-simplify.h"
#include "live_effects/lpe-spiro.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"

#include "display/curve.h"

#include "object/sp-shape.h"

#include "ui/icon-names.h"
#include "ui/tools-switch.h"
#include "ui/tools/pen-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/ink-select-one-action.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/spinbutton-events.h"


using Inkscape::UI::UXManager;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;

//########################
//##     Pen/Pencil     ##
//########################
static void sp_flatten_spiro_bspline(GtkWidget * /*widget*/, GObject *obj);
/* This is used in generic functions below to share large portions of code between pen and pencil tool */
static Glib::ustring const freehand_tool_name(GObject *dataKludge)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(dataKludge, "desktop"));
    return ( tools_isactive(desktop, TOOLS_FREEHAND_PEN)
             ? "/tools/freehand/pen"
             : "/tools/freehand/pencil" );
}

static void freehand_mode_changed(GObject* tbl, int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt(freehand_tool_name(tbl) + "/freehand-mode", mode);

    if (mode == 1 || mode == 2) {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(tbl, "flatten_spiro_bspline") ), true );
    } else {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(tbl, "flatten_spiro_bspline") ), false );
    }
    if (mode == 2) {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(tbl, "flatten_simplify") ), false );
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(tbl, "simplify") ), false );
    } else {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(tbl, "flatten_simplify") ), true );
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(tbl, "simplify") ), true );
    }
}

static void use_pencil_pressure(InkToggleAction* itact, GObject *dataKludge) {
    bool pressure = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION(itact) );
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(freehand_tool_name(dataKludge) + "/pressure", pressure);
    if (pressure) {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "minpressure") ), true );
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "maxpressure") ), true );
        InkSelectOneAction* act =
            static_cast<InkSelectOneAction*>( g_object_get_data( dataKludge, "shape_action" ) );
        act->set_visible (false);
    } else {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "minpressure") ), false );
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "maxpressure") ), false );
        InkSelectOneAction* act =
            static_cast<InkSelectOneAction*>( g_object_get_data( dataKludge, "shape_action" ) );
        act->set_visible (true);
    }
}

static void sp_add_freehand_mode_toggle(GtkActionGroup* mainActions, GObject* holder, bool tool_is_pencil)
{
    /* Freehand mode toggle buttons */

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint freehandMode = prefs->getInt(( tool_is_pencil ?
                                         "/tools/freehand/pencil/freehand-mode" :
                                         "/tools/freehand/pen/freehand-mode" ), 0);
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    InkSelectOneActionColumns columns;

    Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

    Gtk::TreeModel::Row row;

    row = *(store->append());
    row[columns.col_label    ] = _("Bezier");
    row[columns.col_tooltip  ] = _("Create regular Bezier path");
    row[columns.col_icon     ] = INKSCAPE_ICON("path-mode-bezier");
    row[columns.col_sensitive] = true;

    row = *(store->append());
    row[columns.col_label    ] = _("Spiro");
    row[columns.col_tooltip  ] = _("Create Spiro path");
    row[columns.col_icon     ] = INKSCAPE_ICON("path-mode-spiro");
    row[columns.col_sensitive] = true;

    row = *(store->append());
    row[columns.col_label    ] = _("BSpline");
    row[columns.col_tooltip  ] = _("Create BSpline path");
    row[columns.col_icon     ] = INKSCAPE_ICON("path-mode-bspline");
    row[columns.col_sensitive] = true;

    if (!tool_is_pencil) {
        row = *(store->append());
        row[columns.col_label    ] = _("Zigzag");
        row[columns.col_tooltip  ] = _("Create a sequence of straight line segments");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-mode-polyline");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Paraxial");
        row[columns.col_tooltip  ] = _("Create a sequence of paraxial line segments");
        row[columns.col_icon     ] = INKSCAPE_ICON("path-mode-polyline-paraxial");
        row[columns.col_sensitive] = true;
    }

    InkSelectOneAction* act =
        InkSelectOneAction::create( tool_is_pencil ?
                                    "FreehandModeActionPencil" :
                                    "FreehandModeActionPen",
                                    _("Mode"),           // Label
                                    _("Mode of new lines drawn by this tool"), // Tooltip
                                    "Not Used",          // Icon
                                    store );             // Tree store
    act->use_radio( true );
    act->use_label( true );
    act->set_active( freehandMode );

    gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
    // g_object_set_data( dataKludge, "flat_action", act );

    act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&freehand_mode_changed), holder));

    /* LPE bspline spiro flatten */
    InkAction* inky = ink_action_new( tool_is_pencil ? "FlattenSpiroBsplinePencil" :
                                      "FlattenSpiroBsplinePen",
                                      _("LPE spiro or bspline flatten"),
                                      _("LPE spiro or bspline flatten"),
                                      INKSCAPE_ICON("flatten"),
                                      GTK_ICON_SIZE_SMALL_TOOLBAR );
    g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_flatten_spiro_bspline), holder );
    gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    g_object_set_data( holder, "flatten_spiro_bspline", inky );
    if (freehandMode == 1 || freehandMode == 2) {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "flatten_spiro_bspline") ), true );
    } else {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "flatten_spiro_bspline") ), false );
    }
}

static void freehand_change_shape(GObject *dataKludge, int shape) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt(freehand_tool_name(dataKludge) + "/shape", shape);
}

static void freehand_simplify_lpe(InkToggleAction* itact, GObject *dataKludge) {
    bool simplify = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION(itact) );
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(freehand_tool_name(dataKludge) + "/simplify", simplify);
    gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "flatten_simplify") ), simplify );
    if (simplify) {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "flatten_simplify") ), true );
    } else {
        gtk_action_set_visible( GTK_ACTION( g_object_get_data(dataKludge, "flatten_simplify") ), false );
    }
}

static void freehand_add_advanced_shape_options(GtkActionGroup* mainActions, GObject* holder, bool tool_is_pencil)
{
    /*advanced shape options */

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    std::vector<gchar*> freehand_shape_dropdown_items_list = {
        const_cast<gchar *>(C_("Freehand shape", "None")),
        _("Triangle in"),
        _("Triangle out"),
        _("Ellipse"),
        _("From clipboard"),
        _("Bend from clipboard"),
        _("Last applied")
    };

    InkSelectOneActionColumns columns;

    Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

    Gtk::TreeModel::Row row;

    for (auto item:freehand_shape_dropdown_items_list) {

        row = *(store->append());
        row[columns.col_label    ] = item;
        row[columns.col_tooltip  ] = ("");
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;
    }

    InkSelectOneAction* act =
        InkSelectOneAction::create( tool_is_pencil ?
                                    "SetPencilShapeAction" :
                                    "SetPenShapeAction", // Name
                                    _("Shape"),          // Label
                                    _("Shape of new paths drawn by this tool"), // Tooltip
                                    "Not Used",          // Icon
                                    store );             // Tree store

    act->use_radio( false );
    act->use_icon( false );
    act->use_label( true );
    act->use_group_label( true );
    int shape = prefs->getInt( (tool_is_pencil ?
                                "/tools/freehand/pencil/shape" :
                                "/tools/freehand/pen/shape" ), 0);
    act->set_active( shape );

    gtk_action_group_add_action( mainActions, GTK_ACTION( act->gobj() ));
    g_object_set_data( holder, "shape_action", act );

    bool hide = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0) == 3 ||
        (tool_is_pencil && prefs->getBool("/tools/freehand/pencil/pressure", false));
    act->set_visible( !hide );

    act->signal_changed().connect(sigc::bind<0>(sigc::ptr_fun(&freehand_change_shape), holder));
}

void sp_pen_toolbox_prep(SPDesktop * /*desktop*/, GtkActionGroup* mainActions, GObject* holder)
{
    sp_add_freehand_mode_toggle(mainActions, holder, false);
    freehand_add_advanced_shape_options(mainActions, holder, false);
}


static void sp_pencil_tb_defaults(GtkWidget * /*widget*/, GObject *obj)
{
    GtkWidget *tbl = GTK_WIDGET(obj);

    GtkAdjustment *adj;

    // fixme: make settable
    gdouble tolerance = 4;

    adj = GTK_ADJUSTMENT(g_object_get_data(obj, "tolerance"));
    gtk_adjustment_set_value(adj, tolerance);

#if !GTK_CHECK_VERSION(3,18,0)
    gtk_adjustment_value_changed(adj);
#endif

    spinbutton_defocus(tbl);
}

static void sp_flatten_spiro_bspline(GtkWidget * /*widget*/, GObject *obj)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(obj, "desktop"));
    auto selected = desktop->getSelection()->items();
    SPLPEItem* lpeitem = nullptr;
    for (auto it(selected.begin()); it != selected.end(); ++it){
        lpeitem = dynamic_cast<SPLPEItem*>(*it);
        if (lpeitem && lpeitem->hasPathEffect()){
            PathEffectList lpelist = lpeitem->getEffectList();
            PathEffectList::iterator i;
            for (i = lpelist.begin(); i != lpelist.end(); ++i) {
                LivePathEffectObject *lpeobj = (*i)->lpeobject;
                if (lpeobj) {
                    Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                    if (dynamic_cast<Inkscape::LivePathEffect::LPEBSpline *>(lpe) || 
                        dynamic_cast<Inkscape::LivePathEffect::LPESpiro *>(lpe)) 
                    {
                        SPShape * shape = dynamic_cast<SPShape *>(lpeitem);
                        if(shape){
                            SPCurve * c = shape->getCurveForEdit();
                            lpe->doEffect(c);
                            lpeitem->setCurrentPathEffect(*i);
                            if (lpelist.size() > 1){
                                lpeitem->removeCurrentPathEffect(true);
                                shape->setCurveBeforeLPE(c);
                            } else {
                                lpeitem->removeCurrentPathEffect(false);
                                shape->setCurve(c, false);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    if (lpeitem) {
        desktop->getSelection()->remove(lpeitem->getRepr());
        desktop->getSelection()->add(lpeitem->getRepr());
        sp_lpe_item_update_patheffect(lpeitem, false, false);
    }
}

static void sp_simplify_flatten(GtkWidget * /*widget*/, GObject *obj)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(obj, "desktop"));
    auto selected = desktop->getSelection()->items();
    SPLPEItem* lpeitem = nullptr;
    for (auto it(selected.begin()); it != selected.end(); ++it){
        lpeitem = dynamic_cast<SPLPEItem*>(*it);
        if (lpeitem && lpeitem->hasPathEffect()){
            PathEffectList lpelist = lpeitem->getEffectList();
            PathEffectList::iterator i;
            for (i = lpelist.begin(); i != lpelist.end(); ++i) {
                LivePathEffectObject *lpeobj = (*i)->lpeobject;
                if (lpeobj) {
                    Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                    if (dynamic_cast<Inkscape::LivePathEffect::LPESimplify *>(lpe)) {
                        SPShape * shape = dynamic_cast<SPShape *>(lpeitem);
                        if(shape){
                            SPCurve * c = shape->getCurveForEdit();
                            lpe->doEffect(c);
                            lpeitem->setCurrentPathEffect(*i);
                            if (lpelist.size() > 1){
                                lpeitem->removeCurrentPathEffect(true);
                                shape->setCurveBeforeLPE(c);
                            } else {
                                lpeitem->removeCurrentPathEffect(false);
                                shape->setCurve(c, false);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    if (lpeitem) {
        desktop->getSelection()->remove(lpeitem->getRepr());
        desktop->getSelection()->add(lpeitem->getRepr());
        sp_lpe_item_update_patheffect(lpeitem, false, false);
    }
}

static void sp_minpressure_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    // quit if run by the attr_changed listener
    if (g_object_get_data( tbl, "freeze" )) {
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/freehand/pencil/minpressure", gtk_adjustment_get_value(adj));
}

static void sp_maxpressure_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    // quit if run by the attr_changed listener
    if (g_object_get_data( tbl, "freeze" )) {
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/freehand/pencil/maxpressure", gtk_adjustment_get_value(adj));
}

static void sp_pencil_tb_tolerance_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    // quit if run by the attr_changed listener
    if (g_object_get_data( tbl, "freeze" )) {
        return;
    }
    // in turn, prevent listener from responding
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(TRUE) );
    prefs->setDouble("/tools/freehand/pencil/tolerance",
            gtk_adjustment_get_value(adj));
    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(FALSE) );
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data(tbl, "desktop"));
    auto selected = desktop->getSelection()->items();
    for (auto it(selected.begin()); it != selected.end(); ++it){
        SPLPEItem* lpeitem = dynamic_cast<SPLPEItem*>(*it);
        if (lpeitem && lpeitem->hasPathEffect()){
            Inkscape::LivePathEffect::Effect* simplify = lpeitem->getPathEffectOfType(Inkscape::LivePathEffect::SIMPLIFY);
            if(simplify){
                Inkscape::LivePathEffect::LPESimplify *lpe_simplify = dynamic_cast<Inkscape::LivePathEffect::LPESimplify*>(simplify->getLPEObj()->get_lpe());
                if (lpe_simplify) {
                    double tol = prefs->getDoubleLimited("/tools/freehand/pencil/tolerance", 10.0, 1.0, 100.0);
                    tol = tol/(100.0*(102.0-tol));
                    std::ostringstream ss;
                    ss << tol;
                    Inkscape::LivePathEffect::Effect* powerstroke = lpeitem->getPathEffectOfType(Inkscape::LivePathEffect::POWERSTROKE);
                    bool simplified = false;
                    if(powerstroke){
                        Inkscape::LivePathEffect::LPEPowerStroke *lpe_powerstroke = dynamic_cast<Inkscape::LivePathEffect::LPEPowerStroke*>(powerstroke->getLPEObj()->get_lpe());
                        if(lpe_powerstroke){
                            lpe_powerstroke->getRepr()->setAttribute("is_visible", "false");
                            sp_lpe_item_update_patheffect(lpeitem, false, false);
                            SPShape *sp_shape = dynamic_cast<SPShape *>(lpeitem);
                            if (sp_shape) {
                                guint previous_curve_length = sp_shape->getCurve()->get_segment_count();
                                lpe_simplify->getRepr()->setAttribute("threshold", ss.str());
                                sp_lpe_item_update_patheffect(lpeitem, false, false);
                                simplified = true;
                                guint curve_length = sp_shape->getCurve()->get_segment_count();
                                std::vector<Geom::Point> ts = lpe_powerstroke->offset_points.data();
                                double factor = (double)curve_length/ (double)previous_curve_length;
                                for (auto & t : ts) {
                                    t[Geom::X] = t[Geom::X] * factor;
                                }
                                lpe_powerstroke->offset_points.param_setValue(ts);
                            }
                            lpe_powerstroke->getRepr()->setAttribute("is_visible", "true");
                            sp_lpe_item_update_patheffect(lpeitem, false, false);
                        }
                    }
                    if(!simplified){
                        lpe_simplify->getRepr()->setAttribute("threshold", ss.str());
                    }
                }
            }
        }
    }
}

/*
class PencilToleranceObserver : public Inkscape::Preferences::Observer {
public:
    PencilToleranceObserver(Glib::ustring const &path, GObject *x) : Observer(path), _obj(x)
    {
        g_object_set_data(_obj, "prefobserver", this);
    }
    virtual ~PencilToleranceObserver() {
        if (g_object_get_data(_obj, "prefobserver") == this) {
            g_object_set_data(_obj, "prefobserver", NULL);
        }
    }
    virtual void notify(Inkscape::Preferences::Entry const &val) {
        GObject* tbl = _obj;
        if (g_object_get_data( tbl, "freeze" )) {
            return;
        }
        g_object_set_data( tbl, "freeze", GINT_TO_POINTER(TRUE) );

        GtkAdjustment * adj = GTK_ADJUSTMENT(g_object_get_data(tbl, "tolerance"));

        double v = val.getDouble(adj->value);
        gtk_adjustment_set_value(adj, v);
        g_object_set_data( tbl, "freeze", GINT_TO_POINTER(FALSE) );
    }
private:
    GObject *_obj;
};
*/

void sp_pencil_toolbox_prep(SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder)
{
    sp_add_freehand_mode_toggle(mainActions, holder, true);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    EgeAdjustmentAction* eact = nullptr;

    
    /* min pressure */
    {
        eact = create_adjustment_action( "MinPressureAction",
                                         _("Min pressure"), _("Min:"), _("Min percent of pressure"),
                                         "/tools/freehand/pencil/minpressure", 0,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, nullptr,
                                         0, 100, 1, 0,
                                         nullptr, nullptr, 0,
                                         sp_minpressure_value_changed, nullptr, 0 ,0);
                                         
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        g_object_set_data( holder, "minpressure", eact );
        if (prefs->getInt("/tools/freehand/pencil/freehand-mode", 0) == 3) {
            gtk_action_set_visible( GTK_ACTION(eact), true );
        } else {
            gtk_action_set_visible( GTK_ACTION(eact), false );
        }
    }
    /* max pressure */
    {
        eact = create_adjustment_action( "MaxPressureAction",
                                         _("Max pressure"), _("Max:"), _("Max percent of pressure"),
                                         "/tools/freehand/pencil/maxpressure", 100,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, nullptr,
                                         0, 100, 1, 0,
                                         nullptr, nullptr, 0,
                                         sp_maxpressure_value_changed, nullptr, 0 ,0);
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
        g_object_set_data( holder, "maxpressure", eact );
        if (prefs->getInt("/tools/freehand/pencil/freehand-mode", 0) == 3) {
            gtk_action_set_visible( GTK_ACTION(eact), true );
        } else {
            gtk_action_set_visible( GTK_ACTION(eact), false );
        }
    }
    /* Use pressure */
    {
        InkToggleAction* itact = ink_toggle_action_new( "PencilPressureAction",
                                                        _("Use pressure input"),
                                                        _("Use pressure input"),
                                                        INKSCAPE_ICON("draw-use-pressure"),
                                                        GTK_ICON_SIZE_SMALL_TOOLBAR );
        bool pressure = prefs->getBool(freehand_tool_name(holder) + "/pressure", false);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(itact), pressure );
        g_signal_connect_after(  G_OBJECT(itact), "toggled", G_CALLBACK(use_pencil_pressure), holder) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
        if (pressure) {
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "minpressure") ), true );
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "maxpressure") ), true );
        } else {
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "minpressure") ), false );
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "maxpressure") ), false );
        }
    }
    /* Tolerance */
    {
        gchar const* labels[] = {_("(many nodes, rough)"), _("(default)"), nullptr, nullptr, nullptr, nullptr, _("(few nodes, smooth)")};
        gdouble values[] = {1, 10, 20, 30, 50, 75, 100};
        eact = create_adjustment_action( "PencilToleranceAction",
                                         _("Smoothing:"), _("Smoothing: "),
                 _("How much smoothing (simplifying) is applied to the line"),
                                         "/tools/freehand/pencil/tolerance",
                                         3.0,
                                         GTK_WIDGET(desktop->canvas),
                                         holder, TRUE, "altx-pencil",
                                         1, 100.0, 0.5, 1.0,
                                         labels, values, G_N_ELEMENTS(labels),
                                         sp_pencil_tb_tolerance_value_changed,
                                         nullptr /*unit tracker*/,
                                         1, 2);
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* advanced shape options */
    freehand_add_advanced_shape_options(mainActions, holder, true);

    /* Reset */
    {
        InkAction* inky = ink_action_new( "PencilResetAction",
                                          _("Defaults"),
                                          _("Reset pencil parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          GTK_ICON_SIZE_SMALL_TOOLBAR );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_pencil_tb_defaults), holder );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }
    /* LPE simplify based tolerance */
    {
        InkToggleAction* itact = ink_toggle_action_new( "PencilLpeSimplify",
                                                        _("LPE based interactive simplify"),
                                                        _("LPE based interactive simplify"),
                                                        INKSCAPE_ICON("interactive_simplify"),
                                                        GTK_ICON_SIZE_SMALL_TOOLBAR );
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(itact), prefs->getInt("/tools/freehand/pencil/simplify", 0) );
        g_object_set_data( holder, "simplify", itact );
        g_signal_connect_after(  G_OBJECT(itact), "toggled", G_CALLBACK(freehand_simplify_lpe), holder) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
        guint freehandMode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        if (freehandMode == 2) {
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "simplify") ), false );
        } else {
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "simplify") ), true );
        }
    }
    /* LPE simplify flatten */
    {
        InkAction* inky = ink_action_new( "PencilLpeSimplifyFlatten",
                                          _("LPE simplify flatten"),
                                          _("LPE simplify flatten"),
                                          INKSCAPE_ICON("flatten"),
                                          GTK_ICON_SIZE_SMALL_TOOLBAR );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_simplify_flatten), holder );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
        g_object_set_data( holder, "flatten_simplify", inky );
        guint freehandMode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        if (freehandMode == 2 || !prefs->getInt("/tools/freehand/pencil/simplify", 0)) {
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "flatten_simplify") ), false );
        } else {
            gtk_action_set_visible( GTK_ACTION( g_object_get_data(holder, "flatten_simplify") ), true );
        }
    }

    g_signal_connect( holder, "destroy", G_CALLBACK(purge_repr_listener), holder );

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
