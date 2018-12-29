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
}
}

enum SPButtonType {
	SP_BUTTON_TYPE_NORMAL,
	SP_BUTTON_TYPE_TOGGLE
};

struct SPBChoiceData {
	guchar *px;
};

class SPButton : public Gtk::ToggleButton{
private:
    SPButtonType _type;
    GtkIconSize _lsize;
    unsigned int _psize;
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
    virtual void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
    virtual void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
    void on_clicked() override;

public:
    SPButton(GtkIconSize   size,
             SPButtonType  type,
             SPAction     *action,
             SPAction     *doubleclick_action);
    
    SPButton(GtkIconSize               size,
                   SPButtonType              type,
                   Inkscape::UI::View::View *view,
                   const gchar              *name,
                   const gchar              *tip);

    ~SPButton();

    void toggle_set_down(bool down);
};

#define SP_BUTTON_IS_DOWN(b) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b))

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
