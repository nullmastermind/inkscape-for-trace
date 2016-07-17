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
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/dialog.h>
#include <gtkmm/treeselection.h>

#include "desktop.h"
#include "document.h"
#include "ui/dialog/cssdialog.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief The StyleDialog class
 * A list of CSS selectors will show up in this dialog.
 */
typedef std::pair<std::pair<std::string, std::vector<SPObject *> >, std::string>
_selectorVecType;

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

    std::vector<_selectorVecType>_selectorVec;
    std::vector<_selectorVecType> _getSelectorVec();
    std::string _populateTree(std::vector<_selectorVecType>);
    bool _handleButtonEvent(GdkEventButton *event);
    void _buttonEventsSelectObjs(GdkEventButton *event);
    void _selectObjects(int, int);
    void _checkAllChildren(Gtk::TreeModel::Children& children);
    Inkscape::XML::Node *_styleElementNode();
    void _updateStyleContent();
    void _selectRow(Selection *);

    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        { add(_selectorLabel); add(_colAddRemove); add(_colObj); }
        Gtk::TreeModelColumn<Glib::ustring> _selectorLabel;
        Gtk::TreeModelColumn<bool> _colAddRemove;
        Gtk::TreeModelColumn<std::vector<SPObject *> > _colObj;
    };

    SPDesktop* _desktop;
    SPDesktop* _targetDesktop;
    ModelColumns _mColumns;
    Gtk::VBox _mainBox;
    Gtk::HBox _buttonBox;
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::TreeStore> _store;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::Button* del;
    Gtk::Button* create;
    SPDocument* _document;
    bool _styleExists;
    Inkscape::XML::Node *_styleChild;
    unsigned _num;
    std::string _selectorName;
    std::string _selectorValue;
    Gtk::TreeModel::Row _row;
    bool _newDrawing;
    CssDialog *_cssPane;

    // Signal handlers
    void _addSelector();
    void _delSelector();
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // STYLEDIALOG_H
