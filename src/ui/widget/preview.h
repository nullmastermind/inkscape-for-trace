// SPDX-License-Identifier: GPL-2.0-or-later OR MPL-1.1 OR LGPL-2.1-or-later
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Eek Preview Stuffs.
 *
 * The Initial Developer of the Original Code is
 * Jon A. Cruz.
 * Portions created by the Initial Developer are Copyright (C) 2005-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SEEN_EEK_PREVIEW_H
#define SEEN_EEK_PREVIEW_H

#include <gtkmm/drawingarea.h>

/**
 * @file
 * Generic implementation of an object that can be shown by a preview.
 */

namespace Inkscape {
namespace UI {
namespace Widget {

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

enum ViewType {
    VIEW_TYPE_LIST = 0,
    VIEW_TYPE_GRID
};

enum PreviewSize {
    PREVIEW_SIZE_TINY = 0,
    PREVIEW_SIZE_SMALL,
    PREVIEW_SIZE_MEDIUM,
    PREVIEW_SIZE_BIG,
    PREVIEW_SIZE_BIGGER,
    PREVIEW_SIZE_HUGE
};

enum LinkType {
  PREVIEW_LINK_NONE = 0,
  PREVIEW_LINK_IN = 1,
  PREVIEW_LINK_OUT = 2,
  PREVIEW_LINK_OTHER = 4,
  PREVIEW_FILL = 8,
  PREVIEW_STROKE = 16,
  PREVIEW_LINK_ALL = 31
};

enum BorderStyle {
    BORDER_NONE = 0,
    BORDER_SOLID,
    BORDER_WIDE,
    BORDER_SOLID_LAST_ROW,
};

class Preview : public Gtk::DrawingArea {
private:
    int           _scaledW;
    int           _scaledH;

    int           _r;
    int           _g;
    int           _b;

    bool          _hot;
    bool          _within;
    bool          _takesFocus; ///< flag to grab focus when clicked
    ViewType      _view;
    PreviewSize   _size;
    unsigned int  _ratio;
    LinkType      _linked;
    unsigned int  _border;

    Glib::RefPtr<Gdk::Pixbuf> _previewPixbuf;
    Glib::RefPtr<Gdk::Pixbuf> _scaled;

    // signals
    sigc::signal<void> _signal_clicked;
    sigc::signal<void, int> _signal_alt_clicked;

    void size_request(GtkRequisition *req) const;

protected:
    void get_preferred_width_vfunc(int &minimal_width, int &natural_width) const override;
    void get_preferred_height_vfunc(int &minimal_height, int &natural_height) const override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;
    bool on_button_press_event(GdkEventButton *button_event) override;
    bool on_button_release_event(GdkEventButton *button_event) override;
    bool on_enter_notify_event(GdkEventCrossing* event ) override;
    bool on_leave_notify_event(GdkEventCrossing* event ) override;

public:
    Preview();
    bool get_focus_on_click() const {return _takesFocus;}
    void set_focus_on_click(bool focus_on_click) {_takesFocus = focus_on_click;}
    LinkType get_linked() const;
    void set_linked(LinkType link);
    void set_details(ViewType      view,
                     PreviewSize   size,
                     guint         ratio,
                     guint         border);
    void set_color(int r, int g, int b);
    void set_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf);
    static void set_size_mappings(guint count, GtkIconSize const* sizes);

    decltype(_signal_clicked) signal_clicked() {return _signal_clicked;}
    decltype(_signal_alt_clicked) signal_alt_clicked() {return _signal_alt_clicked;}
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif /* SEEN_EEK_PREVIEW_H */

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
