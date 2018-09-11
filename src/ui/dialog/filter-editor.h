// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Filter Editor dialog
 */
/* Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2017 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_FILTER_EDITOR_H
#define INKSCAPE_UI_DIALOG_FILTER_EDITOR_H

#include <gtkmm/notebook.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/builder.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/combobox.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/liststore.h>

#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

#include "ui/widget/panel.h"


namespace Inkscape {
namespace UI {
namespace Dialog {

class FilterEditorDialog : public UI::Widget::Panel {
public:

    FilterEditorDialog();
    ~FilterEditorDialog() override;

    static FilterEditorDialog &getInstance()
    { return *new FilterEditorDialog(); }

//    void set_attrs_locked(const bool);
private:
    Glib::RefPtr<Gtk::Builder> builder;
    Glib::RefPtr<Glib::Object> FilterStore;
    Gtk::Box *FilterEditor;
};
}
}
}
#endif
