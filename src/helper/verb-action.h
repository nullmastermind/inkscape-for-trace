// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file
 * A deprecated Gtk::Action that provides a widget for an Inkscape
 * Verb.
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

#ifndef SEEN_VERB_ACTION_H
#define SEEN_VERB_ACTION_H

#include <gtkmm/action.h>

namespace Inkscape {
class Verb;

namespace UI {
namespace View {
class View;
}
}
}

/**
 * \brief A deprecated Gtk::Action that provides a widget for an Inkscape Verb
 *
 * \deprecated In new code, you should create a Gtk::ToolItem instead of using this
 */
class VerbAction : public Gtk::Action {
public:
    static Glib::RefPtr<VerbAction> create(Inkscape::Verb* verb, Inkscape::Verb* verb2, Inkscape::UI::View::View *view);

    ~VerbAction() override;
    virtual void set_active(bool active = true);

protected:
    Gtk::Widget* create_menu_item_vfunc() override;
    Gtk::Widget* create_tool_item_vfunc() override;

    void connect_proxy_vfunc(Gtk::Widget* proxy) override;
    void disconnect_proxy_vfunc(Gtk::Widget* proxy) override;

    void on_activate() override;

private:
    Inkscape::Verb* verb;
    Inkscape::Verb* verb2;
    Inkscape::UI::View::View *view;
    bool active;

    VerbAction(Inkscape::Verb* verb, Inkscape::Verb* verb2, Inkscape::UI::View::View *view);
};

#endif

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
