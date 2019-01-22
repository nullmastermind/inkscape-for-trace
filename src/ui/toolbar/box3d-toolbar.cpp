// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * 3d box aux toolbar
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

#include <gtk/gtk.h>
#include <glibmm/i18n.h>

#include "box3d-toolbar.h"

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "widgets/toolbox.h"
#include "verbs.h"

#include "object/box3d.h"
#include "object/persp3d.h"

#include "ui/icon-names.h"
#include "ui/pref-pusher.h"
#include "ui/tools/box3d-tool.h"
#include "ui/uxmanager.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/ink-toggle-action.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;

//########################
//##       3D Box       ##
//########################



static Inkscape::XML::NodeEventVector box3d_persp_tb_repr_events =
{
    nullptr, /* child_added */
    nullptr, /* child_removed */
    Inkscape::UI::Toolbar::Box3DToolbar::event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

namespace Inkscape {
namespace UI {
namespace Toolbar {
GtkWidget *
Box3DToolbar::prep(SPDesktop *desktop, GtkActionGroup* mainActions)
{
    auto holder = new Box3DToolbar(desktop);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    SPDocument *document = desktop->getDocument();
    Persp3DImpl *persp_impl = document->getCurrentPersp3DImpl();

    /* Angle X */
    {
        gchar const* labels[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        gdouble values[] = {-90, -60, -30, 0, 30, 60, 90};
        holder->_angle_x_action = create_adjustment_action( "3DBoxAngleXAction",
                                                            _("Angle in X direction"), _("Angle X:"),
                                                            // Translators: PL is short for 'perspective line'
                                                            _("Angle of PLs in X direction"),
                                                            "/tools/shapes/3dbox/box3d_angle_x", 30,
                                                            TRUE, "altx-box3d",
                                                            -360.0, 360.0, 1.0, 10.0,
                                                            labels, values, G_N_ELEMENTS(labels)
                                                            );
        ege_adjustment_action_set_focuswidget(holder->_angle_x_action, GTK_WIDGET(desktop->canvas));
        holder->_angle_x_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_angle_x_action));
        holder->_angle_x_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &Box3DToolbar::angle_value_changed),
                                                                        holder->_angle_x_adj, Proj::X));
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_angle_x_action) );
    }

    if (!persp_impl || !persp3d_VP_is_finite(persp_impl, Proj::X)) {
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_x_action), TRUE );
    } else {
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_x_action), FALSE );
    }


    /* VP X state */
    {
        holder->_vp_x_state_action = ink_toggle_action_new( "3DBoxVPXStateAction",
                                                            // Translators: VP is short for 'vanishing point'
                                                            _("State of VP in X direction"),
                                                            _("Toggle VP in X direction between 'finite' and 'infinite' (=parallel)"),
                                                            INKSCAPE_ICON("perspective-parallel"),
                                                            GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_vp_x_state_action ) );
        g_signal_connect_after( G_OBJECT(holder->_vp_x_state_action), "toggled", G_CALLBACK(Box3DToolbar::vp_state_changed), (gpointer)Proj::X );
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_x_action), !prefs->getBool("/tools/shapes/3dbox/vp_x_state", true) );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_vp_x_state_action), prefs->getBool("/tools/shapes/3dbox/vp_x_state", true) );
    }

    /* Angle Y */
    {
        gchar const* labels[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        gdouble values[] = {-90, -60, -30, 0, 30, 60, 90};
        holder->_angle_y_action = create_adjustment_action( "3DBoxAngleYAction",
                                         _("Angle in Y direction"), _("Angle Y:"),
                                         // Translators: PL is short for 'perspective line'
                                         _("Angle of PLs in Y direction"),
                                         "/tools/shapes/3dbox/box3d_angle_y", 30,
                                         FALSE, nullptr,
                                         -360.0, 360.0, 1.0, 10.0,
                                         labels, values, G_N_ELEMENTS(labels)
                                         );
        ege_adjustment_action_set_focuswidget(holder->_angle_y_action, GTK_WIDGET(desktop->canvas));
        holder->_angle_y_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_angle_y_action));
        holder->_angle_y_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &Box3DToolbar::angle_value_changed),
                                                                        holder->_angle_y_adj, Proj::Y));
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_angle_y_action) );
    }

    if (!persp_impl || !persp3d_VP_is_finite(persp_impl, Proj::Y)) {
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_y_action), TRUE );
    } else {
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_y_action), FALSE );
    }

    /* VP Y state */
    {
        holder->_vp_y_state_action = ink_toggle_action_new( "3DBoxVPYStateAction",
                                                            // Translators: VP is short for 'vanishing point'
                                                            _("State of VP in Y direction"),
                                                            _("Toggle VP in Y direction between 'finite' and 'infinite' (=parallel)"),
                                                            INKSCAPE_ICON("perspective-parallel"),
                                                            GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_vp_y_state_action ) );
        g_signal_connect_after( G_OBJECT(holder->_vp_y_state_action), "toggled", G_CALLBACK(Box3DToolbar::vp_state_changed), (gpointer)Proj::Y);
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_y_action), !prefs->getBool("/tools/shapes/3dbox/vp_y_state", true) );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_vp_y_state_action), prefs->getBool("/tools/shapes/3dbox/vp_y_state", true) );
    }

    /* Angle Z */
    {
        gchar const* labels[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        gdouble values[] = {-90, -60, -30, 0, 30, 60, 90};
        holder->_angle_z_action = create_adjustment_action( "3DBoxAngleZAction",
                                         _("Angle in Z direction"), _("Angle Z:"),
                                         // Translators: PL is short for 'perspective line'
                                         _("Angle of PLs in Z direction"),
                                         "/tools/shapes/3dbox/box3d_angle_z", 30,
                                         FALSE, nullptr,
                                         -360.0, 360.0, 1.0, 10.0,
                                         labels, values, G_N_ELEMENTS(labels)
                                         );
        ege_adjustment_action_set_focuswidget(holder->_angle_z_action, GTK_WIDGET(desktop->canvas));
        holder->_angle_z_adj = Glib::wrap(ege_adjustment_action_get_adjustment(holder->_angle_z_action));
        holder->_angle_z_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*holder, &Box3DToolbar::angle_value_changed),
                                                                        holder->_angle_z_adj, Proj::Z));
        gtk_action_group_add_action( mainActions, GTK_ACTION(holder->_angle_z_action) );
    }

    if (!persp_impl || !persp3d_VP_is_finite(persp_impl, Proj::Z)) {
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_z_action), TRUE );
    } else {
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_z_action), FALSE );
    }

    /* VP Z state */
    {
        holder->_vp_z_state_action = ink_toggle_action_new( "3DBoxVPZStateAction",
                                                            // Translators: VP is short for 'vanishing point'
                                                            _("State of VP in Z direction"),
                                                            _("Toggle VP in Z direction between 'finite' and 'infinite' (=parallel)"),
                                                            INKSCAPE_ICON("perspective-parallel"),
                                                            GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( holder->_vp_z_state_action ) );
        g_signal_connect_after( G_OBJECT(holder->_vp_z_state_action), "toggled", G_CALLBACK(Box3DToolbar::vp_state_changed), (gpointer)Proj::Z );
        gtk_action_set_sensitive( GTK_ACTION(holder->_angle_z_action), !prefs->getBool("/tools/shapes/3dbox/vp_z_state", true) );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(holder->_vp_z_state_action), prefs->getBool("/tools/shapes/3dbox/vp_z_state", true) );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*holder, &Box3DToolbar::check_ec));

    return GTK_WIDGET(holder->gobj());
}

void
Box3DToolbar::angle_value_changed(Glib::RefPtr<Gtk::Adjustment> &adj,
                                  Proj::Axis                     axis)
{
    SPDocument *document = _desktop->getDocument();

    // quit if run by the attr_changed or selection changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;

    std::list<Persp3D *> sel_persps = _desktop->getSelection()->perspList();
    if (sel_persps.empty()) {
        // this can happen when the document is created; we silently ignore it
        return;
    }
    Persp3D *persp = sel_persps.front();

    persp->perspective_impl->tmat.set_infinite_direction (axis,
                                                          adj->get_value());
    persp->updateRepr();

    // TODO: use the correct axis here, too
    DocumentUndo::maybeDone(document, "perspangle", SP_VERB_CONTEXT_3DBOX, _("3D Box: Change perspective (angle of infinite axis)"));

    _freeze = false;
}

void
Box3DToolbar::vp_state_changed(GtkToggleAction *act,
                               Proj::Axis       axis )
{
    // TODO: Take all selected perspectives into account
    std::list<Persp3D *> sel_persps = SP_ACTIVE_DESKTOP->getSelection()->perspList();
    if (sel_persps.empty()) {
        // this can happen when the document is created; we silently ignore it
        return;
    }
    Persp3D *persp = sel_persps.front();

    bool set_infinite = gtk_toggle_action_get_active(act);
    persp3d_set_VP_state (persp, axis, set_infinite ? Proj::VP_INFINITE : Proj::VP_FINITE);
}

void
Box3DToolbar::check_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_BOX3D_CONTEXT(ec)) {
        _changed = desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &Box3DToolbar::selection_changed));
        selection_changed(desktop->getSelection());
    } else {
        if (_changed)
            _changed.disconnect();
    }
}

Box3DToolbar::~Box3DToolbar()
{
    if (_repr) { // remove old listener
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }
}

/**
 *  \param selection Should not be NULL.
 */
// FIXME: This should rather be put into persp3d-reference.cpp or something similar so that it reacts upon each
//        Change of the perspective, and not of the current selection (but how to refer to the toolbar then?)
void
Box3DToolbar::selection_changed(Inkscape::Selection *selection)
{
    // Here the following should be done: If all selected boxes have finite VPs in a certain direction,
    // disable the angle entry fields for this direction (otherwise entering a value in them should only
    // update the perspectives with infinite VPs and leave the other ones untouched).

    Inkscape::XML::Node *persp_repr = nullptr;

    if (_repr) { // remove old listener
        _repr->removeListenerByData(this);
        Inkscape::GC::release(_repr);
        _repr = nullptr;
    }

    SPItem *item = selection->singleItem();
    SPBox3D *box = dynamic_cast<SPBox3D *>(item);
    if (box) {
        // FIXME: Also deal with multiple selected boxes
        Persp3D *persp = box3d_get_perspective(box);
        persp_repr = persp->getRepr();
        if (persp_repr) {
            _repr = persp_repr;
            Inkscape::GC::anchor(_repr);
            _repr->addListener(&box3d_persp_tb_repr_events, this);
            _repr->synthesizeEvents(&box3d_persp_tb_repr_events, this);

            SP_ACTIVE_DOCUMENT->setCurrentPersp3D(persp3d_get_from_repr(_repr));
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setString("/tools/shapes/3dbox/persp", _repr->attribute("id"));

            _freeze = true;
            resync_toolbar(_repr);
            _freeze = false;
        }
    }
}

void
Box3DToolbar::resync_toolbar(Inkscape::XML::Node *persp_repr)
{
    if (!persp_repr) {
        g_print ("No perspective given to box3d_resync_toolbar().\n");
        return;
    }

    Persp3D *persp = persp3d_get_from_repr(persp_repr);
    if (!persp) {
        // Hmm, is it an error if this happens?
        return;
    }
    set_button_and_adjustment(persp, Proj::X,
                              _angle_x_adj,
                              GTK_ACTION(_angle_x_action),
                              GTK_TOGGLE_ACTION(_vp_x_state_action));
    set_button_and_adjustment(persp, Proj::Y,
                              _angle_y_adj,
                              GTK_ACTION(_angle_y_action),
                              GTK_TOGGLE_ACTION(_vp_y_state_action));
    set_button_and_adjustment(persp, Proj::Z,
                              _angle_z_adj,
                              GTK_ACTION(_angle_z_action),
                              GTK_TOGGLE_ACTION(_vp_z_state_action));
}

void
Box3DToolbar::set_button_and_adjustment(Persp3D                        *persp,
                                        Proj::Axis                      axis,
                                        Glib::RefPtr<Gtk::Adjustment>&  adj,
                                        GtkAction                      *act,
                                        GtkToggleAction                *tact)
{
    // TODO: Take all selected perspectives into account but don't touch the state button if not all of them
    //       have the same state (otherwise a call to box3d_vp_z_state_changed() is triggered and the states
    //       are reset).
    bool is_infinite = !persp3d_VP_is_finite(persp->perspective_impl, axis);

    if (is_infinite) {
        gtk_toggle_action_set_active(tact, TRUE);
        gtk_action_set_sensitive(act, TRUE);

        double angle = persp3d_get_infinite_angle(persp, axis);
        if (angle != Geom::infinity()) { // FIXME: We should catch this error earlier (don't show the spinbutton at all)
            adj->set_value(normalize_angle(angle));
        }
    } else {
        gtk_toggle_action_set_active(tact, FALSE);
        gtk_action_set_sensitive(act, FALSE);
    }
}

void
Box3DToolbar::event_attr_changed(Inkscape::XML::Node *repr,
                                 gchar const         * /*name*/,
                                 gchar const         * /*old_value*/,
                                 gchar const         * /*new_value*/,
                                 bool                  /*is_interactive*/,
                                 gpointer              data)
{
    auto toolbar = reinterpret_cast<Box3DToolbar*>(data);

    // quit if run by the attr_changed or selection changed listener
    if (toolbar->_freeze) {
        return;
    }

    // set freeze so that it can be caught in box3d_angle_z_value_changed() (to avoid calling
    // SPDocumentUndo::maybeDone() when the document is undo insensitive)
    toolbar->_freeze = true;

    // TODO: Only update the appropriate part of the toolbar
//    if (!strcmp(name, "inkscape:vp_z")) {
        toolbar->resync_toolbar(repr);
//    }

    Persp3D *persp = persp3d_get_from_repr(repr);
    persp3d_update_box_reprs(persp);

    toolbar->_freeze = false;
}

/**
 * \brief normalize angle so that it lies in the interval [0,360]
 *
 * TODO: Isn't there something in 2Geom or cmath that does this?
 */
double
Box3DToolbar::normalize_angle(double a) {
    double angle = a + ((int) (a/360.0))*360;
    if (angle < 0) {
        angle += 360.0;
    }
    return angle;
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
