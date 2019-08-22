// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEEN_PREVIEWABLE_H
#define SEEN_PREVIEWABLE_H
/*
 * A simple interface for previewing representations.
 *
 * Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2005 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <gtkmm/widget.h>

#include "widget/preview.h"

namespace Inkscape {
namespace UI {

enum PreviewStyle {
    PREVIEW_STYLE_ICON = 0,
    PREVIEW_STYLE_PREVIEW,
    PREVIEW_STYLE_NAME,
    PREVIEW_STYLE_BLURB,
    PREVIEW_STYLE_ICON_NAME,
    PREVIEW_STYLE_ICON_BLURB,
    PREVIEW_STYLE_PREVIEW_NAME,
    PREVIEW_STYLE_PREVIEW_BLURB
};


class Previewable
{
public:
// TODO need to add some nice parameters
    virtual ~Previewable() = default;
    virtual Gtk::Widget* getPreview(UI::Widget::PreviewStyle style,
                                    UI::Widget::ViewType     view,
                                    UI::Widget::PreviewSize  size,
                                    guint                    ratio,
                                    guint                    border) = 0;
};


} //namespace UI
} //namespace Inkscape


#endif // SEEN_PREVIEWABLE_H

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
