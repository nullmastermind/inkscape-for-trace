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

#ifndef SEEN_COLOR_ENTRY_H
#define SEEN_COLOR_ENTRY_H

#include <gtkmm/entry.h>
#include "ui/selected-color.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class ColorEntry : public Gtk::Entry
{
public:
    ColorEntry(SelectedColor &color);
    ~ColorEntry() override;

protected:
    void on_changed() override;

private:
    void _onColorChanged();
    void _inputCheck(guint pos, const gchar * /*chars*/, guint /*n_chars*/);

    SelectedColor &_color;
    sigc::connection _color_changed_connection;
    sigc::connection _color_dragged_connection;
    bool _updating;
    bool _updatingrgba;
    guint32 _lastcolor;
    int _prevpos;
};

}
}
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
