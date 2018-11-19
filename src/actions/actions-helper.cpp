// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-helper.h"

#include "inkscape-application.h"
#include "inkscape.h"
#include "selection.h"

// Helper function: returns true if both document and selection found. Maybe this should
// work on current view. Or better, application could return the selection of the current view.
bool
get_document_and_selection(InkscapeApplication* app, SPDocument** document, Inkscape::Selection** selection)
{
    *document = app->get_active_document();
    if (!(*document)) {
        std::cerr << "get_document_and_selection: No document!" << std::endl;
        return false;
    }

    // To do: get selection from active view (which could be from desktop or not).
    Inkscape::ActionContext context = INKSCAPE.action_context_for_document(*document);
    *selection = context.getSelection();
    if (!*selection) {
        std::cerr << "get_document_and_selection: No selection!" << std::endl;
        return false;
    }

    return true;
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
