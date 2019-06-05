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

#ifndef SELECTORDIALOG_H
#define SELECTORDIALOG_H

#include <gtkmm/dialog.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <ui/widget/panel.h>

#include "ui/dialog/desktop-tracker.h"

#include "xml/helper-observer.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief The SelectorDialog class
 * A list of CSS selectors will show up in this dialog. This dialog allows one to
 * add and delete selectors. Elements can be added to and removed from the selectors
 * in the dialog. Selection of any selector row selects the matching  objects in
 * the drawing and vice-versa. (Only simple selectors supported for now.)
 *
 * This class must keep two things in sync:
 *   1. The text node of the style element.
 *   2. The Gtk::TreeModel.
 */
class SelectorDialog : public Widget::Panel {

public:
    ~SelectorDialog() override;
    // No default constructor, noncopyable, nonassignable
    SelectorDialog();
    SelectorDialog(SelectorDialog const &d) = delete;
    SelectorDialog operator=(SelectorDialog const &d) = delete;

    static SelectorDialog &getInstance() { return *new SelectorDialog(); }
  private:
    // Monitor <style> element for changes.
    class NodeObserver;

    // Monitor all objects for addition/removal/attribute change
    class NodeWatcher;

    std::vector<SelectorDialog::NodeWatcher*> _nodeWatchers;
    void _nodeAdded(   Inkscape::XML::Node &repr );
    void _nodeRemoved( Inkscape::XML::Node &repr );
    void _nodeChanged( Inkscape::XML::Node &repr );
    // Data structure
    enum coltype { OBJECT, SELECTOR, UNHANDLED };
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(_colSelector);
            add(_colExpand);
            add(_colType);
            add(_colObj);
            add(_colProperties);
            add(_colVisible);
        }
        Gtk::TreeModelColumn<Glib::ustring> _colSelector;       // Selector or matching object id.
        Gtk::TreeModelColumn<bool> _colExpand;                  // Open/Close store row.
        Gtk::TreeModelColumn<gint> _colType;                    // Selector row or child object row.
        Gtk::TreeModelColumn<std::vector<SPObject *> > _colObj; // List of matching objects.
        Gtk::TreeModelColumn<Glib::ustring> _colProperties;     // List of properties.
        Gtk::TreeModelColumn<bool> _colVisible;                 // Make visible or not.
    };
    ModelColumns _mColumns;

    // Override Gtk::TreeStore to control drag-n-drop (only allow dragging and dropping of selectors).
    // See: https://developer.gnome.org/gtkmm-tutorial/stable/sec-treeview-examples.html.en
    //
    // TreeStore implements simple drag and drop (DND) but there appears no way to know when a DND
    // has been completed (other than doing the whole DND ourselves). As a hack, we use
    // on_row_deleted to trigger write of style element.
    class TreeStore : public Gtk::TreeStore {
    protected:
        TreeStore();
        bool row_draggable_vfunc(const Gtk::TreeModel::Path& path) const override;
        bool row_drop_possible_vfunc(const Gtk::TreeModel::Path& path,
                                     const Gtk::SelectionData& selection_data) const override;
        void on_row_deleted(const TreeModel::Path& path) override;

    public:
      static Glib::RefPtr<SelectorDialog::TreeStore> create(SelectorDialog *styledialog);

    private:
        SelectorDialog *_selectordialog;
    };

    // TreeView
    Glib::RefPtr<Gtk::TreeModelFilter> _modelfilter;
    Glib::RefPtr<TreeStore> _store;
    Gtk::TreeView _treeView;
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

} // namespace Dialogc
} // namespace UI
} // namespace Inkscape

#endif // SELECTORDIALOG_H

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
