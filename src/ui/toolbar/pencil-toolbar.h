// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_PENCIL_TOOLBAR_H
#define SEEN_PENCIL_TOOLBAR_H

/**
 * @file
 * Pencil aux toolbar
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
typedef struct _InkAction InkAction;
typedef struct _InkToggleAction InkToggleAction;

namespace Inkscape {
namespace XML {
class Node;
}

namespace UI {
namespace Toolbar {
class PencilToolbar : public Toolbar {
private:
    EgeAdjustmentAction *_minpressure;
    EgeAdjustmentAction *_maxpressure;

    XML::Node *_repr;
    InkAction *_flatten_spiro_bspline;
    InkAction *_flatten_simplify;

    InkSelectOneAction *_shape_action;

    InkToggleAction *_simplify;

    bool _freeze;

    Glib::RefPtr<Gtk::Adjustment> _minpressure_adj;
    Glib::RefPtr<Gtk::Adjustment> _maxpressure_adj;
    Glib::RefPtr<Gtk::Adjustment> _tolerance_adj;

    void add_freehand_mode_toggle(GtkActionGroup* mainActions,
                                  bool tool_is_pencil);
    void freehand_mode_changed(int mode);
    Glib::ustring const freehand_tool_name();
    void minpressure_value_changed();
    void maxpressure_value_changed();
    static void use_pencil_pressure(InkToggleAction *itact,
                                    gpointer         data);
    void tolerance_value_changed();
    void freehand_add_advanced_shape_options(GtkActionGroup* mainActions, bool tool_is_pencil);
    void freehand_change_shape(int shape);
    static void defaults(GtkWidget *widget, GObject *obj);
    static void freehand_simplify_lpe(InkToggleAction* itact, GObject *data);
    static void simplify_flatten(GtkWidget *widget, GObject *data);
    static void flatten_spiro_bspline(GtkWidget *widget, gpointer data);

protected:
    PencilToolbar(SPDesktop *desktop)
        : Toolbar(desktop),
          _repr(nullptr),
          _freeze(false),
          _flatten_simplify(nullptr),
          _simplify(nullptr)
    {}

    ~PencilToolbar() override;

public:
    static GtkWidget * prep_pencil(SPDesktop *desktop, GtkActionGroup* mainActions);
    static GtkWidget * prep_pen(SPDesktop *desktop, GtkActionGroup* mainActions);
};
}
}
}

#endif /* !SEEN_PENCIL_TOOLBAR_H */
