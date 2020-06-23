// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief  Fill style configuration
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2010 Jon A. Cruz
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_DIALOGS_SP_FILL_STYLE_H
#define SEEN_DIALOGS_SP_FILL_STYLE_H

#include "ui/widget/paint-selector.h"

#include <gtkmm/box.h>

namespace Gtk {
class Widget;
}

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Tools {
class ToolBase;
}

namespace Widget {

class FillNStroke : public Gtk::Box {
  private:
    FillOrStroke kind;
    SPDesktop     *_desktop   = nullptr;
    PaintSelector *_psel      = nullptr;
    guint32        _last_drag = 0;
    guint          _drag_id   = 0;
    bool           _update    = false;

    sigc::connection selectChangedConn;
    sigc::connection subselChangedConn;
    sigc::connection selectModifiedConn;
    sigc::connection eventContextConn;

    void paintModeChangeCB(UI::Widget::PaintSelector::Mode mode);
    void paintChangedCB();
    static gboolean dragDelayCB(gpointer data);

    void selectionModifiedCB(guint flags);
    void eventContextCB(SPDesktop *desktop, Inkscape::UI::Tools::ToolBase *eventcontext);

    void dragFromPaint();
    void updateFromPaint();

    void performUpdate();

  public:
    FillNStroke(FillOrStroke k);
    ~FillNStroke() override;

    void setFillrule(PaintSelector::FillRule mode);
    void setDesktop(SPDesktop *desktop);
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // SEEN_DIALOGS_SP_FILL_STYLE_H

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
