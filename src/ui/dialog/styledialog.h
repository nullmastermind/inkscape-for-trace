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

#ifndef STYLEDIALOG_H
#define STYLEDIALOG_H

#include <ui/widget/panel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/dialog.h>

#include "desktop.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief The StyleDialog class
 * A list of CSS selectors will show up in this dialog.
 */

class StyleDialog : public UI::Widget::Panel
{
public:
    StyleDialog();
    ~StyleDialog();

    static StyleDialog &getInstance() { return *new StyleDialog(); }
    void setDesktop( SPDesktop* desktop);

private:
    void _styleButton( Gtk::Button& btn, char const* iconName, char const* tooltip);
    std::string _setClassAttribute(std::vector<SPObject*>);
    std::map<std::string, std::string>_selectorMap;
    void _populateTree(std::map<std::string, std::string>);

    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        { add(_selectorNumber); add(_selectorLabel); }
        Gtk::TreeModelColumn<int> _selectorNumber;
        Gtk::TreeModelColumn<Glib::ustring> _selectorLabel;
    };

    SPDesktop* _desktop;
    ModelColumns _mColumns;
    Gtk::VBox _mainBox;
    Gtk::HBox _buttonBox;
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::ListStore> _store;
    Gtk::ScrolledWindow _scrolledWindow;

    // Signal handlers
    void _addSelector();
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // STYLEDIALOG_H
