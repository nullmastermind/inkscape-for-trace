/** @file
 * @brief A dialog for XML attributes based on Gtk TreeView
 */
/* Authors:
 *   Martin Owens
 *
 * Copyright (C) Martin Owens 2018 <doctormo@gmail.com>
 *
 * Released under GNU GPLv2 or later, read the file 'COPYING' for more information
 */

#ifndef ATTRDIALOG_H
#define ATTRDIALOG_H

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/dialog.h>
#include <ui/widget/panel.h>

#include "desktop.h"

#define ATTR_DIALOG(obj) (dynamic_cast<Inkscape::UI::Dialog::AttrDialog*>((Inkscape::UI::Dialog::AttrDialog*)obj))

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * @brief The AttrDialog class
 * This dialog allows to add, delete and modify XML attributes created in the
 * xml editor.
 */
class AttrDialog : public UI::Widget::Panel
{
public:
    AttrDialog();
    ~AttrDialog() override;

    static AttrDialog &getInstance() { return *new AttrDialog(); }

    // Data structure
    class AttrColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        AttrColumns() {
	  add(_colUnsetAttr);
	  add(_attributeName);
	  add(_attributeValue);
	}
        Gtk::TreeModelColumn<bool> _colUnsetAttr;
        Gtk::TreeModelColumn<Glib::ustring> _attributeName;
        Gtk::TreeModelColumn<Glib::ustring> _attributeValue;
    };
    AttrColumns _attrColumns;

    // TreeView
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::ListStore> _store;
    Gtk::CellRendererText *_nameRenderer;
    Gtk::CellRendererText *_valueRenderer;
    Gtk::TreeViewColumn *_nameCol;
    Gtk::TreeViewColumn *_valueCol;

    // Widgets
    Gtk::VBox _mainBox;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::HBox _buttonBox;
    Gtk::Button _buttonAddAttribute;

    // Variables - Inkscape
    SPDesktop* _desktop;
    Inkscape::XML::Node* _repr;

    // Helper functions
    void setDesktop(SPDesktop* desktop) override;
    void setRepr(Inkscape::XML::Node * repr);

    // Signal handlers
    void onAttrChanged(Inkscape::XML::Node *repr, const gchar * name, const gchar * new_value);
    bool onAttrCreate(GdkEventButton *event);
    bool onAttrDelete(GdkEventButton *event);
    void nameEdited(const Glib::ustring &path, const Glib::ustring &name);
    void valueEdited(const Glib::ustring &path, const Glib::ustring &value);

};


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // ATTRDIALOG_H
