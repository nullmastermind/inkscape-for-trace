// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_COLOR_ICC_SELECTOR_H
#define SEEN_SP_COLOR_ICC_SELECTOR_H

#include <gtkmm/widget.h>
#include <gtkmm/grid.h>

#include "ui/selected-color.h"

namespace Inkscape {

class ColorProfile;

namespace UI {
namespace Widget {

class ColorICCSelectorImpl;

class ColorICCSelector
    : public Gtk::Grid
      {
  public:
    static const gchar *MODE_NAME;

    ColorICCSelector(SelectedColor &color);
    ~ColorICCSelector() override;

    virtual void init();

  protected:
    void on_show() override;

    virtual void _colorChanged();

    void _recalcColor(gboolean changing);

  private:
    friend class ColorICCSelectorImpl;

    // By default, disallow copy constructor and assignment operator
    ColorICCSelector(const ColorICCSelector &obj);
    ColorICCSelector &operator=(const ColorICCSelector &obj);

    ColorICCSelectorImpl *_impl;
};


class ColorICCSelectorFactory : public ColorSelectorFactory {
  public:
    Gtk::Widget *createWidget(SelectedColor &color) const override;
    Glib::ustring modeName() const override;
};
}
}
}
#endif // SEEN_SP_COLOR_ICC_SELECTOR_H

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
