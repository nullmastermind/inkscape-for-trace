// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Zoom aux toolbar
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

#include "zoom-toolbar.h"

#include "desktop.h"
#include "verbs.h"

#include "helper/action.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {
ZoomToolbar::ZoomToolbar(SPDesktop *desktop)
    : Toolbar(desktop)
{
    add_toolbutton_for_verb(SP_VERB_ZOOM_IN);
    add_toolbutton_for_verb(SP_VERB_ZOOM_OUT);

    add_separator();

    add_toolbutton_for_verb(SP_VERB_ZOOM_1_1);
    add_toolbutton_for_verb(SP_VERB_ZOOM_1_2);
    add_toolbutton_for_verb(SP_VERB_ZOOM_2_1);

    add_separator();

    add_toolbutton_for_verb(SP_VERB_ZOOM_SELECTION);
    add_toolbutton_for_verb(SP_VERB_ZOOM_DRAWING);
    add_toolbutton_for_verb(SP_VERB_ZOOM_PAGE);
    add_toolbutton_for_verb(SP_VERB_ZOOM_PAGE_WIDTH);
    add_toolbutton_for_verb(SP_VERB_ZOOM_CENTER_PAGE);

    add_separator();

    add_toolbutton_for_verb(SP_VERB_ZOOM_PREV);
    add_toolbutton_for_verb(SP_VERB_ZOOM_NEXT);

    show_all();
}

GtkWidget *
ZoomToolbar::create(SPDesktop *desktop)
{
    auto toolbar = Gtk::manage(new ZoomToolbar(desktop));
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
