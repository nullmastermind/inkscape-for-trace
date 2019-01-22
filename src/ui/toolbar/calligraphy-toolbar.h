// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CALLIGRAPHY_TOOLBAR_H
#define SEEN_CALLIGRAPHY_TOOLBAR_H

/**
 * @file
 * Calligraphy aux toolbar
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

class CalligraphyToolbar : public Toolbar {
private:
    bool _presets_blocked;

    EgeAdjustmentAction *_angle_action;
    InkSelectOneAction *_profile_selector;

    std::map<Glib::ustring, GObject *> _widget_map;

    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _mass_adj;
    Glib::RefPtr<Gtk::Adjustment> _wiggle_adj;
    Glib::RefPtr<Gtk::Adjustment> _angle_adj;
    Glib::RefPtr<Gtk::Adjustment> _thinning_adj;
    Glib::RefPtr<Gtk::Adjustment> _tremor_adj;
    Glib::RefPtr<Gtk::Adjustment> _fixation_adj;
    Glib::RefPtr<Gtk::Adjustment> _cap_rounding_adj;
    InkToggleAction *_usepressure;
    InkToggleAction *_tracebackground;
    InkToggleAction *_usetilt;

    std::unique_ptr<PrefPusher> _tracebackground_pusher;
    std::unique_ptr<PrefPusher> _usepressure_pusher;
    std::unique_ptr<PrefPusher> _usetilt_pusher;

    void width_value_changed();
    void velthin_value_changed();
    void angle_value_changed();
    void flatness_value_changed();
    void cap_rounding_value_changed();
    void tremor_value_changed();
    void wiggle_value_changed();
    void mass_value_changed();
    void build_presets_list();
    void change_profile(int mode);
    void save_profile(GtkWidget *widget);

    static void edit_profile(GtkAction *act, gpointer data);
    static void update_presets_list(gpointer data);
    static void tilt_state_changed( GtkToggleAction *act, gpointer data );
    
protected:
    CalligraphyToolbar(SPDesktop *desktop)
        : Toolbar(desktop)
    {}

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}

#endif /* !SEEN_CALLIGRAPHY_TOOLBAR_H */
