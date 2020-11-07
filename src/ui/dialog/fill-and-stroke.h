// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Fill and Stroke dialog
 */
/* Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Gustav Broberg <broberg@kth.se>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2004--2007 Authors
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_FILL_AND_STROKE_H
#define INKSCAPE_UI_DIALOG_FILL_AND_STROKE_H

#include <gtkmm/notebook.h>

#include "ui/dialog/dialog-base.h"
#include "ui/widget/object-composite-settings.h"
#include "ui/widget/style-subject.h"

namespace Inkscape {
namespace UI {

namespace Widget {
class FillNStroke;
class NotebookPage;
class StrokeStyle;
}

namespace Dialog {

class FillAndStroke : public DialogBase
{
public:
    FillAndStroke();
    ~FillAndStroke() override;

    static FillAndStroke &getInstance() { return *new FillAndStroke(); }

    void update() override;

    void showPageFill();
    void showPageStrokePaint();
    void showPageStrokeStyle();

protected:
    Gtk::Notebook   _notebook;

    UI::Widget::NotebookPage    *_page_fill;
    UI::Widget::NotebookPage    *_page_stroke_paint;
    UI::Widget::NotebookPage    *_page_stroke_style;

    UI::Widget::StyleSubject::Selection _subject;
    UI::Widget::ObjectCompositeSettings _composite_settings;

    Gtk::Box &_createPageTabLabel(const Glib::ustring &label,
                                  const char *label_image);

    void _layoutPageFill();
    void _layoutPageStrokePaint();
    void _layoutPageStrokeStyle();
    void _savePagePref(guint page_num);
    void _onSwitchPage(Gtk::Widget *page, guint pagenum);

private:
    FillAndStroke(FillAndStroke const &d) = delete;
    FillAndStroke& operator=(FillAndStroke const &d) = delete;

    SPDesktop *targetDesktop;
    UI::Widget::FillNStroke *fillWdgt;
    UI::Widget::FillNStroke *strokeWdgt;
    UI::Widget::StrokeStyle *strokeStyleWdgt;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape


#endif // INKSCAPE_UI_DIALOG_FILL_AND_STROKE_H

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
