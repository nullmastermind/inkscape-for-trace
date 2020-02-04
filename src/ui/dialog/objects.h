// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A simple dialog for objects UI.
 *
 * Authors:
 *   Theodore Janeczko
 *   Tavmjong Bah
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *               Tavmjong Bah 2017
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_OBJECTS_PANEL_H
#define SEEN_OBJECTS_PANEL_H

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/scale.h>
#include <gtkmm/dialog.h>
#include "ui/widget/spinbutton.h"
#include "ui/widget/panel.h"
#include "desktop-tracker.h"
#include "ui/widget/style-subject.h"
#include "selection.h"
#include "ui/widget/filter-effect-chooser.h"

class SPObject;
class SPGroup;
struct SPColorSelector;

namespace Inkscape {

namespace UI {

class SelectedColor;

namespace Dialog {


/**
 * A panel that displays objects.
 */
class ObjectsPanel : public UI::Widget::Panel
{
public:
    ObjectsPanel();
    ~ObjectsPanel() override;

    static ObjectsPanel& getInstance();

    void setDesktop( SPDesktop* desktop ) override;
    void setDocument( SPDesktop* desktop, SPDocument* document);

private:
    //Internal Classes:
    class ModelColumns;
    class InternalUIBounce;
    class ObjectWatcher;

    //Connections, Watchers, Trackers:
    
    //Document root watcher
    ObjectsPanel::ObjectWatcher* _rootWatcher;
    
    //All object watchers
    std::map<SPItem*, std::pair<ObjectsPanel::ObjectWatcher*, bool> > _objectWatchers;
    
    //Connection for when the desktop changes
    sigc::connection desktopChangeConn;

    //Connection for when the desktop is destroyed (I.e. its deconstructor is called)
    sigc::connection _desktopDestroyedConnection;

    //Connection for when the document changes
    sigc::connection _documentChangedConnection;
    
    //Connection for when the active layer changes
    sigc::connection _documentChangedCurrentLayer;

    //Connection for when the active selection in the document changes
    sigc::connection _selectionChangedConnection;

    //Connection for when the selection in the dialog changes
    sigc::connection _selectedConnection;
    
    //Connections for when the opacity/blend/blur of the active selection in the document changes
    sigc::connection _isolationConnection;
    sigc::connection _opacityConnection;
    sigc::connection _blendConnection;
    sigc::connection _blurConnection;
    
    sigc::connection _processQueue_sig;
    sigc::connection _executeUpdate_sig;

    //Desktop tracker for grabbing the desktop changed connection
    DesktopTracker _deskTrack;
    
    //Members:
    
    //The current desktop
    SPDesktop* _desktop;
    
    //The current document
    SPDocument* _document;
    
    //Tree data model
    ModelColumns* _model;
    
    //Prevents the composite controls from updating
    bool _blockCompositeUpdate;
    
    //
    InternalUIBounce* _pending;
    bool _pending_update;
    
    //Whether the drag & drop was dragged into an item
    gboolean _dnd_into;
    
    //List of drag & drop source items
    std::vector<SPItem*> _dnd_source;
    
    //Drag & drop target item
    SPItem* _dnd_target;
    
    // Whether the drag sources include a layer
    bool _dnd_source_includes_layer;

    //List of items to change the highlight on
    std::vector<SPItem*> _highlight_target;

    //Show icons in the context menu
    bool _show_contextmenu_icons;

    //GUI Members:
    
    GdkEvent* _toggleEvent;
    
    Gtk::TreeModel::Path _defer_target;

    Glib::RefPtr<Gtk::TreeStore> _store;
    std::list<std::tuple<SPItem*, Gtk::TreeModel::iterator, bool> > _tree_update_queue;
    //When the user selects an item in the document, we need to find that item in the tree view
    //and highlight it. When looking up a specific item in the tree though, we don't want to have
    //to iterate through the whole list, as this would take too long if the list is very long. So
    //we will use a std::map for this instead, which is much faster (and call it _tree_cache). It
    //would have been cleaner to create our own custom tree model, as described here
    //https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Tree_Models
    std::map<SPItem*, Gtk::TreeModel::iterator> _tree_cache;
    std::list<SPItem *> _selected_objects_order; // ordered by time of selection
    std::list<Gtk::TreePath> _paths_to_be_expanded;

    std::vector<Gtk::Widget*> _watching;
    std::vector<Gtk::Widget*> _watchingNonTop;
    std::vector<Gtk::Widget*> _watchingNonBottom;

    Gtk::TreeView _tree;
    Gtk::CellRendererText *_text_renderer;
    Gtk::TreeView::Column *_name_column;
    Gtk::Box _buttonsRow;
    Gtk::Box _buttonsPrimary;
    Gtk::Box _buttonsSecondary;
    Gtk::ScrolledWindow _scroller;
    Gtk::Menu _popupMenu;
    Inkscape::UI::Widget::SpinButton _spinBtn;
    Gtk::VBox _page;

    Gtk::Label _visibleHeader;
    Gtk::Label _lockHeader;
    Gtk::Label _typeHeader;
    Gtk::Label _clipmaskHeader;
    Gtk::Label _highlightHeader;
    Gtk::Label _nameHeader;

    /* Composite Settings (blend, blur, opacity). */
    Inkscape::UI::Widget::SimpleFilterModifier _filter_modifier;

    Gtk::Dialog _colorSelectorDialog;
    std::unique_ptr<Inkscape::UI::SelectedColor> _selectedColor;
    
    //Methods:
    
    ObjectsPanel(ObjectsPanel const &) = delete; // no copy
    ObjectsPanel &operator=(ObjectsPanel const &) = delete; // no assign

    void _styleButton( Gtk::Button& btn, char const* iconName, char const* tooltip );
    void _fireAction( unsigned int code );
    
    Gtk::MenuItem& _addPopupItem( SPDesktop *desktop, unsigned int code, int id );
    
    void _setVisibleIter( const Gtk::TreeModel::iterator& iter, const bool visible );
    void _setLockedIter( const Gtk::TreeModel::iterator& iter, const bool locked );
    
    bool _handleButtonEvent(GdkEventButton *event);
    bool _handleKeyEvent(GdkEventKey *event);
    
    void _storeHighlightTarget(const Gtk::TreeModel::iterator& iter);
    void _storeDragSource(const Gtk::TreeModel::iterator& iter);
    bool _handleDragDrop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
    void _handleEdited(const Glib::ustring& path, const Glib::ustring& new_text);
    void _handleEditingCancelled();

    void _doTreeMove();
    void _renameObject(Gtk::TreeModel::Row row, const Glib::ustring& name);

    void _pushTreeSelectionToCurrent();
    bool _selectItemCallback(const Gtk::TreeModel::iterator& iter, bool *setOpacity, bool *first_pass);
    bool _clearPrevSelectionState(const Gtk::TreeModel::iterator& iter);
    void _desktopDestroyed(SPDesktop* desktop);
    
    void _checkTreeSelection();

    void _blockAllSignals(bool should_block);
    void _takeAction( int val );
    bool _executeAction();
    bool _executeUpdate();

    void _setExpanded( const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path, bool isexpanded );
    void _setCollapsed(SPGroup * group);
    
    bool _noSelection( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );
    bool _rowSelectFunction( Glib::RefPtr<Gtk::TreeModel> const & model, Gtk::TreeModel::Path const & path, bool b );

    void _compositingChanged( const Gtk::TreeModel::iterator& iter, bool *setValues );
    void _updateComposite();
    void _setCompositingValues(SPItem *item);
    
    bool _findInTreeCache(SPItem* item, Gtk::TreeModel::iterator &tree_iter);
    void _updateObject(SPObject *obj, bool recurse);

    void _objectsSelected(Selection *sel);
    void _updateObjectSelected(SPItem* item, bool scrollto, bool expand);

    void _removeWatchers(bool only_unused);
    void _addWatcher(SPItem* item);
    void _objectsChangedWrapper(SPObject *obj);
    void _objectsChanged(SPObject *obj);
    bool _processQueue();
    void _queueObject(SPObject* obj, Gtk::TreeModel::Row* parentRow);
    void _addObjectToTree(SPItem* item, const Gtk::TreeModel::Row &parentRow, bool expanded);

    void _isolationChangedIter(const Gtk::TreeIter &iter);
    void _isolationValueChanged();

    void _opacityChangedIter(const Gtk::TreeIter& iter);
    void _opacityValueChanged();

    void _blendChangedIter(const Gtk::TreeIter &iter);
    void _blendValueChanged();

    void _blurChangedIter(const Gtk::TreeIter& iter, double blur);
    void _blurValueChanged();

    void _highlightPickerColorMod();
};



} //namespace Dialogs
} //namespace UI
} //namespace Inkscape



#endif // SEEN_OBJECTS_PANEL_H

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
