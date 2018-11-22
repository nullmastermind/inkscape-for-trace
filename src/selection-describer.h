// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::SelectionDescriber - shows messages describing selection
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_SELECTION_DESCRIPTION_HANDLER_H
#define SEEN_INKSCAPE_SELECTION_DESCRIPTION_HANDLER_H

#include <cstddef>
#include <memory>
#include <sigc++/sigc++.h>
#include "message-context.h"

namespace Inkscape {

class MessageStack;
class Selection;

class SelectionDescriber : public sigc::trackable {
public:
    SelectionDescriber(Inkscape::Selection *selection, std::shared_ptr<MessageStack> stack, char *when_selected, char *when_nothing);
    ~SelectionDescriber();

private:
    void _updateMessageFromSelection(Inkscape::Selection *selection);
    sigc::connection *_selection_changed_connection;

    MessageContext _context;

    char *_when_selected;
    char *_when_nothing;
};

}

#endif

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
