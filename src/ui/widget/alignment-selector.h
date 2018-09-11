// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * anchor-selector.h
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef ANCHOR_SELECTOR_H_
#define ANCHOR_SELECTOR_H_

#include <gtkmm/bin.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>

namespace Inkscape {
namespace UI {
namespace Widget {

class AlignmentSelector : public Gtk::Bin
{
private:
    Gtk::Button _buttons[9];
    Gtk::Grid   _container;

    sigc::signal<void, int> _alignmentClicked;

    void setupButton(const Glib::ustring &icon, Gtk::Button &button);
    void btn_activated(int index);

public:

    sigc::signal<void, int> &on_alignmentClicked() { return _alignmentClicked; }

    AlignmentSelector();
    ~AlignmentSelector() override;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif /* ANCHOR_SELECTOR_H_ */

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
