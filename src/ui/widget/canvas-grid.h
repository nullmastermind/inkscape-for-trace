// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_CANVASGRID_H
#define INKSCAPE_UI_WIDGET_CANVASGRID_H
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm.h>
#include <gtkmm/label.h>
#include <gtkmm/overlay.h>
#include "ui/dialog/command-palette.h"

class SPCanvas;
class SPDesktopWidget;

namespace Inkscape {
namespace UI {
namespace Widget {

class Canvas;
class Ruler;

/**
 * A Gtk::Grid widget that contains rulers, scrollbars, buttons, and, of course, the canvas.
 * Canvas has an overlay to let us put stuff on the canvas.
 */ 
class CanvasGrid : public Gtk::Grid
{
public:

    CanvasGrid(SPDesktopWidget *dtw);

    void ShowScrollbars(bool state = true);
    void ToggleScrollbars();

    void ShowRulers(bool state = true);
    void ToggleRulers();
    void UpdateRulers();

    void ShowCommandPalette(bool state = true);
    void ToggleCommandPalette();

    Inkscape::UI::Widget::Canvas *GetCanvas() { return _canvas; };

    // Hopefully temp.
    Inkscape::UI::Widget::Ruler *GetHRuler() { return _vruler; };
    Inkscape::UI::Widget::Ruler *GetVRuler() { return _hruler; };
    Glib::RefPtr<Gtk::Adjustment> GetHAdj() { return _hadj; };
    Glib::RefPtr<Gtk::Adjustment> GetVAdj() { return _vadj; };
    Gtk::ToggleButton *GetGuideLock()  { return _guide_lock; }
    Gtk::ToggleButton *GetCmsAdjust()  { return _cms_adjust; }
    Gtk::ToggleButton *GetStickyZoom() { return _sticky_zoom; };

private:

    // Signal callbacks
    void OnSizeAllocate(Gtk::Allocation& allocation);
    bool SignalEvent(GdkEvent *event);

    // The Widgets
    Inkscape::UI::Widget::Canvas *_canvas;

    Gtk::Overlay      _canvas_overlay;

    Dialog::CommandPalette _command_palette;

    Glib::RefPtr<Gtk::Adjustment> _hadj;
    Glib::RefPtr<Gtk::Adjustment> _vadj;
    Gtk::Scrollbar    *_hscrollbar;
    Gtk::Scrollbar    *_vscrollbar;

    Inkscape::UI::Widget::Ruler *_hruler;
    Inkscape::UI::Widget::Ruler *_vruler;

    Gtk::ToggleButton *_guide_lock;
    Gtk::ToggleButton *_cms_adjust;
    Gtk::ToggleButton *_sticky_zoom;

    // To be replaced by stateful Gio::Actions
    bool _show_scrollbars = true;
    bool _show_rulers = true;

    // Hopefully temp
    SPDesktopWidget *_dtw;

    // Store allocation so we don't redraw too often.
    Gtk::Allocation _allocation;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape


#endif // INKSCAPE_UI_WIDGET_CANVASGRID_H

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
