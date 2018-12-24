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

#ifndef CSSDIALOG_H
#define CSSDIALOG_H

#include "message.h"
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <ui/widget/panel.h>

#include "desktop.h"

namespace Inkscape {
class MessageStack;
class MessageContext;
namespace UI {
namespace Dialog {

/**
 * @brief The CssDialog class
 * This dialog allows to add, delete and modify CSS properties for selectors
 * created in Style Dialog. Double clicking any selector in Style dialog, a list
 * of CSS properties will show up in this dialog (if any exist), else new properties
 * can be added and each new property forms a new row in this pane.
 */
class CssDialog : public UI::Widget::Panel
{
public:
    CssDialog();
    ~CssDialog() override;

    static CssDialog &getInstance() { return *new CssDialog(); }

    // Data structure
    class CssColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        CssColumns() {
	  add(_colUnsetProp);
	  add(_propertyLabel);
	  add(_styleSheetVal);
	  add(_styleAttrVal);
	}
        Gtk::TreeModelColumn<bool> _colUnsetProp;
        Gtk::TreeModelColumn<Glib::ustring> _propertyLabel;
        Gtk::TreeModelColumn<Glib::ustring> _styleSheetVal;
        Gtk::TreeModelColumn<Glib::ustring> _styleAttrVal;
    };
    CssColumns _cssColumns;

    /**
     * Status bar
     */
    std::shared_ptr<Inkscape::MessageStack> _message_stack;
    std::unique_ptr<Inkscape::MessageContext> _message_context;

    // TreeView
    Gtk::TreeView _treeView;
    Glib::RefPtr<Gtk::ListStore> _store;
    Gtk::TreeModel::Row _propRow;
    Gtk::CellRendererText *_propRenderer;
    Gtk::CellRendererText *_sheetRenderer;
    Gtk::CellRendererText *_attrRenderer;
    Gtk::TreeViewColumn *_propCol;
    Gtk::TreeViewColumn *_sheetCol;
    Gtk::TreeViewColumn *_attrCol;
    Gtk::HBox status_box;
    Gtk::Label status;

    /**
     * Sets the XML status bar, depending on which attr is selected.
     */
    void css_reset_context(gint css);
    static void _set_status_message(Inkscape::MessageType type, const gchar *message, GtkWidget *dialog);


    // Widgets
    Gtk::VBox _mainBox;
    Gtk::ScrolledWindow _scrolledWindow;
    Gtk::HBox _buttonBox;
    Gtk::Button _buttonAddProperty;

    // Variables - Inkscape
    SPDesktop* _desktop;

    // Helper functions
    void setDesktop(SPDesktop* desktop) override;

    /**
     * Signal handlers
     */
    sigc::connection _message_changed_connection;
    bool _addProperty(GdkEventButton *event);
};


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // CSSDIALOG_H
