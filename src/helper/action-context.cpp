// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ActionContext implementation.
 *
 * Author:
 *   Eric Greveson <eric@greveson.co.uk>
 *
 * Copyright (C) 2013 Eric Greveson
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "desktop.h"
#include "document.h"
#include "layer-model.h"
#include "selection.h"
#include "helper/action-context.h"

namespace Inkscape {

ActionContext::ActionContext()
  : _selection(nullptr)
  , _view(nullptr)
{
}

ActionContext::ActionContext(Selection *selection)
  : _selection(selection)
  , _view(nullptr)
{
}

ActionContext::ActionContext(UI::View::View *view)
  : _selection(nullptr)
  , _view(view)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(view);
    if (desktop) {
        _selection = desktop->selection;
    }
}

SPDocument *ActionContext::getDocument() const
{
    if (_selection == nullptr) {
        return nullptr;
    }

    // Should be the same as the view's document, if view is non-NULL
    return _selection->layers()->getDocument();
}

Selection *ActionContext::getSelection() const
{
    return _selection;
}

UI::View::View *ActionContext::getView() const
{
    return _view;
}

SPDesktop *ActionContext::getDesktop() const
{
    return static_cast<SPDesktop *>(_view);
}

} // namespace Inkscape

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
