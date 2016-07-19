/** @file
 * @brief A dialog for CSS selectors
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef CSSDIALOG_H
#define CSSDIALOG_H

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/dialog.h>
#include <ui/widget/panel.h>

#include "desktop.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief The CssDialog class
 * A list of CSS properties will show up in this dialog.
 */
class CssDialog : public UI::Widget::Panel
{
public:
    CssDialog();
    ~CssDialog();

    static CssDialog &getInstance() { return *new CssDialog(); }
    void setDesktop( SPDesktop* desktop);

    class CssColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        CssColumns()
        { add(_colUnsetProp); add(_propertyLabel); }
        Gtk::TreeModelColumn<bool> _colUnsetProp;
        Gtk::TreeModelColumn<Glib::ustring> _propertyLabel;
    };

    SPDesktop* _desktop;
    SPDesktop* _targetDesktop;
    CssColumns _cssColumns;
    Gtk::VBox _mainBox;
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::ListStore> _store;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::TreeModel::Row _propRow;
    Gtk::CellRendererText *_textRenderer;
    Gtk::TreeViewColumn *_propCol;
    Glib::ustring _editedProp;

    void _handleButtonEvent(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *col);
};


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // CSSDIALOG_H
