// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Dropper aux toolbar
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
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "dropper-toolbar.h"
#include "document-undo.h"
#include "preferences.h"
#include "widgets/spinbutton-events.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

void DropperToolbar::on_pick_alpha_button_toggled()
{
    auto active = _pick_alpha_button->get_active();

    auto prefs = Inkscape::Preferences::get();
    prefs->setInt( "/tools/dropper/pick", active );

    _set_alpha_button->set_sensitive(active);

    spinbutton_defocus(GTK_WIDGET(gobj()));
}

void DropperToolbar::on_set_alpha_button_toggled()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool( "/tools/dropper/setalpha", _set_alpha_button->get_active( ) );
    spinbutton_defocus(GTK_WIDGET(gobj()));
}

/*
 * TODO: Would like to add swatch of current color.
 * TODO: Add queue of last 5 or so colors selected with new swatches so that
 *       can drag and drop places. Will provide a nice mixing palette.
 */
DropperToolbar::DropperToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
{
    // Add widgets to toolbar
    add_label(_("Opacity:"));
    _pick_alpha_button = add_toggle_button(_("Pick"),
                                           _("Pick both the color and the alpha (transparency) under cursor; "
                                             "otherwise, pick only the visible color premultiplied by alpha"));
    _set_alpha_button = add_toggle_button(_("Assign"),
                                          _("If alpha was picked, assign it to selection "
                                            "as fill or stroke transparency"));

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Set initial state of widgets
    auto pickAlpha = prefs->getInt( "/tools/dropper/pick", 1 );
    auto setAlpha  = prefs->getBool( "/tools/dropper/setalpha", true);

    _pick_alpha_button->set_active(pickAlpha);
    _set_alpha_button->set_active(setAlpha);

    // Make sure the set-alpha button is disabled if we're not picking alpha
    _set_alpha_button->set_sensitive(pickAlpha);

    // Connect signal handlers
    auto pick_alpha_button_toggled_cb = sigc::mem_fun(*this, &DropperToolbar::on_pick_alpha_button_toggled);
    auto set_alpha_button_toggled_cb  = sigc::mem_fun(*this, &DropperToolbar::on_set_alpha_button_toggled);

    _pick_alpha_button->signal_toggled().connect(pick_alpha_button_toggled_cb);
    _set_alpha_button->signal_toggled().connect(set_alpha_button_toggled_cb);

    show_all();
}

GtkWidget *
DropperToolbar::create(SPDesktop *desktop)
{
    auto toolbar = Gtk::manage(new DropperToolbar(desktop));
    return GTK_WIDGET(toolbar->gobj());
}
}
}
}

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
