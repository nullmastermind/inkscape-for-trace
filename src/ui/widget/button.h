// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Generic button widget
 *//*
 * Authors:
 *  see git history
 *  Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_BUTTON_H
#define SEEN_SP_BUTTON_H

#include <gtkmm/togglebutton.h>
#include <sigc++/connection.h>

struct SPAction;

namespace Inkscape {
namespace UI {
namespace View {
class View;
}

namespace Widget {

enum ButtonType {
    BUTTON_TYPE_NORMAL,
    BUTTON_TYPE_TOGGLE
};

class Button : public Gtk::ToggleButton{
private:
    ButtonType _type;
    GtkIconSize _lsize;
    SPAction *_action;
    SPAction *_doubleclick_action;

    sigc::connection _c_set_active;
    sigc::connection _c_set_sensitive;

    void set_action(SPAction *action);
    void set_doubleclick_action(SPAction *action);
    void set_composed_tooltip(SPAction *action);
    void action_set_active(bool active);
    void perform_action();
    bool process_event(GdkEvent *event);

    sigc::connection _on_clicked;

protected:
    void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
    void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
    void on_clicked() override;

public:
    Button(GtkIconSize  size,
           ButtonType   type,
           SPAction    *action,
           SPAction    *doubleclick_action);
    
    Button(GtkIconSize               size,
           ButtonType                type,
           Inkscape::UI::View::View *view,
           const gchar              *name,
           const gchar              *tip);

    ~Button() override;

    void toggle_set_down(bool down);
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape
#endif // !SEEN_SP_BUTTON_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
