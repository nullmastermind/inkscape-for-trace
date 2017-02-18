/** @file
 * @brief A dialog for CSS selectors
 */
/* Authors:
 *   Kamalpreet Kaur Grewal
 *   Tavmjong Bah
 *
 * Copyright (C) Kamalpreet Kaur Grewal 2016 <grewalkamal005@gmail.com>
 * Copyright (C) Tavmjong Bah 2017 <tavmjong@free.fr>
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
#include <gtkmm/paned.h>

#include "desktop.h"
#include "document.h"
#include "ui/dialog/cssdialog.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief The StyleDialog class
 * A list of CSS selectors will show up in this dialog. This dialog allows one to
 * add and delete selectors. Elements can be added to and removed from the selectors
 * in the dialog. Selection of any selector row selects the matching  objects in
 * the drawing and vice-versa. (Only simple selectors supported for now.)
 *
 * This class must keep two things in sync:
 *   1. The text node of the style element.
 *   2. The Gtk::TreeModel.
 */
class StyleDialog : public Widget::Panel {

public:
    StyleDialog();
    ~StyleDialog();

    static StyleDialog &getInstance() { return *new StyleDialog(); }

    void setDesktop(SPDesktop* desktop);

private:

    class NodeObserver;

    // Data structure
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(_colSelector);
            add(_colAddRemove);
            add(_colObj);
            add(_colProperties);
        }
        Gtk::TreeModelColumn<Glib::ustring> _colSelector;       // Selector or matching object id.
        Gtk::TreeModelColumn<bool> _colAddRemove;               // Selector row or child object row.
        Gtk::TreeModelColumn<std::vector<SPObject *> > _colObj; // List of matching objects.
        Gtk::TreeModelColumn<Glib::ustring> _colProperties;     // List of properties.
    };
    ModelColumns _mColumns;

    // TreeView
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::TreeStore> _store;

    // Widgets
    Gtk::VPaned _paned;
    Gtk::VBox _mainBox;
    Gtk::HBox _buttonBox;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::Button* del;
    Gtk::Button* create;
    CssDialog *_cssPane;

    // Variables - Inkscape
    SPDesktop* _desktop;  // To do: use panel _desktop
    SPDocument* _document;
    bool _updating;
    Inkscape::XML::Node *_textNode;
    
    // Reading and writing the style element.
    Inkscape::XML::Node *_getStyleTextNode();
    void _readStyleElement();
    void _writeStyleElement();
    
    // Manipulate Tree
    void _addToSelector(Gtk::TreeModel::Row row);
    void _removeFromSelector(Gtk::TreeModel::Row row);
    Glib::ustring _getIdList(std::vector<SPObject *>);
    std::vector<SPObject *> _getObjVec(Glib::ustring selector);
    void _insertClass(const std::vector<SPObject *>& objVec, const Glib::ustring& className);
    void _selectObjects(int, int);
    void _updateCSSPanel();

    // Signal handlers
    void _addSelector();
    void _delSelector();
    bool _handleButtonEvent(GdkEventButton *event);
    void _buttonEventsSelectObjs(GdkEventButton *event);
    void _selectRow(Selection *); // Select row in tree when selection changed.
    void _selChanged();

    // Signal handlers for CssDialog
    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);
    bool _delProperty(GdkEventButton *event);

    // GUI
    void _styleButton(Gtk::Button& btn, char const* iconName, char const* tooltip);
};

} // namespace Dialogc
} // namespace UI
} // namespace Inkscape

#endif // STYLEDIALOG_H

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
