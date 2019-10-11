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
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#include <algorithm>
#include <gdkmm/general.h>
#include "preview.h"
#include "preferences.h"

namespace Inkscape {
namespace UI {
namespace Widget {

#define PRIME_BUTTON_MAGIC_NUMBER 1

/* Keep in sync with last value in eek-preview.h */
#define PREVIEW_SIZE_LAST PREVIEW_SIZE_HUGE
#define PREVIEW_SIZE_NEXTFREE (PREVIEW_SIZE_HUGE + 1)

#define PREVIEW_MAX_RATIO 500

void
Preview::set_color(int r, int g, int b )
{
    _r = r;
    _g = g;
    _b = b;

    queue_draw();
}


void
Preview::set_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf)
{
    _previewPixbuf = pixbuf;

    queue_draw();

    if (_scaled)
    {
        _scaled.reset();
    }

    _scaledW = _previewPixbuf->get_width();
    _scaledH = _previewPixbuf->get_height();
}

static gboolean setupDone = FALSE;
static GtkRequisition sizeThings[PREVIEW_SIZE_NEXTFREE];

void
Preview::set_size_mappings( guint count, GtkIconSize const* sizes )
{
    gint width = 0;
    gint height = 0;
    gint smallest = 512;
    gint largest = 0;
    guint i = 0;
    guint delta = 0;

    for ( i = 0; i < count; ++i ) {
        gboolean worked = gtk_icon_size_lookup( sizes[i], &width, &height );
        if ( worked ) {
            if ( width < smallest ) {
                smallest = width;
            }
            if ( width > largest ) {
                largest = width;
            }
        }
    }

    smallest = (smallest * 3) / 4;

    delta = largest - smallest;

    for ( i = 0; i < G_N_ELEMENTS(sizeThings); ++i ) {
        guint val = smallest + ( (i * delta) / (G_N_ELEMENTS(sizeThings) - 1) );
        sizeThings[i].width = val;
        sizeThings[i].height = val;
    }

    setupDone = TRUE;
}

void
Preview::size_request(GtkRequisition* req) const
{
    int               width   = 0;
    int               height  = 0;

    if ( !setupDone ) {
        GtkIconSize sizes[] = {
            GTK_ICON_SIZE_MENU,
            GTK_ICON_SIZE_SMALL_TOOLBAR,
            GTK_ICON_SIZE_LARGE_TOOLBAR,
            GTK_ICON_SIZE_BUTTON,
            GTK_ICON_SIZE_DIALOG
        };
        set_size_mappings( G_N_ELEMENTS(sizes), sizes );
    }

    width = sizeThings[_size].width;
    height = sizeThings[_size].height;

    if ( _view == VIEW_TYPE_LIST ) {
        width *= 3;
    }

    if ( _ratio != 100 ) {
        width = (width * _ratio) / 100;
        if ( width < 0 ) {
            width = 1;
        }
    }

    req->width = width;
    req->height = height;
}

void
Preview::get_preferred_width_vfunc(int &minimal_width, int &natural_width) const
{
    GtkRequisition requisition;
    size_request(&requisition);
    minimal_width = natural_width = requisition.width;
}

void
Preview::get_preferred_height_vfunc(int &minimal_height, int &natural_height) const
{
    GtkRequisition requisition;
    size_request(&requisition);
    minimal_height = natural_height = requisition.height;
}

bool
Preview::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    auto allocation = get_allocation();

    gint insetTop = 0, insetBottom = 0;
    gint insetLeft = 0, insetRight = 0;

    if (_border == BORDER_SOLID) {
        insetTop = 1;
        insetLeft = 1;
    }
    if (_border == BORDER_SOLID_LAST_ROW) {
        insetTop = insetBottom = 1;
        insetLeft = 1;
    }
    if (_border == BORDER_WIDE) {
        insetTop = insetBottom = 1;
        insetLeft = insetRight = 1;
    }

    auto context = get_style_context();

    context->render_frame(cr,
                          0, 0,
                          allocation.get_width(), allocation.get_height());

    context->render_background(cr,
                               0, 0,
                               allocation.get_width(), allocation.get_height());

    // Border
    if (_border != BORDER_NONE) {
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->rectangle(0, 0, allocation.get_width(), allocation.get_height());
        cr->fill();
    }

    cr->set_source_rgb(_r/65535.0, _g/65535.0, _b/65535.0 );
    cr->rectangle(insetLeft, insetTop, allocation.get_width() - (insetLeft + insetRight), allocation.get_height() - (insetTop + insetBottom));
    cr->fill();

    if (_previewPixbuf )
    {
        if ((allocation.get_width() != _scaledW) || (allocation.get_height() != _scaledH)) {
            if (_scaled)
            {
                _scaled.reset();
            }

            _scaledW = allocation.get_width() - (insetLeft + insetRight);
            _scaledH = allocation.get_height() - (insetTop + insetBottom);

            _scaled = _previewPixbuf->scale_simple(_scaledW,
                                                   _scaledH,
                                                   Gdk::INTERP_BILINEAR);
        }

        Glib::RefPtr<Gdk::Pixbuf> pix = (_scaled) ? _scaled : _previewPixbuf;

        // Border
        if (_border != BORDER_NONE) {
            cr->set_source_rgb(0.0, 0.0, 0.0);
            cr->rectangle(0, 0, allocation.get_width(), allocation.get_height());
            cr->fill();
        }

        Gdk::Cairo::set_source_pixbuf(cr, pix, insetLeft, insetTop);
        cr->paint();
    }

    if (_linked)
    {
        /* Draw arrow */
        GdkRectangle possible = {insetLeft,
                                 insetTop,
                                 (allocation.get_width() - (insetLeft + insetRight)),
                                 (allocation.get_height() - (insetTop + insetBottom))
                                };

        GdkRectangle area = {possible.x,
                             possible.y,
                             possible.width / 2,
                             possible.height / 2 };

        /* Make it square */
        if ( area.width > area.height )
            area.width = area.height;
        if ( area.height > area.width )
            area.height = area.width;

        /* Center it horizontally */
        if ( area.width < possible.width ) {
            int diff = (possible.width - area.width) / 2;
            area.x += diff;
        }

        if (_linked & PREVIEW_LINK_IN)
        {
            context->render_arrow(cr,
                                  G_PI, // Down-pointing arrow
                                  area.x, area.y,
                                  std::min(area.width, area.height)
                                 );
        }

        if (_linked & PREVIEW_LINK_OUT)
        {
            GdkRectangle otherArea = {area.x, area.y, area.width, area.height};
            if ( otherArea.height < possible.height ) {
                otherArea.y = possible.y + (possible.height - otherArea.height);
            }

            context->render_arrow(cr,
                                  G_PI, // Down-pointing arrow
                                  otherArea.x, otherArea.y,
                                  std::min(otherArea.width, otherArea.height)
                                 );
        }

        if (_linked & PREVIEW_LINK_OTHER)
        {
            GdkRectangle otherArea = {insetLeft, area.y, area.width, area.height};
            if ( otherArea.height < possible.height ) {
                otherArea.y = possible.y + (possible.height - otherArea.height) / 2;
            }

            context->render_arrow(cr,
                                  1.5*G_PI, // Left-pointing arrow
                                  otherArea.x, otherArea.y,
                                  std::min(otherArea.width, otherArea.height)
                                 );
        }


        if (_linked & PREVIEW_FILL)
        {
            GdkRectangle otherArea = {possible.x + ((possible.width / 4) - (area.width / 2)),
                                      area.y,
                                      area.width, area.height};
            if ( otherArea.height < possible.height ) {
                otherArea.y = possible.y + (possible.height - otherArea.height) / 2;
            }
            context->render_check(cr,
                                  otherArea.x, otherArea.y,
                                  otherArea.width, otherArea.height );
        }

        if (_linked & PREVIEW_STROKE)
        {
            GdkRectangle otherArea = {possible.x + (((possible.width * 3) / 4) - (area.width / 2)),
                                      area.y,
                                      area.width, area.height};
            if ( otherArea.height < possible.height ) {
                otherArea.y = possible.y + (possible.height - otherArea.height) / 2;
            }
            // This should be a diamond too?
            context->render_check(cr,
                                  otherArea.x, otherArea.y,
                                  otherArea.width, otherArea.height );
        }
    }


    if ( has_focus() ) {
        allocation = get_allocation();

        context->render_focus(cr,
                              0 + 1, 0 + 1,
                              allocation.get_width() - 2, allocation.get_height() - 2 );
    }

    return false;
}


bool
Preview::on_enter_notify_event(GdkEventCrossing* event )
{
    _within = true;
    set_state_flags(_hot ? Gtk::STATE_FLAG_ACTIVE : Gtk::STATE_FLAG_PRELIGHT, false);

    return false;
}

bool
Preview::on_leave_notify_event(GdkEventCrossing* event)
{
    _within = false;
    set_state_flags(Gtk::STATE_FLAG_NORMAL, false);

    return false;
}

bool
Preview::on_button_press_event(GdkEventButton *event)
{
    if (_takesFocus && !has_focus() )
    {
        grab_focus();
    }

    if ( event->button == PRIME_BUTTON_MAGIC_NUMBER ||
            event->button == 2 )
    {
        _hot = true;

        if ( _within )
        {
            set_state_flags(Gtk::STATE_FLAG_ACTIVE, false);
        }
    }

    return false;
}

bool
Preview::on_button_release_event(GdkEventButton* event)
{
    _hot = false;
    set_state_flags(Gtk::STATE_FLAG_NORMAL, false);

    if (_within &&
         (event->button == PRIME_BUTTON_MAGIC_NUMBER ||
          event->button == 2))
    {
        gboolean isAlt = ( ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK) ||
                (event->button == 2));

        if ( isAlt )
        {
            _signal_alt_clicked(2);
        }
        else
        {
            _signal_clicked.emit();
        }
    }

    return false;
}

void
Preview::set_linked(LinkType link)
{
    link = (LinkType)(link & PREVIEW_LINK_ALL);

    if (link != _linked)
    {
        _linked = link;

        queue_draw();
    }
}

LinkType
Preview::get_linked() const
{
    return (LinkType)_linked;
}

void
Preview::set_details(ViewType      view,
                     PreviewSize   size,
                     guint         ratio,
                     guint         border)
{
    _view  = view;

    if ( size > PREVIEW_SIZE_LAST )
    {
        size = PREVIEW_SIZE_LAST;
    }

    _size = size;

    if ( ratio > PREVIEW_MAX_RATIO )
    {
        ratio = PREVIEW_MAX_RATIO;
    }

    _ratio  = ratio;
    _border = border;

    queue_draw();
}

Preview::Preview()
    : _r(0x80),
      _g(0x80),
      _b(0xcc),
      _scaledW(0),
      _scaledH(0),
      _hot(false),
      _within(false),
      _takesFocus(false),
      _view(VIEW_TYPE_LIST),
      _size(PREVIEW_SIZE_SMALL),
      _ratio(100),
      _border(BORDER_NONE),
      _previewPixbuf(nullptr),
      _scaled(nullptr),
      _linked(PREVIEW_LINK_NONE)
{
    set_can_focus(true);
    set_receives_default(true);

    set_sensitive(true);

    add_events(Gdk::BUTTON_PRESS_MASK
              |Gdk::BUTTON_RELEASE_MASK
              |Gdk::KEY_PRESS_MASK
              |Gdk::KEY_RELEASE_MASK
              |Gdk::FOCUS_CHANGE_MASK
              |Gdk::ENTER_NOTIFY_MASK
              |Gdk::LEAVE_NOTIFY_MASK );
}

} // namespace Widget
} // namespace UI
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
