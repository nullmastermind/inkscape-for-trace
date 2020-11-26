// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple dialog for creating grid type arrangements of selected objects
 *
 * Authors:
 *   Bob Jamison ( based off trace dialog)
 *   John Cliff
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *   Declara Denis
 *
 * Copyright (C) 2004 Bob Jamison
 * Copyright (C) 2004 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/dialog/grid-arrange-tab.h"
#include "ui/dialog/polar-arrange-tab.h"

#include <glibmm/i18n.h>

#include "tile.h"
#include "verbs.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

ArrangeDialog::ArrangeDialog()
    : DialogBase("/dialogs/gridtiler", SP_VERB_SELECTION_ARRANGE)
{
    _arrangeBox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    _notebook = Gtk::manage(new Gtk::Notebook());
    _gridArrangeTab = Gtk::manage(new GridArrangeTab(this));
    _polarArrangeTab = Gtk::manage(new PolarArrangeTab(this));

    _notebook->append_page(*_gridArrangeTab, C_("Arrange dialog", "Rectangular grid"));
    _notebook->append_page(*_polarArrangeTab, C_("Arrange dialog", "Polar Coordinates"));
    _arrangeBox->pack_start(*_notebook);
    pack_start(*_arrangeBox);

    // Add button
    _arrangeButton = Gtk::manage(new Gtk::Button(C_("Arrange dialog", "_Arrange")));
    _arrangeButton->signal_clicked().connect(sigc::mem_fun(*this, &ArrangeDialog::_apply));
    _arrangeButton->set_use_underline(true);
    _arrangeButton->set_tooltip_text(_("Arrange selected objects"));

    Gtk::ButtonBox *button_box = Gtk::manage(new Gtk::ButtonBox());
    button_box->set_layout(Gtk::BUTTONBOX_END);
    button_box->set_spacing(6);
    button_box->set_border_width(4);

    button_box->pack_end(*_arrangeButton);
    pack_end(*button_box);

    show();
    show_all_children();
}

void ArrangeDialog::_apply()
{
	switch(_notebook->get_current_page())
	{
	case 0:
		_gridArrangeTab->arrange();
		break;
	case 1:
		_polarArrangeTab->arrange();
		break;
	}
}

void ArrangeDialog::update()
{
    if (!_app) {
        std::cerr << "ArrangeDialog::update(): _app is null" << std::endl;
        return;
    }

    SPDesktop *desktop = getDesktop();

    _gridArrangeTab->setDesktop(desktop);
}

} //namespace Dialog
} //namespace UI
} //namespace Inkscape

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
