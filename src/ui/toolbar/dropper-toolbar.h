// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_DROPPER_TOOLBAR_H
#define SEEN_DROPPER_TOOLBAR_H

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

#include "toolbar.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

/**
 * \brief A toolbar for controlling the dropper tool
 */
class DropperToolbar : public Toolbar {
private:
    // Tool widgets
    Gtk::ToggleToolButton *_pick_alpha_button; ///< Control whether to pick opacity
    Gtk::ToggleToolButton *_set_alpha_button;  ///< Control whether to set opacity

    // Event handlers
    void on_pick_alpha_button_toggled();
    void on_set_alpha_button_toggled();

protected:
    DropperToolbar(SPDesktop *desktop);

public:
    static GtkWidget * create(SPDesktop *desktop);
};
}
}
}
#endif /* !SEEN_DROPPER_TOOLBAR_H */

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
