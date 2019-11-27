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

#ifndef STYLEDIALOG_H
#define STYLEDIALOG_H

#include "style-enums.h"
#include <glibmm/regex.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/builder.h>
#include <gtkmm/celleditable.h>
#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/entrycompletion.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/switch.h>
#include <gtkmm/tooltip.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/viewport.h>
#include <ui/widget/panel.h>

#include "ui/dialog/desktop-tracker.h"

#include "xml/helper-observer.h"

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
    ~StyleDialog() override;
    // No default constructor, noncopyable, nonassignable
    StyleDialog();
    StyleDialog(StyleDialog const &d) = delete;
    StyleDialog operator=(StyleDialog const &d) = delete;

    static StyleDialog &getInstance() { return *new StyleDialog(); }
    void setCurrentSelector(Glib::ustring current_selector);
    Gtk::TreeView *_current_css_tree;
    Gtk::TreeViewColumn *_current_value_col;
    Gtk::TreeModel::Path _current_path;
    bool _deletion;
    Glib::ustring fixCSSSelectors(Glib::ustring selector);
    void readStyleElement();

  private:
    // Monitor <style> element for changes.
    class NodeObserver;
    // Monitor all objects for addition/removal/attribute change
    class NodeWatcher;
    Glib::RefPtr<Glib::Regex> r_props = Glib::Regex::create("\\s*;\\s*");
    Glib::RefPtr<Glib::Regex> r_pair = Glib::Regex::create("\\s*:\\s*");
    std::vector<StyleDialog::NodeWatcher *> _nodeWatchers;
    void _nodeAdded(Inkscape::XML::Node &repr);
    void _nodeRemoved(Inkscape::XML::Node &repr);
    void _nodeChanged(Inkscape::XML::Node &repr);
    /* void _stylesheetChanged( Inkscape::XML::Node &repr ); */
    // Data structure
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
      public:
        ModelColumns()
        {
            add(_colActive);
            add(_colName);
            add(_colValue);
            add(_colStrike);
            add(_colSelector);
            add(_colSelectorPos);
            add(_colOwner);
            add(_colLinked);
            add(_colHref);
        }
        Gtk::TreeModelColumn<bool> _colActive;            // Active or inactive property
        Gtk::TreeModelColumn<Glib::ustring> _colName;     // Name of the property.
        Gtk::TreeModelColumn<Glib::ustring> _colValue;    // Value of the property.
        Gtk::TreeModelColumn<bool> _colStrike;            // Property not used, overloaded
        Gtk::TreeModelColumn<Glib::ustring> _colSelector; // Style or matching object id.
        Gtk::TreeModelColumn<gint> _colSelectorPos;       // Position of the selector to handle dup selectors
        Gtk::TreeModelColumn<Glib::ustring> _colOwner;    // Store the owner of the property for popup
        Gtk::TreeModelColumn<bool> _colLinked;            // Other object linked
        Gtk::TreeModelColumn<SPObject *> _colHref;        // Is going to another object
    };
    ModelColumns _mColumns;

    class CSSData : public Gtk::TreeModel::ColumnRecord {
      public:
        CSSData() { add(_colCSSData); }
        Gtk::TreeModelColumn<Glib::ustring> _colCSSData; // Name of the property.
    };
    CSSData _mCSSData;
    guint _deleted_pos;
    // Widgets
    Gtk::ScrolledWindow _scrolledWindow;
    Glib::RefPtr<Gtk::Adjustment> _vadj;
    Gtk::Box _mainBox;
    Gtk::Box _styleBox;
    // Reading and writing the style element.
    Inkscape::XML::Node *_getStyleTextNode();
    Glib::RefPtr<Gtk::TreeModel> _selectTree(Glib::ustring selector);
    void _writeStyleElement(Glib::RefPtr<Gtk::TreeStore> store, Glib::ustring selector,
                            Glib::ustring new_selector = "");
    // void _selectorActivate(Glib::RefPtr<Gtk::TreeStore> store, Gtk::Label *selector, Gtk::Entry *selector_edit);
    bool _selectorEditKeyPress(GdkEventKey *event, Glib::RefPtr<Gtk::TreeStore> store, Gtk::Label *selector,
                               Gtk::Entry *selector_edit);
    bool _selectorStartEdit(GdkEventButton *event, Gtk::Label *selector, Gtk::Entry *selector_edit);
    void _activeToggled(const Glib::ustring &path, Glib::RefPtr<Gtk::TreeStore> store);
    bool _addRow(GdkEventButton *evt, Glib::RefPtr<Gtk::TreeStore> store, Gtk::TreeView *css_tree,
                 Glib::ustring selector, gint pos);
    void _onPropDelete(Glib::ustring path, Glib::RefPtr<Gtk::TreeStore> store);
    void _nameEdited(const Glib::ustring &path, const Glib::ustring &name, Glib::RefPtr<Gtk::TreeStore> store,
                     Gtk::TreeView *css_tree);
    bool _onNameKeyReleased(GdkEventKey *event, Gtk::Entry *entry);
    bool _onValueKeyReleased(GdkEventKey *event, Gtk::Entry *entry);
    bool _onNameKeyPressed(GdkEventKey *event, Gtk::Entry *entry);
    bool _onValueKeyPressed(GdkEventKey *event, Gtk::Entry *entry);
    void _onLinkObj(Glib::ustring path, Glib::RefPtr<Gtk::TreeStore> store);
    void _valueEdited(const Glib::ustring &path, const Glib::ustring &value, Glib::RefPtr<Gtk::TreeStore> store);
    void _startNameEdit(Gtk::CellEditable *cell, const Glib::ustring &path);

    void _startValueEdit(Gtk::CellEditable *cell, const Glib::ustring &path, Glib::RefPtr<Gtk::TreeStore> store);
    void _setAutocompletion(Gtk::Entry *entry, SPStyleEnum const cssenum[]);
    void _setAutocompletion(Gtk::Entry *entry, Glib::ustring name);
    bool _on_foreach_iter(const Gtk::TreeModel::iterator &iter);
    void _reload();
    void _vscrool();
    bool _scroollock;
    double _scroolpos;
    Glib::ustring _current_selector;

    // Update watchers
    void _addWatcherRecursive(Inkscape::XML::Node *node);
    void _updateWatchers();

    // Manipulate Tree
    std::vector<SPObject *> _getObjVec(Glib::ustring selector);
    std::map<Glib::ustring, Glib::ustring> parseStyle(Glib::ustring style_string);
    std::map<Glib::ustring, Glib::ustring> _owner_style;
    void _addOwnerStyle(Glib::ustring name, Glib::ustring selector);
    // Variables
    Inkscape::XML::Node *_textNode; // Track so we know when to add a NodeObserver.
    bool _updating;                 // Prevent cyclic actions: read <-> write, select via dialog <-> via desktop

    // Signals and handlers - External
    sigc::connection _document_replaced_connection;
    sigc::connection _desktop_changed_connection;
    sigc::connection _selection_changed_connection;

    void _handleDocumentReplaced(SPDesktop *desktop, SPDocument *document);
    void _handleDesktopChanged(SPDesktop *desktop);
    void _handleSelectionChanged();
    void _closeDialog(Gtk::Dialog *textDialogPtr);
    DesktopTracker _desktopTracker;
};

} // namespace Dialog
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
