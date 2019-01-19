// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_MEASURE_TOOLBAR_H
#define SEEN_MEASURE_TOOLBAR_H

/**
 * @file
 * Measure aux toolbar
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

class SPDesktop;

typedef struct _GtkActionGroup GtkActionGroup;

namespace Inkscape {
namespace UI {
namespace Widget {
class UnitTracker;
}

namespace Toolbar {
class MeasureToolbar : public Toolbar {
private:
    UI::Widget::UnitTracker *_tracker;
    Glib::RefPtr<Gtk::Adjustment> _font_size_adj;
    Glib::RefPtr<Gtk::Adjustment> _precision_adj;
    Glib::RefPtr<Gtk::Adjustment> _scale_adj;
    Glib::RefPtr<Gtk::Adjustment> _offset_adj;

    void fontsize_value_changed();
    void unit_changed(int notUsed);
    void precision_value_changed();
    void scale_value_changed();
    void offset_value_changed();

protected:
    MeasureToolbar(SPDesktop *desktop);

public:
    static GtkWidget * prep(SPDesktop *desktop, GtkActionGroup* mainActions);
};

}
}
}

#endif /* !SEEN_MEASURE_TOOLBAR_H */
