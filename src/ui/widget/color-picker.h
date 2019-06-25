// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Color picker button and window.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *
 * Copyright (C) Authors 2000-2005
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef __COLOR_PICKER_H__
#define __COLOR_PICKER_H__

#include "labelled.h"

#include <cstddef>

#include "ui/selected-color.h"
#include "ui/widget/color-preview.h"
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/window.h>
#include <sigc++/sigc++.h>

struct SPColorSelector;

namespace Inkscape
{
namespace UI
{
namespace Widget
{


class ColorPicker : public Gtk::Button {
public:

    ColorPicker (const Glib::ustring& title,
                 const Glib::ustring& tip,
                 const guint32 rgba,
                 bool undo);

    ~ColorPicker() override;

    void setRgba32 (guint32 rgba);
    void setSensitive(bool sensitive);
    void closeWindow();
    sigc::connection connectChanged (const sigc::slot<void,guint>& slot)
        { return _changed_signal.connect (slot); }

protected:

    void _onSelectedColorChanged();
    void on_clicked() override;
    virtual void on_changed (guint32);

    ColorPreview        _preview;

    /*const*/ Glib::ustring _title;
    sigc::signal<void,guint32> _changed_signal;
    guint32             _rgba;
    bool                _undo;
    bool                _updating;

    //Dialog
    void setupDialog(const Glib::ustring &title);
    //Inkscape::UI::Dialog::Dialog _colorSelectorDialog;
    Gtk::Dialog _colorSelectorDialog;
    SelectedColor _selected_color;
    Gtk::Widget *_color_selector;
};


class LabelledColorPicker : public Labelled {
public:

    LabelledColorPicker (const Glib::ustring& label,
                         const Glib::ustring& title,
                         const Glib::ustring& tip,
                         const guint32 rgba,
                         bool undo) : Labelled(label, tip, new ColorPicker(title, tip, rgba, undo)) {}

    ~LabelledColorPicker() override
        { static_cast<ColorPicker*>(_widget)->~ColorPicker(); }

    void setRgba32 (guint32 rgba)
        { static_cast<ColorPicker*>(_widget)->setRgba32 (rgba); }

    void closeWindow()
        { static_cast<ColorPicker*>(_widget)->closeWindow (); }

    sigc::connection connectChanged (const sigc::slot<void,guint>& slot)
        { return static_cast<ColorPicker*>(_widget)->connectChanged(slot); }
};

}//namespace Widget
}//namespace UI
}//namespace Inkscape

#endif /* !__COLOR_PICKER_H__ */

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
