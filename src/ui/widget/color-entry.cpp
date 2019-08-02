// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Entry widget for typing color value in css form
 *//*
 * Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <glibmm.h>
#include <glibmm/i18n.h>
#include <iomanip>

#include "color-entry.h"

namespace Inkscape {
namespace UI {
namespace Widget {

ColorEntry::ColorEntry(SelectedColor &color)
    : _color(color)
    , _updating(false)
    , _updatingrgba(false)
    , _prevpos(0)
    , _lastcolor(0)
{
    _color_changed_connection = color.signal_changed.connect(sigc::mem_fun(this, &ColorEntry::_onColorChanged));
    _color_dragged_connection = color.signal_dragged.connect(sigc::mem_fun(this, &ColorEntry::_onColorChanged));
    signal_activate().connect(sigc::mem_fun(this, &ColorEntry::_onColorChanged));
    get_buffer()->signal_inserted_text().connect(sigc::mem_fun(this, &ColorEntry::_inputCheck));
    _onColorChanged();

    // add extra character for pasting a hash, '#11223344'
    set_max_length(9);
    set_width_chars(8);
    set_tooltip_text(_("Hexadecimal RGBA value of the color"));
}

ColorEntry::~ColorEntry()
{
    _color_changed_connection.disconnect();
    _color_dragged_connection.disconnect();
}

void ColorEntry::_inputCheck(guint pos, const gchar * /*chars*/, guint n_chars)
{
    // remember position of last character, so we can remove it.
    // we only overflow by 1 character at most.
    _prevpos = pos + n_chars - 1;
}

void ColorEntry::on_changed()
{
    if (_updating) {
        return;
    }
    if (_updatingrgba) {
        return;  // Typing text into entry box
    }

    Glib::ustring text = get_text();
    bool changed = false;

    // Coerce the value format to hexadecimal
    for (auto it = text.begin(); it != text.end(); /*++it*/) {
        if (!g_ascii_isxdigit(*it)) {
            text.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (text.size() > 8) {
        text.erase(_prevpos, 1);
        changed = true;
    }

    // autofill rules
    gchar *str = g_strdup(text.c_str());
    gchar *end = nullptr;
    guint64 rgba = g_ascii_strtoull(str, &end, 16);
    ptrdiff_t len = end - str;
    if (len < 8) {
        if (len == 0) {
            rgba = _lastcolor;
        } else if (len <= 2) {
            if (len == 1) {
                rgba *= 17;
            }
            rgba = (rgba << 24) + (rgba << 16) + (rgba << 8);
        } else if (len <= 4) {
            // display as rrggbbaa
            rgba = rgba << (4 * (4 - len));
            guint64 r = rgba & 0xf000;
            guint64 g = rgba & 0x0f00;
            guint64 b = rgba & 0x00f0;
            guint64 a = rgba & 0x000f;
            rgba = 17 * ((r << 12) + (g << 8) + (b << 4) + a);
        } else {
            rgba = rgba << (4 * (8 - len));
        }

        if (len == 7) {
            rgba = (rgba & 0xfffffff0) + (_lastcolor & 0x00f);
        } else if (len == 5) {
            rgba = (rgba & 0xfffff000) + (_lastcolor & 0xfff);
        } else if (len != 4 && len != 8) {
            rgba = (rgba & 0xffffff00) + (_lastcolor & 0x0ff);
        }
    }

    _updatingrgba = true;
    if (changed) {
        set_text(str);
    }
    SPColor color(rgba);
    _color.setColorAlpha(color, SP_RGBA32_A_F(rgba));
    _updatingrgba = false;

    g_free(str);
}


void ColorEntry::_onColorChanged()
{
    if (_updatingrgba) {
        return;
    }

    SPColor color = _color.color();
    gdouble alpha = _color.alpha();

    _lastcolor = color.toRGBA32(alpha);
    Glib::ustring text = Glib::ustring::format(std::hex, std::setw(8), std::setfill(L'0'), _lastcolor);

    Glib::ustring old_text = get_text();
    if (old_text != text) {
        _updating = true;
        set_text(text);
        _updating = false;
    }
}
}
}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
