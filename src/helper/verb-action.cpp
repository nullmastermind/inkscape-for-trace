// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file
 * Deprecated Gtk::Action that provides a widget for an Inkscape verb
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
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2015 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "verb-action.h"

#include <vector>

#include <glibmm/i18n.h>
#include <gtkmm/toolitem.h>

#include "verbs.h"
#include "helper/action.h"
#include "ui/widget/button.h"
#include "widgets/toolbox.h"

static GtkToolItem * sp_toolbox_button_item_new_from_verb_with_doubleclick( GtkWidget *t, GtkIconSize size, Inkscape::UI::Widget::ButtonType type,
                                                                     Inkscape::Verb *verb, Inkscape::Verb *doubleclick_verb,
                                                                     Inkscape::UI::View::View *view);

GtkToolItem * sp_toolbox_button_item_new_from_verb_with_doubleclick(GtkWidget *t, GtkIconSize size, Inkscape::UI::Widget::ButtonType type,
                                                             Inkscape::Verb *verb, Inkscape::Verb *doubleclick_verb,
                                                             Inkscape::UI::View::View *view)
{
    SPAction *action = verb->get_action(Inkscape::ActionContext(view));
    if (!action) {
        return nullptr;
    }

    SPAction *doubleclick_action;
    if (doubleclick_verb) {
        doubleclick_action = doubleclick_verb->get_action(Inkscape::ActionContext(view));
    } else {
        doubleclick_action = nullptr;
    }

    /* fixme: Handle sensitive/unsensitive */
    /* fixme: Implement Inkscape::UI::Widget::Button construction from action */
    auto b = Gtk::manage(new Inkscape::UI::Widget::Button(size, type, action, doubleclick_action));
    b->show();
    auto b_toolitem = Gtk::manage(new Gtk::ToolItem());
    b_toolitem->add(*b);

    return GTK_TOOL_ITEM(b_toolitem->gobj());
}

Glib::RefPtr<VerbAction> VerbAction::create(Inkscape::Verb* verb, Inkscape::Verb* verb2, Inkscape::UI::View::View *view)
{
    Glib::RefPtr<VerbAction> result;
    SPAction *action = verb->get_action(Inkscape::ActionContext(view));
    if ( action ) {
        //SPAction* action2 = verb2 ? verb2->get_action(Inkscape::ActionContext(view)) : 0;
        result = Glib::RefPtr<VerbAction>(new VerbAction(verb, verb2, view));
    }

    return result;
}

VerbAction::VerbAction(Inkscape::Verb* verb, Inkscape::Verb* verb2, Inkscape::UI::View::View *view) :
    Gtk::Action(Glib::ustring(verb->get_id()), verb->get_image(), Glib::ustring(g_dpgettext2(nullptr, "ContextVerb", verb->get_name())), Glib::ustring(_(verb->get_tip()))),
    verb(verb),
    verb2(verb2),
    view(view),
    active(false)
{
}

VerbAction::~VerbAction()
= default;

Gtk::Widget* VerbAction::create_menu_item_vfunc()
{
    Gtk::Widget* widg = Gtk::Action::create_menu_item_vfunc();
//     g_message("create_menu_item_vfunc() = %p  for '%s'", widg, verb->get_id());
    return widg;
}

Gtk::Widget* VerbAction::create_tool_item_vfunc()
{
//     Gtk::Widget* widg = Gtk::Action::create_tool_item_vfunc();
    GtkIconSize toolboxSize = Inkscape::UI::ToolboxFactory::prefToSize("/toolbox/tools/small");
    GtkWidget* toolbox = nullptr;
    auto holder = Glib::wrap(sp_toolbox_button_item_new_from_verb_with_doubleclick( toolbox, toolboxSize,
                                                                                    Inkscape::UI::Widget::BUTTON_TYPE_TOGGLE,
                                                                                    verb,
                                                                                    verb2,
                                                                                    view ));

    auto button_widget = static_cast<Inkscape::UI::Widget::Button *>(holder->get_child());

    if ( active ) {
        button_widget->toggle_set_down(active);
    }
    button_widget->show_all();

//     g_message("create_tool_item_vfunc() = %p  for '%s'", holder, verb->get_id());
    return holder;
}

void VerbAction::connect_proxy_vfunc(Gtk::Widget* proxy)
{
//     g_message("connect_proxy_vfunc(%p)  for '%s'", proxy, verb->get_id());
    Gtk::Action::connect_proxy_vfunc(proxy);
}

void VerbAction::disconnect_proxy_vfunc(Gtk::Widget* proxy)
{
//     g_message("disconnect_proxy_vfunc(%p)  for '%s'", proxy, verb->get_id());
    Gtk::Action::disconnect_proxy_vfunc(proxy);
}

void VerbAction::set_active(bool active)
{
    this->active = active;
    std::vector<Gtk::Widget*> proxies = get_proxies();
    for (auto proxie : proxies) {
        Gtk::ToolItem* ti = dynamic_cast<Gtk::ToolItem*>(proxie);
        if (ti) {
            // *should* have one child that is the Inkscape::UI::Widget::Button
            auto child = dynamic_cast<Inkscape::UI::Widget::Button *>(ti->get_child());
            if (child) {
                child->toggle_set_down(active);
            }
        }
    }
}

void VerbAction::on_activate()
{
    if ( verb ) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(view));
        if ( action ) {
            sp_action_perform(action, nullptr);
        }
    }
}


