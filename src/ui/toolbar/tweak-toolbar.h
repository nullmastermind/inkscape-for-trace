// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_TWEAK_TOOLBAR_H
#define SEEN_TWEAK_TOOLBAR_H

/**
 * @file
 * Tweak aux toolbar
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

class InkSelectOneAction;
class SPDesktop;

typedef struct _EgeAdjustmentAction EgeAdjustmentAction;
typedef struct _EgeOutputAction EgeOutputAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkToggleAction InkToggleAction;

namespace Inkscape {
namespace UI {
namespace Toolbar {
class TweakToolbar : public Toolbar {
private:
    Glib::RefPtr<Gtk::Adjustment> _adj_tweak_width;
    Glib::RefPtr<Gtk::Adjustment> _adj_tweak_force;
    Glib::RefPtr<Gtk::Adjustment> _adj_tweak_fidelity;

    InkSelectOneAction *_tweak_tool_mode;

    EgeOutputAction *_tweak_channels_label;
    InkToggleAction *_tweak_doh;
    InkToggleAction *_tweak_dos;
    InkToggleAction *_tweak_dol;
    InkToggleAction *_tweak_doo;

    EgeAdjustmentAction *_tweak_fidelity;

    void tweak_width_value_changed();
    void tweak_force_value_changed();
    void tweak_mode_changed(int mode);
    void tweak_fidelity_value_changed();

protected:
    TweakToolbar(SPDesktop *_desktop)
        : Toolbar(_desktop)
    {}

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};
}
}
}

#endif /* !SEEN_SELECT_TOOLBAR_H */
