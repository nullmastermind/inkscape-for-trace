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

namespace Inkscape {
namespace UI {
namespace Toolbar {

GtkWidget *
PencilToolbar::prep_pencil(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new PencilToolbar(desktop);
    toolbar->add_freehand_mode_toggle(mainActions, true);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* min pressure */
    {
        toolbar->_minpressure = create_adjustment_action( "MinPressureAction",
                                                          _("Min pressure"), _("Min:"), _("Min percent of pressure"),
                                                          "/tools/freehand/pencil/minpressure", 0,
                                                          FALSE, nullptr,
                                                          0, 100, 1, 0,
                                                          nullptr, nullptr, 0,
                                                          nullptr, 0 ,0);
        
        ege_adjustment_action_set_focuswidget(toolbar->_minpressure, GTK_WIDGET(desktop->canvas));
        toolbar->_minpressure_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_minpressure));
        toolbar->_minpressure_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &PencilToolbar::minpressure_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_minpressure) );
        if (prefs->getInt("/tools/freehand/pencil/freehand-mode", 0) == 3) {
            gtk_action_set_visible( GTK_ACTION(toolbar->_minpressure), true );
        } else {
            gtk_action_set_visible( GTK_ACTION(toolbar->_minpressure), false );
        }
    }
    /* max pressure */
    {
        toolbar->_maxpressure = create_adjustment_action( "MaxPressureAction",
                                                          _("Max pressure"), _("Max:"), _("Max percent of pressure"),
                                                          "/tools/freehand/pencil/maxpressure", 100,
                                                          FALSE, nullptr,
                                                          0, 100, 1, 0,
                                                          nullptr, nullptr, 0,
                                                          nullptr, 0 ,0);
        ege_adjustment_action_set_focuswidget(toolbar->_maxpressure, GTK_WIDGET(desktop->canvas));
        toolbar->_maxpressure_adj = Glib::wrap(ege_adjustment_action_get_adjustment(toolbar->_maxpressure));
        toolbar->_maxpressure_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &PencilToolbar::maxpressure_value_changed));
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_maxpressure) );
        if (prefs->getInt("/tools/freehand/pencil/freehand-mode", 0) == 3) {
            gtk_action_set_visible( GTK_ACTION(toolbar->_maxpressure), true );
        } else {
            gtk_action_set_visible( GTK_ACTION(toolbar->_maxpressure), false );
        }
    }
    /* Use pressure */
    {
        InkToggleAction* itact = ink_toggle_action_new( "PencilPressureAction",
                                                        _("Use pressure input"),
                                                        _("Use pressure input"),
                                                        INKSCAPE_ICON("draw-use-pressure"),
                                                        GTK_ICON_SIZE_SMALL_TOOLBAR );
        bool pressure = prefs->getBool(toolbar->freehand_tool_name() + "/pressure", false);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(itact), pressure );
        g_signal_connect_after(  G_OBJECT(itact), "toggled", G_CALLBACK(PencilToolbar::use_pencil_pressure), toolbar);
        gtk_action_group_add_action( mainActions, GTK_ACTION(itact) );
        if (pressure) {
            gtk_action_set_visible( GTK_ACTION( toolbar->_minpressure ), true );
            gtk_action_set_visible( GTK_ACTION( toolbar->_maxpressure ), true );
        } else {
            gtk_action_set_visible( GTK_ACTION( toolbar->_minpressure ), false );
            gtk_action_set_visible( GTK_ACTION( toolbar->_maxpressure ), false );
        }
    }
    /* Tolerance */
    {
        gchar const* labels[] = {_("(many nodes, rough)"), _("(default)"), nullptr, nullptr, nullptr, nullptr, _("(few nodes, smooth)")};
        gdouble values[] = {1, 10, 20, 30, 50, 75, 100};
        EgeAdjustmentAction *eact = create_adjustment_action( "PencilToleranceAction",
                                                              _("Smoothing:"), _("Smoothing: "),
                                                              _("How much smoothing (simplifying) is applied to the line"),
                                                              "/tools/freehand/pencil/tolerance",
                                                              3.0,
                                                              TRUE, "altx-pencil",
                                                              1, 100.0, 0.5, 1.0,
                                                              labels, values, G_N_ELEMENTS(labels),
                                                              nullptr /*unit tracker*/,
                                                              1, 2);
        ege_adjustment_action_set_focuswidget(eact, GTK_WIDGET(desktop->canvas));

        toolbar->_tolerance_adj = Glib::wrap(ege_adjustment_action_get_adjustment(eact));
        toolbar->_tolerance_adj->signal_value_changed().connect(sigc::mem_fun(*toolbar, &PencilToolbar::tolerance_value_changed));
        ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }

    /* advanced shape options */
    toolbar->freehand_add_advanced_shape_options(mainActions, true);

    /* Reset */
    {
        InkAction* inky = ink_action_new( "PencilResetAction",
                                          _("Defaults"),
                                          _("Reset pencil parameters to defaults (use Inkscape Preferences > Tools to change defaults)"),
                                          INKSCAPE_ICON("edit-clear"),
                                          GTK_ICON_SIZE_SMALL_TOOLBAR );
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(PencilToolbar::defaults), toolbar );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }
    /* LPE simplify based tolerance */
    {
        toolbar->_simplify = ink_toggle_action_new( "PencilLpeSimplify",
                                                    _("LPE based interactive simplify"),
                                                    _("LPE based interactive simplify"),
                                                    INKSCAPE_ICON("interactive_simplify"),
                                                    GTK_ICON_SIZE_SMALL_TOOLBAR );
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(toolbar->_simplify), prefs->getInt("/tools/freehand/pencil/simplify", 0) );
        g_signal_connect_after(  G_OBJECT(toolbar->_simplify), "toggled", G_CALLBACK(PencilToolbar::freehand_simplify_lpe), toolbar) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_simplify) );
        guint freehandMode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        if (freehandMode == 2) {
            gtk_action_set_visible( GTK_ACTION( toolbar->_simplify ), false );
        } else {
            gtk_action_set_visible( GTK_ACTION( toolbar->_simplify ), true );
        }
    }
    /* LPE simplify flatten */
    {
        toolbar->_flatten_simplify = ink_action_new( "PencilLpeSimplifyFlatten",
                                                     _("LPE simplify flatten"),
                                                     _("LPE simplify flatten"),
                                                     INKSCAPE_ICON("flatten"),
                                                     GTK_ICON_SIZE_SMALL_TOOLBAR );
        g_signal_connect_after( G_OBJECT(toolbar->_flatten_simplify), "activate", G_CALLBACK(PencilToolbar::simplify_flatten), toolbar );
        gtk_action_group_add_action( mainActions, GTK_ACTION(toolbar->_flatten_simplify) );
        guint freehandMode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        if (freehandMode == 2 || !prefs->getInt("/tools/freehand/pencil/simplify", 0)) {
            gtk_action_set_visible( GTK_ACTION(toolbar->_flatten_simplify), false );
        } else {
            gtk_action_set_visible( GTK_ACTION(toolbar->_flatten_simplify), true );
        }
    }

    return GTK_WIDGET(toolbar->gobj());
}

PencilToolbar::~PencilToolbar()
{
    if(_repr) {
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }
}

void
PencilToolbar::freehand_mode_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt(freehand_tool_name() + "/freehand-mode", mode);

    if (mode == 1 || mode == 2) {
        gtk_action_set_visible( GTK_ACTION( _flatten_spiro_bspline ), true );
    } else {
        gtk_action_set_visible( GTK_ACTION( _flatten_spiro_bspline ), false );
    }

    bool visible = (mode != 2);

    if (_flatten_simplify) {
        gtk_action_set_visible(GTK_ACTION(_flatten_simplify), visible);
    }

    if (_simplify) {
        gtk_action_set_visible(GTK_ACTION(_simplify), visible);
    }
}

/* This is used in generic functions below to share large portions of code between pen and pencil tool */
Glib::ustring const
PencilToolbar::freehand_tool_name()
{
    return ( tools_isactive(_desktop, TOOLS_FREEHAND_PEN)
             ? "/tools/freehand/pen"
             : "/tools/freehand/pencil" );
}

void
PencilToolbar::add_freehand_mode_toggle(GtkActionGroup* mainActions,
                                        bool tool_is_pencil)
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

    act->signal_changed().connect(sigc::mem_fun(*this, &PencilToolbar::freehand_mode_changed));

    /* LPE bspline spiro flatten */
    _flatten_spiro_bspline = ink_action_new( tool_is_pencil ? "FlattenSpiroBsplinePencil" :
                                             "FlattenSpiroBsplinePen",
                                             _("LPE spiro or bspline flatten"),
                                             _("LPE spiro or bspline flatten"),
                                             INKSCAPE_ICON("flatten"),
                                             GTK_ICON_SIZE_SMALL_TOOLBAR );
    g_signal_connect_after( G_OBJECT(_flatten_spiro_bspline), "activate", G_CALLBACK(PencilToolbar::flatten_spiro_bspline), this);
    gtk_action_group_add_action( mainActions, GTK_ACTION(_flatten_spiro_bspline) );

    if (freehandMode == 1 || freehandMode == 2) {
        gtk_action_set_visible( GTK_ACTION( _flatten_spiro_bspline ), true );
    } else {
        gtk_action_set_visible( GTK_ACTION( _flatten_spiro_bspline ), false );
    }
}

void
PencilToolbar::minpressure_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/freehand/pencil/minpressure", _minpressure_adj->get_value());
}

void
PencilToolbar::maxpressure_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/freehand/pencil/maxpressure", _maxpressure_adj->get_value());
}

void
PencilToolbar::use_pencil_pressure(InkToggleAction* itact, gpointer data) {
    auto toolbar = reinterpret_cast<PencilToolbar *>(data);

    bool pressure = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION(itact) );
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(toolbar->freehand_tool_name() + "/pressure", pressure);
    if (pressure) {
        gtk_action_set_visible( GTK_ACTION( toolbar->_minpressure ), true );
        gtk_action_set_visible( GTK_ACTION( toolbar->_maxpressure ), true );
        toolbar->_shape_action->set_visible (false);
    } else {
        gtk_action_set_visible( GTK_ACTION( toolbar->_minpressure ), false );
        gtk_action_set_visible( GTK_ACTION( toolbar->_maxpressure ), false );
        toolbar->_shape_action->set_visible (true);
    }
}

void
PencilToolbar::freehand_add_advanced_shape_options(GtkActionGroup* mainActions, bool tool_is_pencil)
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

    _shape_action =
        InkSelectOneAction::create( tool_is_pencil ?
                                    "SetPencilShapeAction" :
                                    "SetPenShapeAction", // Name
                                    _("Shape"),          // Label
                                    _("Shape of new paths drawn by this tool"), // Tooltip
                                    "Not Used",          // Icon
                                    store );             // Tree store

    _shape_action->use_radio( false );
    _shape_action->use_icon( false );
    _shape_action->use_label( true );
    _shape_action->use_group_label( true );
    int shape = prefs->getInt( (tool_is_pencil ?
                                "/tools/freehand/pencil/shape" :
                                "/tools/freehand/pen/shape" ), 0);
    _shape_action->set_active( shape );

    gtk_action_group_add_action( mainActions, GTK_ACTION( _shape_action->gobj() ));

    bool hide = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0) == 3 ||
        (tool_is_pencil && prefs->getBool("/tools/freehand/pencil/pressure", false));
    _shape_action->set_visible( !hide );

    _shape_action->signal_changed().connect(sigc::mem_fun(*this, &PencilToolbar::freehand_change_shape));
}

void
PencilToolbar::freehand_change_shape(int shape) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt(freehand_tool_name() + "/shape", shape);
}

void
PencilToolbar::defaults(GtkWidget * /*widget*/, GObject *obj)
{
    auto toolbar = reinterpret_cast<PencilToolbar *>(obj);

    // fixme: make settable
    gdouble tolerance = 4;

    toolbar->_tolerance_adj->set_value(tolerance);

#if !GTK_CHECK_VERSION(3,18,0)
    toolbar->_tolerance_adj->value_changed();
#endif

    if(toolbar->_desktop->canvas) gtk_widget_grab_focus(GTK_WIDGET(toolbar->_desktop->canvas));
}

void
PencilToolbar::freehand_simplify_lpe(InkToggleAction* itact, GObject *data)
{
    auto toolbar = reinterpret_cast<PencilToolbar *>(data);

    bool simplify = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION(itact) );
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(toolbar->freehand_tool_name() + "/simplify", simplify);
    gtk_action_set_visible( GTK_ACTION( toolbar->_flatten_simplify ), simplify );
    if (simplify) {
        gtk_action_set_visible( GTK_ACTION( toolbar->_flatten_simplify ), true );
    } else {
        gtk_action_set_visible( GTK_ACTION( toolbar->_flatten_simplify ), false );
    }
}

void
PencilToolbar::simplify_flatten(GtkWidget * /*widget*/, GObject *data)
{
    auto toolbar = reinterpret_cast<PencilToolbar *>(data);
    auto selected = toolbar->_desktop->getSelection()->items();
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
        toolbar->_desktop->getSelection()->remove(lpeitem->getRepr());
        toolbar->_desktop->getSelection()->add(lpeitem->getRepr());
        sp_lpe_item_update_patheffect(lpeitem, false, false);
    }
}

void
PencilToolbar::flatten_spiro_bspline(GtkWidget * /*widget*/, gpointer data)
{
    auto toolbar = reinterpret_cast<PencilToolbar *>(data);
    auto selected = toolbar->_desktop->getSelection()->items();
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
        toolbar->_desktop->getSelection()->remove(lpeitem->getRepr());
        toolbar->_desktop->getSelection()->add(lpeitem->getRepr());
        sp_lpe_item_update_patheffect(lpeitem, false, false);
    }
}

GtkWidget *
PencilToolbar::prep_pen(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto toolbar = new PencilToolbar(desktop);
    toolbar->add_freehand_mode_toggle(mainActions, false);
    toolbar->freehand_add_advanced_shape_options(mainActions, false);
    return GTK_WIDGET(toolbar->gobj());
}

void
PencilToolbar::tolerance_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _freeze = true;
    prefs->setDouble("/tools/freehand/pencil/tolerance",
                     _tolerance_adj->get_value());
    _freeze = false;
    auto selected = _desktop->getSelection()->items();
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
