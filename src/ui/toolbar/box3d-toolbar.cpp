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

#include "box3d-toolbar.h"

#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "verbs.h"

#include "object/box3d.h"
#include "object/persp3d.h"

#include "ui/icon-names.h"
#include "ui/pref-pusher.h"
#include "ui/tools/box3d-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/spin-button-tool-item.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;

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
Box3DToolbar::Box3DToolbar(SPDesktop *desktop)
        : Toolbar(desktop),
        _repr(nullptr),
        _freeze(false)
{
    auto prefs      = Inkscape::Preferences::get();
    auto document   = desktop->getDocument();
    auto persp_impl = document->getCurrentPersp3DImpl();

    /* Angle X */
    {
        std::vector<double> values = {-90, -60, -30, 0, 30, 60, 90};
        auto angle_x_val = prefs->getDouble("/tools/shapes/3dbox/box3d_angle_x", 30);
        _angle_x_adj = Gtk::Adjustment::create(angle_x_val, -360.0, 360.0, 1.0, 10.0);
        _angle_x_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("box3d-angle-x", _("Angle X:"), _angle_x_adj));
        // TRANSLATORS: PL is short for 'perspective line'
        _angle_x_item->set_tooltip_text(_("Angle of PLs in X direction"));
        _angle_x_item->set_custom_numeric_menu_data(values);
        _angle_x_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _angle_x_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::angle_value_changed),
                                                                _angle_x_adj, Proj::X));
        add(*_angle_x_item);
    }

    if (!persp_impl || !persp3d_VP_is_finite(persp_impl, Proj::X)) {
        _angle_x_item->set_sensitive(true);
    } else {
        _angle_x_item->set_sensitive(false);
    }

    /* VP X state */
    {
        // TRANSLATORS: VP is short for 'vanishing point'
        _vp_x_state_item = add_toggle_button(_("State of VP in X direction"),
                                             _("Toggle VP in X direction between 'finite' and 'infinite' (=parallel)"));
        _vp_x_state_item->set_icon_name(INKSCAPE_ICON("perspective-parallel"));
        _vp_x_state_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::vp_state_changed), Proj::X));
        _angle_x_item->set_sensitive( !prefs->getBool("/tools/shapes/3dbox/vp_x_state", true) );
        _vp_x_state_item->set_active( prefs->getBool("/tools/shapes/3dbox/vp_x_state", true) );
    }

    /* Angle Y */
    {
        auto angle_y_val = prefs->getDouble("/tools/shapes/3dbox/box3d_angle_y", 30);
        _angle_y_adj = Gtk::Adjustment::create(angle_y_val, -360.0, 360.0, 1.0, 10.0);
        _angle_y_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("box3d-angle-y", _("Angle Y:"), _angle_y_adj));
        // TRANSLATORS: PL is short for 'perspective line'
        _angle_y_item->set_tooltip_text(_("Angle of PLs in Y direction"));
        std::vector<double> values = {-90, -60, -30, 0, 30, 60, 90};
        _angle_y_item->set_custom_numeric_menu_data(values);
        _angle_y_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _angle_y_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::angle_value_changed),
                                                                _angle_y_adj, Proj::Y));
        add(*_angle_y_item);
    }

    if (!persp_impl || !persp3d_VP_is_finite(persp_impl, Proj::Y)) {
        _angle_y_item->set_sensitive(true);
    } else {
        _angle_y_item->set_sensitive(false);
    }

    /* VP Y state */
    {
        // TRANSLATORS: VP is short for 'vanishing point'
        _vp_y_state_item = add_toggle_button(_("State of VP in Y direction"),
                                             _("Toggle VP in Y direction between 'finite' and 'infinite' (=parallel)"));
        _vp_y_state_item->set_icon_name(INKSCAPE_ICON("perspective-parallel"));
        _vp_y_state_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::vp_state_changed), Proj::Y));
        _angle_y_item->set_sensitive( !prefs->getBool("/tools/shapes/3dbox/vp_y_state", true) );
        _vp_y_state_item->set_active( prefs->getBool("/tools/shapes/3dbox/vp_y_state", true) );
    }

    /* Angle Z */
    {
        auto angle_z_val = prefs->getDouble("/tools/shapes/3dbox/box3d_angle_z", 30);
        _angle_z_adj = Gtk::Adjustment::create(angle_z_val, -360.0, 360.0, 1.0, 10.0);
        _angle_z_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("box3d-angle-z", _("Angle Z:"), _angle_z_adj));
        // TRANSLATORS: PL is short for 'perspective line'
        _angle_z_item->set_tooltip_text(_("Angle of PLs in Z direction"));
        std::vector<double> values = {-90, -60, -30, 0, 30, 60, 90};
        _angle_z_item->set_custom_numeric_menu_data(values);
        _angle_z_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _angle_z_adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::angle_value_changed),
                                                                _angle_z_adj, Proj::Z));
        add(*_angle_z_item);
    }

    if (!persp_impl || !persp3d_VP_is_finite(persp_impl, Proj::Z)) {
        _angle_z_item->set_sensitive(true);
    } else {
        _angle_z_item->set_sensitive(false);
    }

    /* VP Z state */
    {
        // TRANSLATORS: VP is short for 'vanishing point'
        _vp_z_state_item = add_toggle_button(_("State of VP in Z direction"),
                                             _("Toggle VP in Z direction between 'finite' and 'infinite' (=parallel)"));
        _vp_z_state_item->set_icon_name(INKSCAPE_ICON("perspective-parallel"));
        _vp_z_state_item->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::vp_state_changed), Proj::Z));
        _angle_z_item->set_sensitive(!prefs->getBool("/tools/shapes/3dbox/vp_z_state", true));
        _vp_z_state_item->set_active( prefs->getBool("/tools/shapes/3dbox/vp_z_state", true) );
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*this, &Box3DToolbar::check_ec));

    show_all();
}

GtkWidget *
Box3DToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new Box3DToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
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
Box3DToolbar::vp_state_changed(Proj::Axis axis)
{
    // TODO: Take all selected perspectives into account
    auto sel_persps = SP_ACTIVE_DESKTOP->getSelection()->perspList();
    if (sel_persps.empty()) {
        // this can happen when the document is created; we silently ignore it
        return;
    }
    Persp3D *persp = sel_persps.front();

    Gtk::ToggleToolButton *btn = nullptr;

    switch(axis) {
        case Proj::X:
            btn = _vp_x_state_item;
            break;
        case Proj::Y:
            btn = _vp_y_state_item;
            break;
        case Proj::Z:
            btn = _vp_z_state_item;
            break;
        default:
            return;
    }

    bool set_infinite = btn->get_active();
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
                              _angle_x_item,
                              _vp_x_state_item);
    set_button_and_adjustment(persp, Proj::Y,
                              _angle_y_adj,
                              _angle_y_item,
                              _vp_y_state_item);
    set_button_and_adjustment(persp, Proj::Z,
                              _angle_z_adj,
                              _angle_z_item,
                              _vp_z_state_item);
}

void
Box3DToolbar::set_button_and_adjustment(Persp3D                        *persp,
                                        Proj::Axis                      axis,
                                        Glib::RefPtr<Gtk::Adjustment>&  adj,
                                        UI::Widget::SpinButtonToolItem *spin_btn,
                                        Gtk::ToggleToolButton          *toggle_btn)
{
    // TODO: Take all selected perspectives into account but don't touch the state button if not all of them
    //       have the same state (otherwise a call to box3d_vp_z_state_changed() is triggered and the states
    //       are reset).
    bool is_infinite = !persp3d_VP_is_finite(persp->perspective_impl, axis);

    if (is_infinite) {
        toggle_btn->set_active(true);
        spin_btn->set_sensitive(true);

        double angle = persp3d_get_infinite_angle(persp, axis);
        if (angle != Geom::infinity()) { // FIXME: We should catch this error earlier (don't show the spinbutton at all)
            adj->set_value(normalize_angle(angle));
        }
    } else {
        toggle_btn->set_active(false);
        spin_btn->set_sensitive(false);
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
