// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!

#include "actions-transform.h"
#include "actions-helper.h"
#include "inkscape-application.h"

#include "inkscape.h"             // Inkscape::Application
#include "selection.h"            // Selection

void
transform_rotate(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    auto selection = app->get_active_selection();
    selection->rotate(d.get());
}

void
add_actions_transform(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)

    app->add_action_with_parameter( "transform-rotate",         Double, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&transform_rotate),          app));

#endif
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
