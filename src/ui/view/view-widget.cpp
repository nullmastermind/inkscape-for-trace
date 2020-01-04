// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "view.h"
#include "view-widget.h"

/**
 * Callback to disconnect from view and destroy SPViewWidget.
 *
 * Apparently, this gets only called when a desktop is closed, but then twice!
 */
void SPViewWidget::on_unrealize()
{
    SPViewWidget *vw = this;

    if (vw->view) {
        vw->view->close();
        Inkscape::GC::release(vw->view);
        vw->view = nullptr;
    }

    parent_type::on_unrealize();

    Inkscape::GC::request_early_collection();
}

void SPViewWidget::setView(view_type *view)
{
    auto vw = this;
    g_return_if_fail(view != nullptr);
    
    g_return_if_fail(vw->view == nullptr);
    
    vw->view = view;
    Inkscape::GC::anchor(view);
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
