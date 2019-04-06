// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_DIALOGS_STYLEDIALOG_H
#define SEEN_UI_DIALOGS_STYLEDIALOG_H

#include "desktop.h"
#include "message.h"

#include <glibmm/regex.h>
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <ui/widget/panel.h>

#define STYLE_DIALOG(obj) (dynamic_cast<Inkscape::UI::Dialog::StyleDialog *>((Inkscape::UI::Dialog::StyleDialog *)obj))
#define REMOVE_SPACES(x)                                                                                               \
    x.erase(0, x.find_first_not_of(' '));                                                                              \
    x.erase(x.find_last_not_of(' ') + 1);

namespace Inkscape {
class MessageStack;
class MessageContext;
namespace UI {
namespace Dialog {

/**
 * @brief The StyleDialog class
 * This dialog allows to add, delete and modify CSS properties for selectors
 * created in Style Dialog. Double clicking any selector in Style dialog, a list
 * of CSS properties will show up in this dialog (if any exist), else new properties
 * can be added and each new property forms a new row in this pane.
 */
class StyleDialog : public UI::Widget::Panel
{
public:
    StyleDialog();
    ~StyleDialog() override;

    static StyleDialog &getInstance() { return *new StyleDialog(); }
    Gtk::TreeView _treeView;
  private:
    // Monitor <style> element for changes.
    class NodeObserver;

    // Monitor all objects for addition/removal/attribute change
    class NodeWatcher;

    std::vector<StyleDialog::NodeWatcher*> _nodeWatchers;
    void _nodeAdded(   Inkscape::XML::Node &repr );
    void _nodeRemoved( Inkscape::XML::Node &repr );
    void _nodeChanged( Inkscape::XML::Node &repr );
    // Data structure
    class CssColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        CssColumns() {
            add(deleteButton);
            add(label);
            add(_styleAttrVal);
            add(label_color);
            add(attr_color);
        }
        Gtk::TreeModelColumn<bool> deleteButton;
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<Glib::ustring> _styleAttrVal;
        Gtk::TreeModelColumn<Gdk::RGBA> label_color;
        Gtk::TreeModelColumn<Gdk::RGBA> attr_color;
    };
    CssColumns _cssColumns;

    /**
     * Status bar
     */
    std::shared_ptr<Inkscape::MessageStack> _message_stack;
    std::unique_ptr<Inkscape::MessageContext> _message_context;

    // TreeView
    Glib::RefPtr<TreeStore> _store;

    // Widgets
    Gtk::Paned _paned;
    Gtk::Box   _mainBox;
    Gtk::Box   _buttonBox;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::Button* del;
    Gtk::Button* create;

    // Reading and writing the style element.
    Inkscape::XML::Node *_getStyleTextNode();
    void _readStyleElement();
    void _writeStyleElement();

    // Update watchers
    void _addWatcherRecursive(Inkscape::XML::Node *node);
    void _updateWatchers();
    
    // Manipulate Tree
    void _addToSelector(Gtk::TreeModel::Row row);
    void _removeFromSelector(Gtk::TreeModel::Row row);
    Glib::ustring _getIdList(std::vector<SPObject *>);
    std::vector<SPObject *> _getObjVec(Glib::ustring selector);
    void _insertClass(const std::vector<SPObject *>& objVec, const Glib::ustring& className);
    void _selectObjects(int, int);

    // Variables
    bool _updating;  // Prevent cyclic actions: read <-> write, select via dialog <-> via desktop
    Inkscape::XML::Node *_textNode; // Track so we know when to add a NodeObserver.

    // Signals and handlers - External
    sigc::connection _document_replaced_connection;
    sigc::connection _desktop_changed_connection;
    sigc::connection _selection_changed_connection;

    void _handleDocumentReplaced(SPDesktop* desktop, SPDocument *document);
    void _handleDesktopChanged(SPDesktop* desktop);
    void _handleSelectionChanged();
    void _rowExpand(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
    void _rowCollapse(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
    void _closeDialog(Gtk::Dialog *textDialogPtr);

    DesktopTracker _desktopTracker;

    Inkscape::XML::SignalObserver _objObserver; // Track object in selected row (for style change).

    // Signal and handlers - Internal
    void _addSelector();
    void _delSelector();
    bool _handleButtonEvent(GdkEventButton *event);
    void _buttonEventsSelectObjs(GdkEventButton *event);
    void _selectRow(); // Select row in tree when selection changed.

    // GUI
    void _styleButton(Gtk::Button& btn, char const* iconName, char const* tooltip);
};


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // STYLEDIALOG_H
