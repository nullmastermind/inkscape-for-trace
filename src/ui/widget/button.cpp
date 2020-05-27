// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Generic button widget
 *//*
 * Authors:
 *  see git history
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm.h>

#include "button.h"
#include "helper/action-context.h"
#include "helper/action.h"
#include "ui/icon-loader.h"
#include "ui/shortcuts.h"

namespace Inkscape {
namespace UI {
namespace Widget {

Button::~Button()
{
    if (_action) {
        _c_set_active.disconnect();
        _c_set_sensitive.disconnect();
        g_object_unref(_action);
    }

    if (_doubleclick_action) {
        set_doubleclick_action(nullptr);
    }
}

void
Button::get_preferred_width_vfunc(int &minimal_width, int &natural_width) const
{
    auto child = get_child();

    if (child) {
        child->get_preferred_width(minimal_width, natural_width);
    } else {
        minimal_width = 0;
        natural_width = 0;
    }

    auto context = get_style_context();

    auto padding = context->get_padding(context->get_state());
    auto border = context->get_border(context->get_state());

    minimal_width += MAX(2, padding.get_left() + padding.get_right() + border.get_left() + border.get_right());
    natural_width += MAX(2, padding.get_left() + padding.get_right() + border.get_left() + border.get_right());
}

void
Button::get_preferred_height_vfunc(int &minimal_height, int &natural_height) const
{
    auto child = get_child();

    if (child) {
        child->get_preferred_height(minimal_height, natural_height);
    } else {
        minimal_height = 0;
        natural_height = 0;
    }

    auto context = get_style_context();

    auto padding = context->get_padding(context->get_state());
    auto border = context->get_border(context->get_state());

    minimal_height += MAX(2, padding.get_top() + padding.get_bottom() + border.get_top() + border.get_bottom());
    natural_height += MAX(2, padding.get_top() + padding.get_bottom() + border.get_top() + border.get_bottom());
}

void
Button::on_clicked()
{
    if (_type == BUTTON_TYPE_TOGGLE) {
        Gtk::Button::on_clicked();
    }
}

bool
Button::process_event(GdkEvent *event)
{
    switch (event->type) {
    case GDK_2BUTTON_PRESS:
        if (_doubleclick_action) {
            sp_action_perform(_doubleclick_action, nullptr);
        }
        return true;
        break;
    default:
        break;
    }

    return false;
}

void
Button::perform_action()
{
    if (_action) {
        sp_action_perform(_action, nullptr);
    }
}

// Used by buttons to select tools.
Button::Button(GtkIconSize   size,
                   ButtonType  type,
                   SPAction     *action,
                   SPAction     *doubleclick_action)
    :
    _action(nullptr),
    _doubleclick_action(nullptr),
    _type(type),
    _lsize(CLAMP(size, GTK_ICON_SIZE_MENU, GTK_ICON_SIZE_DIALOG))
{
    set_border_width(0);

    set_can_focus(false);
    set_can_default(false);

    _on_clicked = signal_clicked().connect(sigc::mem_fun(*this, &Button::perform_action));

    signal_event().connect(sigc::mem_fun(*this, &Button::process_event));

    set_action(action);

    if (doubleclick_action) {
        set_doubleclick_action(doubleclick_action);
    }

    // The Inkscape style is no-relief buttons
    set_relief(Gtk::RELIEF_NONE);
}

void
Button::toggle_set_down(bool down)
{
    _on_clicked.block();
    set_active(down);
    _on_clicked.unblock();
}

void
Button::set_doubleclick_action(SPAction *action)
{
    if (_doubleclick_action) {
        g_object_unref(_doubleclick_action);
    }
    _doubleclick_action = action;
    if (action) {
        g_object_ref(action);
    }
}

void
Button::set_action(SPAction *action)
{
    Gtk::Widget *child;

    if (_action) {
        _c_set_active.disconnect();
        _c_set_sensitive.disconnect();
        child = get_child();
        if (child) {
            remove();
        }
        g_object_unref(_action);
    }

    _action = action;
    if (action) {
        g_object_ref(action);
        _c_set_active = action->signal_set_active.connect(
                sigc::mem_fun(*this, &Button::action_set_active));

        _c_set_sensitive = action->signal_set_sensitive.connect(
                sigc::mem_fun(*this, &Gtk::Widget::set_sensitive));

        if (action->image) {
            child = Glib::wrap(sp_get_icon_image(action->image, _lsize));
            child->show();
            add(*child);
        }
    }

    set_composed_tooltip(action);
}

void
Button::action_set_active(bool active)
{
    if (_type != BUTTON_TYPE_TOGGLE) {
        return;
    }

    /* temporarily lobotomized until SPActions are per-view */
    if (false && !active != !get_active()) {
        toggle_set_down(active);
    }
}

void
Button::set_composed_tooltip(SPAction *action)
{
    Glib::ustring tip;

    if (action) {
        if (action->tip) {
            tip = action->tip;
        }

        Gtk::AccelKey shortcut = Inkscape::Shortcuts::getInstance().get_shortcut_from_verb(action->verb);
        if (shortcut.get_key() != GDK_KEY_VoidSymbol) {
            // Action with shortcut.
            Glib::ustring key = Inkscape::Shortcuts::get_label(shortcut);
            if (!key.empty()) {
                tip += " (" + key + ")";
            }
        }
    }

    set_tooltip_text(tip.c_str());
}

// Used by object-lock, zoom-original, color-management.
Button::Button(GtkIconSize               size,
                   ButtonType              type,
                   Inkscape::UI::View::View *view,
                   const gchar              *name,
                   const gchar              *tip)
    :
        _action(nullptr),
        _doubleclick_action(nullptr),
        _type(type),
        _lsize(CLAMP(size, GTK_ICON_SIZE_MENU, GTK_ICON_SIZE_DIALOG))
{
    set_border_width(0);

    set_can_focus(false);
    set_can_default(false);

    _on_clicked = signal_clicked().connect(sigc::mem_fun(*this, &Button::perform_action));
    signal_event().connect(sigc::mem_fun(*this, &Button::process_event));

    auto action = sp_action_new(Inkscape::ActionContext(view), name, name, tip, name, nullptr);
    set_action(action);
    g_object_unref(action);
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
