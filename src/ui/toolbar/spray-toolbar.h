// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SPRAY_TOOLBAR_H
#define SEEN_SPRAY_TOOLBAR_H

/**
 * @file
 * Spray aux toolbar
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
 * Copyright (C) 1999-2015 authors
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
class SprayToolbar : public Toolbar {
private:
    Glib::RefPtr<Gtk::Adjustment> _width_adj;
    Glib::RefPtr<Gtk::Adjustment> _mean_adj;
    Glib::RefPtr<Gtk::Adjustment> _sd_adj;
    Glib::RefPtr<Gtk::Adjustment> _population_adj;
    Glib::RefPtr<Gtk::Adjustment> _rotation_adj;
    Glib::RefPtr<Gtk::Adjustment> _offset_adj;
    Glib::RefPtr<Gtk::Adjustment> _scale_adj;

    std::unique_ptr<PrefPusher> _usepressurewidth_pusher;
    std::unique_ptr<PrefPusher> _usepressurepopulation_pusher;

    InkSelectOneAction *_spray_tool_mode;
    EgeAdjustmentAction *_spray_population;
    EgeAdjustmentAction *_spray_rotation;
    EgeAdjustmentAction *_spray_scale;
    InkToggleAction * _usepressurescale;
    InkToggleAction *_picker;
    InkToggleAction *_pick_center;
    InkToggleAction *_pick_inverse_value;
    InkToggleAction *_pick_fill;
    InkToggleAction *_pick_stroke;
    InkToggleAction *_pick_no_overlap;
    InkToggleAction *_over_transparent;
    InkToggleAction *_over_no_transparent;
    InkToggleAction *_no_overlap;
    EgeAdjustmentAction *_offset;

    void width_value_changed();
    void mean_value_changed();
    void standard_deviation_value_changed();
    void mode_changed(int mode);
    void init();
    void population_value_changed();
    void rotation_value_changed();
    void update_widgets();
    void scale_value_changed();
    void offset_value_changed();
    static void toggle_no_overlap         (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_pressure_scale     (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_over_no_transparent(GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_over_transparent   (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_picker             (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_pick_center        (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_pick_fill          (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_pick_stroke        (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_pick_no_overlap    (GtkToggleAction *toggleaction,
                                           gpointer         user_data);
    static void toggle_pick_inverse_value (GtkToggleAction *toggleaction,
                                           gpointer         user_data);

protected:
    SprayToolbar(SPDesktop *desktop) :
        Toolbar(desktop)
    {}

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};
}
}
}

#endif /* !SEEN_SELECT_TOOLBAR_H */
