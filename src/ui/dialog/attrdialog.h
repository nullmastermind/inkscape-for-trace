// SPDX-License-Identifier: GPL-2.0-or-later
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

#ifndef SEEN_UI_DIALOGS_ATTRDIALOG_H
#define SEEN_UI_DIALOGS_ATTRDIALOG_H

#include "desktop.h"
#include "message.h"
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/popover.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/treeview.h>
#include <ui/widget/panel.h>

#define ATTR_DIALOG(obj) (dynamic_cast<Inkscape::UI::Dialog::AttrDialog*>((Inkscape::UI::Dialog::AttrDialog*)obj))

namespace Inkscape {
class MessageStack;
class MessageContext;
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
	  add(_attributeName);
	  add(_attributeValue);
      add(_attributeValueRender);
        }
        Gtk::TreeModelColumn<Glib::ustring> _attributeName;
        Gtk::TreeModelColumn<Glib::ustring> _attributeValue;
        Gtk::TreeModelColumn<Glib::ustring> _attributeValueRender;
    };
    AttrColumns _attrColumns;

    // TreeView
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::ListStore> _store;
    Gtk::CellRendererText *_nameRenderer;
    Gtk::CellRendererText *_valueRenderer;
    Gtk::TreeViewColumn *_nameCol;
    Gtk::TreeViewColumn *_valueCol;
    Gtk::TreeModel::Path _modelpath;
    Gtk::Popover *_popover;
    Gtk::TextView *_textview;
    Glib::ustring valuepath;
    Glib::ustring valueediting;

    /**
     * Status bar
     */
    std::shared_ptr<Inkscape::MessageStack> _message_stack;
    std::unique_ptr<Inkscape::MessageContext> _message_context;

    // Widgets
    Gtk::VBox _mainBox;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::ScrolledWindow _scrolled_text_view;
    Gtk::HBox _buttonBox;
    Gtk::Button _buttonAddAttribute;
    // Variables - Inkscape
    SPDesktop* _desktop;
    Inkscape::XML::Node* _repr;
    Gtk::HBox status_box;
    Gtk::Label status;
    bool _updating;

    // Helper functions
    void setDesktop(SPDesktop* desktop) override;
    void setRepr(Inkscape::XML::Node * repr);
    void setUndo(Glib::ustring const &event_description);
    /**
     * Sets the XML status bar, depending on which attr is selected.
     */
    void attr_reset_context(gint attr);
    static void _set_status_message(Inkscape::MessageType type, const gchar *message, GtkWidget *dialog);

    /**
     * Signal handlers
     */
    sigc::connection _message_changed_connection;
    void onAttrChanged(Inkscape::XML::Node *repr, const gchar * name, const gchar * new_value);
    bool onNameKeyPressed(GdkEventKey *event, Gtk::Entry *entry);
    bool onValueKeyPressed(GdkEventKey *event, Gtk::Entry *entry);
    void onAttrDelete(Glib::ustring path);
    bool onAttrCreate(GdkEventButton *event);
    bool onKeyPressed(GdkEventKey *event);
    void popClosed();
    void startNameEdit(Gtk::CellEditable *cell, const Glib::ustring &path);
    void startValueEdit(Gtk::CellEditable *cell, const Glib::ustring &path);
    void nameEdited(const Glib::ustring &path, const Glib::ustring &name);
    void valueEdited(const Glib::ustring &path, const Glib::ustring &value);
    void textViewMap();
    void valueCanceledPop();
    void valueEditedPop();
};


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // ATTRDIALOG_H
