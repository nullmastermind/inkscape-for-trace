// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Andrius R. <knutux@gmail.com>
 *   Johan Engelen
 *
 * Copyright (C) 2006-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DIALOG_GUIDELINE_H
#define INKSCAPE_DIALOG_GUIDELINE_H

#include <gtkmm/checkbutton.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

#include "ui/widget/unit-menu.h"
#include "ui/widget/scalar-unit.h"
#include "ui/widget/entry.h"
#include <2geom/point.h>

class SPGuide;
class SPDesktop;

namespace Inkscape {
namespace UI {

namespace Widget {
  class UnitMenu;
};

namespace Dialogs {

/**
 * Dialog for modifying guidelines.
 */
class GuidelinePropertiesDialog : public Gtk::Dialog {
public:
    GuidelinePropertiesDialog(SPGuide *guide, SPDesktop *desktop);
    ~GuidelinePropertiesDialog() override;

    Glib::ustring     getName() const { return "GuidelinePropertiesDialog"; }

    static void showDialog(SPGuide *guide, SPDesktop *desktop);

protected:
    void _setup();

    void _onOK();
    void _onDelete();
    void _onDuplicate();

    void _response(gint response);
    void _modeChanged();

private:
    GuidelinePropertiesDialog(GuidelinePropertiesDialog const &) = delete; // no copy
    GuidelinePropertiesDialog &operator=(GuidelinePropertiesDialog const &) = delete; // no assign

    SPDesktop *_desktop;
    SPGuide *_guide;

    Gtk::Grid   _layout_table;
    Gtk::Label  _label_name;
    Gtk::Label  _label_descr;
    Gtk::CheckButton _locked_toggle;
    Gtk::CheckButton _relative_toggle;
    static bool _relative_toggle_status; // remember the status of the _relative_toggle_status button across instances
    Inkscape::UI::Widget::UnitMenu _unit_menu;
    Inkscape::UI::Widget::ScalarUnit _spin_button_x;
    Inkscape::UI::Widget::ScalarUnit _spin_button_y;
    Inkscape::UI::Widget::Entry _label_entry;
    Gtk::ColorButton _color;

    Inkscape::UI::Widget::ScalarUnit _spin_angle;
    static Glib::ustring _angle_unit_status; // remember the status of the _relative_toggle_status button across instances

    bool _mode;
    Geom::Point _oldpos;
    gdouble _oldangle;
};

} // namespace
} // namespace
} // namespace


#endif // INKSCAPE_DIALOG_GUIDELINE_H

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
