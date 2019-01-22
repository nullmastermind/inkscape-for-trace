// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_ERASOR_TOOLBAR_H
#define SEEN_ERASOR_TOOLBAR_H

/**
 * @file
 * Erasor aux toolbar
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

#include <gtkmm/adjustment.h>

class InkSelectOneAction;
class SPDesktop;

typedef struct _EgeAdjustmentAction EgeAdjustmentAction;
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _InkToggleAction InkToggleAction;

namespace Inkscape {
namespace UI {
class PrefPusher;

namespace Toolbar {
class EraserToolbar : public Toolbar {
private:
    InkSelectOneAction  *_eraser_mode_action;
    EgeAdjustmentAction *_width;
    EgeAdjustmentAction *_mass;
    EgeAdjustmentAction *_thinning;
    EgeAdjustmentAction *_cap_rounding;
    EgeAdjustmentAction *_tremor;

    InkToggleAction *_usepressure;
    InkToggleAction *_split;

    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _mass_adj;
    Glib::RefPtr<Gtk::Adjustment> _thinning_adj;
    Glib::RefPtr<Gtk::Adjustment> _cap_rounding_adj;
    Glib::RefPtr<Gtk::Adjustment> _tremor_adj;

    std::unique_ptr<PrefPusher> _pressure_pusher;

    bool _freeze;

    void mode_changed(int mode);
    void set_eraser_mode_visibility(const guint eraser_mode);
    void width_value_changed();
    void mass_value_changed();
    void velthin_value_changed();
    void cap_rounding_value_changed();
    void tremor_value_changed();
    static void update_presets_list(gpointer data);
    static void toggle_break_apart(GtkToggleAction *act,
                                   gpointer         data);

protected:
    EraserToolbar(SPDesktop *desktop);

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}

#endif /* !SEEN_ERASOR_TOOLBAR_H */
