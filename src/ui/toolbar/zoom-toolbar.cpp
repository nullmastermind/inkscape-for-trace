// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Zoom aux toolbar: Temp until we convert all toolbars to ui files with Gio::Actions.
 */
/* Authors:
 *   Tavmjong Bah <tavmjong@free.fr>

 * Copyright (C) 2019 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>

#include "zoom-toolbar.h"

#include "desktop.h"
#include "io/resource.h"

using Inkscape::IO::Resource::UIS;

namespace Inkscape {
namespace UI {
namespace Toolbar {

GtkWidget *
ZoomToolbar::create(SPDesktop *desktop)
{
    Glib::ustring zoom_toolbar_builder_file = get_filename(UIS, "toolbar-zoom.ui");
    auto builder = Gtk::Builder::create();
    try
    {
        builder->add_from_file(zoom_toolbar_builder_file);
    }
    catch (const Glib::Error& ex)
    {
        std::cerr << "ZoomToolbar: " << zoom_toolbar_builder_file << " file not read! " << ex.what() << std::endl;
    }

    Gtk::Toolbar* toolbar = nullptr;
    builder->get_widget("zoom-toolbar", toolbar);
    if (!toolbar) {
        std::cerr << "InkscapeWindow: Failed to load zoom toolbar!" << std::endl;
        return nullptr;
    }

    toolbar->reference(); // Or it will be deleted when builder is destroyed since we haven't added
                          // it to a container yet. This probably causes a memory leak but we'll
                          // fix it when all toolbars are converted to use Gio::Actions.

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
