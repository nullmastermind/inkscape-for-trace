// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Simple guideline dialog.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Andrius R. <knutux@gmail.com>
 *   Johan Engelen
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "guides.h"

#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "message-context.h"
#include "verbs.h"

#include "include/gtkmm_version.h"

#include "object/sp-guide.h"
#include "object/sp-namedview.h"

#include "ui/dialog-events.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/spinbutton.h"

#include "widgets/desktop-widget.h"

namespace Inkscape {
namespace UI {
namespace Dialogs {

GuidelinePropertiesDialog::GuidelinePropertiesDialog(SPGuide *guide, SPDesktop *desktop)
: _desktop(desktop), _guide(guide),
  _locked_toggle(_("Lo_cked")),
  _relative_toggle(_("Rela_tive change")),
  _spin_button_x(C_("Guides", "_X:"), "", UNIT_TYPE_LINEAR, "", "", &_unit_menu),
  _spin_button_y(C_("Guides", "_Y:"), "", UNIT_TYPE_LINEAR, "", "", &_unit_menu),
  _label_entry(_("_Label:"), _("Optionally give this guideline a name")),
  _spin_angle(_("_Angle:"), "", UNIT_TYPE_RADIAL),
  _mode(true), _oldpos(0.,0.), _oldangle(0.0)
{
    _locked_toggle.set_use_underline();
    _locked_toggle.set_tooltip_text(_("Lock the movement of guides"));
    _relative_toggle.set_use_underline();
    _relative_toggle.set_tooltip_text(_("Move and/or rotate the guide relative to current settings"));
}

bool GuidelinePropertiesDialog::_relative_toggle_status = false; // initialize relative checkbox status for when this dialog is opened for first time
Glib::ustring GuidelinePropertiesDialog::_angle_unit_status = DEG; // initialize angle unit status

GuidelinePropertiesDialog::~GuidelinePropertiesDialog() {
    // save current status
    _relative_toggle_status = _relative_toggle.get_active();
    _angle_unit_status = _spin_angle.getUnit()->abbr;
}

void GuidelinePropertiesDialog::showDialog(SPGuide *guide, SPDesktop *desktop) {
    GuidelinePropertiesDialog dialog(guide, desktop);
    dialog._setup();
    dialog.run();
}

void GuidelinePropertiesDialog::_modeChanged()
{
    _mode = !_relative_toggle.get_active();
    if (!_mode) {
        // relative
        _spin_angle.setValue(0);

        _spin_button_y.setValue(0);
        _spin_button_x.setValue(0);
    } else {
        // absolute
        _spin_angle.setValueKeepUnit(_oldangle, DEG);

        _spin_button_x.setValueKeepUnit(_oldpos[Geom::X], "px");
        _spin_button_y.setValueKeepUnit(_oldpos[Geom::Y], "px");
    }
}

void GuidelinePropertiesDialog::_onOK()
{
    this->_onOKimpl();
    DocumentUndo::done(_guide->document, SP_VERB_NONE,
                       _("Set guide properties"));

}

void GuidelinePropertiesDialog::_onOKimpl()
{
    double deg_angle = _spin_angle.getValue(DEG);
    if (!_mode)
        deg_angle += _oldangle;
    Geom::Point normal;
    if ( deg_angle == 90. || deg_angle == 270. || deg_angle == -90. || deg_angle == -270.) {
        normal = Geom::Point(1.,0.);
    } else if ( deg_angle == 0. || deg_angle == 180. || deg_angle == -180.) {
        normal = Geom::Point(0.,1.);
    } else {
        double rad_angle = Geom::rad_from_deg( deg_angle );
        normal = Geom::rot90(Geom::Point::polar(rad_angle, 1.0));
    }
    //To allow reposition from dialog
    _guide->set_locked(false, false);

    _guide->set_normal(normal, true);

    double const points_x = _spin_button_x.getValue("px");
    double const points_y = _spin_button_y.getValue("px");
    Geom::Point newpos(points_x, points_y);
    if (!_mode)
        newpos += _oldpos;

    _guide->moveto(newpos, true);

    const gchar* name = g_strdup( _label_entry.getEntry()->get_text().c_str() );

    _guide->set_label(name, true);
    
    const bool locked = _locked_toggle.get_active();

    _guide->set_locked(locked, true);
    
    g_free((gpointer) name);

    const auto c = _color.get_rgba();
    unsigned r = c.get_red_u()/257, g = c.get_green_u()/257, b = c.get_blue_u()/257;
    //TODO: why 257? verify this!
    // don't know why, but introduced: 761f7da58cd6d625b88c24eee6fae1b7fa3bfcdd

    _guide->set_color(r, g, b, true);
}

void GuidelinePropertiesDialog::_onDelete()
{
    SPDocument *doc = _guide->document;
    sp_guide_remove(_guide);
    DocumentUndo::done(doc, SP_VERB_NONE, 
                       _("Delete guide"));
}

void GuidelinePropertiesDialog::_onDuplicate()
{
    _guide = _guide->duplicate();
    this->_onOKimpl();
    DocumentUndo::done(_guide->document, SP_VERB_NONE, _("Duplicate guide"));
}

void GuidelinePropertiesDialog::_response(gint response)
{
    switch (response) {
	case Gtk::RESPONSE_OK:
            _onOK();
            break;
	case -12:
            _onDelete();
            break;
	case -13:
            _onDuplicate();
            break;
	case Gtk::RESPONSE_CANCEL:
            break;
	case Gtk::RESPONSE_DELETE_EVENT:
            break;
	default:
            g_assert_not_reached();
    }
}

void GuidelinePropertiesDialog::_setup() {
    set_title(_("Guideline"));
    add_button(_("_OK"), Gtk::RESPONSE_OK);
    add_button(_("_Duplicate"), -13);
    add_button(_("_Delete"), -12);
    add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);

    auto mainVBox = get_content_area();
    _layout_table.set_row_spacing(4);
    _layout_table.set_column_spacing(4);
    _layout_table.set_border_width(4);

    mainVBox->pack_start(_layout_table, false, false, 0);

    _label_name.set_label("foo0");
    _label_name.set_halign(Gtk::ALIGN_START);
    _label_name.set_valign(Gtk::ALIGN_CENTER);

    _label_descr.set_label("foo1");
    _label_descr.set_halign(Gtk::ALIGN_START);
    _label_descr.set_valign(Gtk::ALIGN_CENTER);
    
    _label_name.set_halign(Gtk::ALIGN_FILL);
    _label_name.set_valign(Gtk::ALIGN_FILL);
    _layout_table.attach(_label_name, 0, 0, 3, 1);

    _label_descr.set_halign(Gtk::ALIGN_FILL);
    _label_descr.set_valign(Gtk::ALIGN_FILL);
    _layout_table.attach(_label_descr, 0, 1, 3, 1);

    _label_entry.set_halign(Gtk::ALIGN_FILL);
    _label_entry.set_valign(Gtk::ALIGN_FILL);
    _label_entry.set_hexpand();
    _layout_table.attach(_label_entry, 1, 2, 2, 1);

    _color.set_halign(Gtk::ALIGN_FILL);
    _color.set_valign(Gtk::ALIGN_FILL);
    _color.set_hexpand();
    _color.set_margin_end(6);
    _layout_table.attach(_color, 1, 3, 2, 1);

    // unitmenus
    /* fixme: We should allow percents here too, as percents of the canvas size */
    _unit_menu.setUnitType(UNIT_TYPE_LINEAR);
    _unit_menu.setUnit("px");
    if (_desktop->namedview->display_units) {
        _unit_menu.setUnit( _desktop->namedview->display_units->abbr );
    }
    _spin_angle.setUnit(_angle_unit_status);

    // position spinbuttons
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    size_t minimumexponent = std::min(std::abs(prefs->getInt("/options/svgoutput/minimumexponent", -8)), 5); //we limit to 5 to minimize rounding errors 
    _spin_button_x.setDigits(minimumexponent);
    _spin_button_x.setAlignment(1.0);
    _spin_button_x.setIncrements(1.0, 10.0);
    _spin_button_x.setRange(-1e6, 1e6);
    _spin_button_y.setDigits(minimumexponent);
    
    _spin_button_y.setAlignment(1.0);
    _spin_button_y.setIncrements(1.0, 10.0);
    _spin_button_y.setRange(-1e6, 1e6);

    _spin_button_x.set_halign(Gtk::ALIGN_FILL);
    _spin_button_x.set_valign(Gtk::ALIGN_FILL);
    _spin_button_x.set_hexpand();
    _layout_table.attach(_spin_button_x, 1, 4, 1, 1);
    
    _spin_button_y.set_halign(Gtk::ALIGN_FILL);
    _spin_button_y.set_valign(Gtk::ALIGN_FILL);
    _spin_button_y.set_hexpand();
    _layout_table.attach(_spin_button_y, 1, 5, 1, 1);

    _unit_menu.set_halign(Gtk::ALIGN_FILL);
    _unit_menu.set_valign(Gtk::ALIGN_FILL);
    _unit_menu.set_margin_end(6);
    _layout_table.attach(_unit_menu, 2, 4, 1, 1);

    // angle spinbutton
    _spin_angle.setDigits(3);
    _spin_angle.setDigits(minimumexponent);
    _spin_angle.setAlignment(1.0);
    _spin_angle.setIncrements(1.0, 10.0);
    _spin_angle.setRange(-3600., 3600.);

    _spin_angle.set_halign(Gtk::ALIGN_FILL);
    _spin_angle.set_valign(Gtk::ALIGN_FILL);
    _spin_angle.set_hexpand();
    _layout_table.attach(_spin_angle, 1, 6, 2, 1);

    // mode radio button
    _relative_toggle.set_halign(Gtk::ALIGN_FILL);
    _relative_toggle.set_valign(Gtk::ALIGN_FILL);
    _relative_toggle.set_hexpand();
    _relative_toggle.set_margin_start(6);
    _layout_table.attach(_relative_toggle, 1, 7, 2, 1);

    // locked radio button
    _locked_toggle.set_halign(Gtk::ALIGN_FILL);
    _locked_toggle.set_valign(Gtk::ALIGN_FILL);
    _locked_toggle.set_hexpand();
    _locked_toggle.set_margin_start(6);
    _layout_table.attach(_locked_toggle, 1, 8, 2, 1);

    _relative_toggle.signal_toggled().connect(sigc::mem_fun(*this, &GuidelinePropertiesDialog::_modeChanged));
    _relative_toggle.set_active(_relative_toggle_status);

    bool global_guides_lock = _desktop->namedview->lockguides;
    if(global_guides_lock){
        _locked_toggle.set_sensitive(false);
    }
    _locked_toggle.set_active(_guide->getLocked());

    // This results in the dialog closing when entering a value in one of the spinbuttons and pressing enter (see LP bug 484187)
    auto sbx = dynamic_cast<UI::Widget::SpinButton *>(_spin_button_x.getWidget());
    auto sby = dynamic_cast<UI::Widget::SpinButton *>(_spin_button_y.getWidget());
    auto sba = dynamic_cast<UI::Widget::SpinButton *>(_spin_button_y.getWidget());

    if(sbx) sbx->signal_activate().connect(sigc::mem_fun(this, &GuidelinePropertiesDialog::on_sb_activate));
    if(sby) sby->signal_activate().connect(sigc::mem_fun(this, &GuidelinePropertiesDialog::on_sb_activate));
    if(sba) sba->signal_activate().connect(sigc::mem_fun(this, &GuidelinePropertiesDialog::on_sb_activate));

    // dialog
    set_default_response(Gtk::RESPONSE_OK);
    signal_response().connect(sigc::mem_fun(*this, &GuidelinePropertiesDialog::_response));

    // initialize dialog
    _oldpos = _guide->getPoint();
    if (_guide->isVertical()) {
        _oldangle = 90;
    } else if (_guide->isHorizontal()) {
        _oldangle = 0;
    } else {
        _oldangle = Geom::deg_from_rad( std::atan2( - _guide->getNormal()[Geom::X], _guide->getNormal()[Geom::Y] ) );
    }

    {
        // FIXME holy crap!!!
        Inkscape::XML::Node *repr = _guide->getRepr();
        const gchar *guide_id = repr->attribute("id");
        gchar *label = g_strdup_printf(_("Guideline ID: %s"), guide_id);
        _label_name.set_label(label);
        g_free(label);
    }
    {
        gchar *guide_description = _guide->description(false);
        gchar *label = g_strdup_printf(_("Current: %s"), guide_description);
        g_free(guide_description);
        _label_descr.set_markup(label);
        g_free(label);
    }

    // init name entry
    _label_entry.getEntry()->set_text(_guide->getLabel() ? _guide->getLabel() : "");

    Gdk::RGBA c;
    c.set_rgba(((_guide->getColor()>>24)&0xff) / 255.0, ((_guide->getColor()>>16)&0xff) / 255.0, ((_guide->getColor()>>8)&0xff) / 255.0);
    _color.set_rgba(c);

    _modeChanged(); // sets values of spinboxes.

    if ( _oldangle == 90. || _oldangle == 270. || _oldangle == -90. || _oldangle == -270.) {
        _spin_button_x.grabFocusAndSelectEntry();
    } else if ( _oldangle == 0. || _oldangle == 180. || _oldangle == -180.) {
        _spin_button_y.grabFocusAndSelectEntry();
    } else {
        _spin_angle.grabFocusAndSelectEntry();
    }

    set_position(Gtk::WIN_POS_MOUSE);

    show_all_children();
    set_modal(true);
    _desktop->setWindowTransient (gobj());
    property_destroy_with_parent() = true;
}

void
GuidelinePropertiesDialog::on_sb_activate()
{
    activate_default();
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
