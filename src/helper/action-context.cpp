/*
 * ActionContext implementation.
 *
 * Author:
 *   Eric Greveson <eric@greveson.co.uk>
 *
 * Copyright (C) 2013 Eric Greveson
 *
 * This code is in public domain
 */

#include "desktop.h"
#include "document.h"
#include "layer-model.h"
#include "selection.h"
#include "helper/action-context.h"
#include "ui/view/view.h"

namespace Inkscape {

ActionContext::ActionContext()
  : _selection(NULL)
  , _view(NULL)
{
}

ActionContext::ActionContext(Selection *selection)
  : _selection(selection)
  , _view(NULL)
{
}

ActionContext::ActionContext(UI::View::View *view)
  : _selection(NULL)
  , _view(view)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(view);
    if (desktop) {
        _selection = desktop->selection;
    }
}

SPDocument *ActionContext::getDocument() const
{
    if (_selection == NULL) {
        return NULL;
    }

    // Should be the same as the view's document, if view is non-NULL
    return _selection->layerModel()->getDocument();
}

Selection *ActionContext::getSelection() const
{
    return _selection;
}

UI::View::View *ActionContext::getView() const
{
    return _view;
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
