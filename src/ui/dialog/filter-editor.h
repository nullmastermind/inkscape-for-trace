/** @file
 * @brief Filter Editor dialog
 */
/* Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2017 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_FILTER_EFFECTS_H
#define INKSCAPE_UI_DIALOG_FILTER_EFFECTS_H

#include "attributes.h"
#include "ui/widget/panel.h"
#include "sp-filter.h"

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

namespace Inkscape {
namespace UI {
namespace Dialog {

class FilterEditorDialog : public UI::Widget::Panel {
public:

    FilterEditorDialog();
    ~FilterEditorDialog();

    static FilterEditorDialog &getInstance()
    { return *new FilterEditorDialog(); }

//    void set_attrs_locked(const bool);
private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::ComboBoxText *FilterList;
    Gtk::SpinButton *FilterFERX, *FilterFERY, *FilterFERW, *FilterFERH;
    Gtk::DrawingArea *FilterPreview;
    Gtk::Image *FilterPrimitiveDescImage;
    Gtk::Box *FilterPrimitiveParameters;
    Gtk::Label *FilterPrimitiveDescText;
    Gtk::ComboBox *FilterPrimitiveList;
    Gtk::Button *FilterPrimitiveAdd;
    Gtk::ListStore *FilterStore;
    Gtk::Box *FilterEditor;
};
}
}
}
#endif
