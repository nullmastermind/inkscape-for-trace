// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * anchor-selector.cpp
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/alignment-selector.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"

#include <gtkmm/image.h>

namespace Inkscape {
namespace UI {
namespace Widget {

void AlignmentSelector::setupButton(const Glib::ustring& icon, Gtk::Button& button) {
    Gtk::Image *buttonIcon = Gtk::manage(sp_get_icon_image(icon, Gtk::ICON_SIZE_SMALL_TOOLBAR));
    buttonIcon->show();

    button.set_relief(Gtk::RELIEF_NONE);
    button.show();
    button.add(*buttonIcon);
    button.set_can_focus(false);
}

AlignmentSelector::AlignmentSelector()
    : _container()
{
    set_halign(Gtk::ALIGN_CENTER);
    // clang-format off
    setupButton(INKSCAPE_ICON("boundingbox_top_left"),     _buttons[0]);
    setupButton(INKSCAPE_ICON("boundingbox_top"),          _buttons[1]);
    setupButton(INKSCAPE_ICON("boundingbox_top_right"),    _buttons[2]);
    setupButton(INKSCAPE_ICON("boundingbox_left"),         _buttons[3]);
    setupButton(INKSCAPE_ICON("boundingbox_center"),       _buttons[4]);
    setupButton(INKSCAPE_ICON("boundingbox_right"),        _buttons[5]);
    setupButton(INKSCAPE_ICON("boundingbox_bottom_left"),  _buttons[6]);
    setupButton(INKSCAPE_ICON("boundingbox_bottom"),       _buttons[7]);
    setupButton(INKSCAPE_ICON("boundingbox_bottom_right"), _buttons[8]);
    // clang-format on

    _container.set_row_homogeneous();
    _container.set_column_homogeneous(true);

    for(int i = 0; i < 9; ++i) {
        _buttons[i].signal_clicked().connect(
            sigc::bind(sigc::mem_fun(*this, &AlignmentSelector::btn_activated), i));

        _container.attach(_buttons[i], i % 3, i / 3, 1, 1);
    }

    this->add(_container);
}

AlignmentSelector::~AlignmentSelector()
{
    // TODO Auto-generated destructor stub
}

void AlignmentSelector::btn_activated(int index)
{
    _alignmentClicked.emit(index);
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
