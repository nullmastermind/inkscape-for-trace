// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkflow-box widget.
 * This widget allow pack widgets in a flowbox with a controller to show-hide
 *
 * Author:
 *   Jabier Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2018 Jabier Arraiza
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_INK_FLOW_BOX_H
#define INKSCAPE_INK_FLOW_BOX_H

#include <gtkmm/actionbar.h>
#include <gtkmm/box.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/flowboxchild.h>
#include <gtkmm/togglebutton.h>
#include <sigc++/signal.h>

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * A flowbox widget with filter controller for dialogs.
 */

class InkFlowBox : public Gtk::Box {
  public:
    InkFlowBox(const gchar *name);
    ~InkFlowBox() override;
    void insert(Gtk::Widget *widget, Glib::ustring label, gint pos, bool active, int minwidth);
    void on_toggle(gint pos, Gtk::ToggleButton *tbutton);
    void on_global_toggle(Gtk::ToggleButton *tbutton);
    void set_visible(gint pos, bool visible);
    bool on_filter(Gtk::FlowBoxChild *child);
    Glib::ustring getPrefsPath(gint pos);
    /**
     * Construct a InkFlowBox.
     */

  private:
    Gtk::FlowBox _flowbox;
    Gtk::ActionBar _controller;
    gint showing;
    bool sensitive;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_INK_FLOW_BOX_H

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
