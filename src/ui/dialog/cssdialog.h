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

#ifndef SEEN_UI_DIALOGS_CSSDIALOG_H
#define SEEN_UI_DIALOGS_CSSDIALOG_H

#include "desktop.h"
#include "message.h"

#include <glibmm/regex.h>
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <ui/widget/panel.h>

#define CSS_DIALOG(obj) (dynamic_cast<Inkscape::UI::Dialog::CssDialog *>((Inkscape::UI::Dialog::CssDialog *)obj))
#define REMOVE_SPACES(x)                                                                                               \
    x.erase(0, x.find_first_not_of(' '));                                                                              \
    x.erase(x.find_last_not_of(' ') + 1);

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
            add(deleteButton);
            add(label);
            add(_styleSheetVal);
            add(_styleAttrVal);
            add(label_color);
            add(attr_color);
            add(attr_strike);
            add(editable);
        }
        Gtk::TreeModelColumn<bool> deleteButton;
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<Glib::ustring> _styleAttrVal;
        Gtk::TreeModelColumn<Glib::ustring> _styleSheetVal;
        Gtk::TreeModelColumn<Gdk::RGBA> label_color;
        Gtk::TreeModelColumn<Gdk::RGBA> attr_color;
        Gtk::TreeModelColumn<bool> attr_strike;
        Gtk::TreeModelColumn<bool> editable;
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


    // Variables - Inkscape
    SPDesktop* _desktop;
    Inkscape::XML::Node *_repr;

    // Helper functions
    void setDesktop(SPDesktop* desktop) override;
    void setRepr(Inkscape::XML::Node *repr);

    // Parsing functions
    std::map<Glib::ustring, Glib::ustring> parseStyle(Glib::ustring style_string);
    Glib::ustring compileStyle(std::map<Glib::ustring, Glib::ustring> props);

    // Signal handlers
    void onAttrChanged(Inkscape::XML::Node *repr, const gchar *name, const gchar *new_value);

  private:
    Glib::RefPtr<Glib::Regex> r_props = Glib::Regex::create("\\s*;\\s*");
    Glib::RefPtr<Glib::Regex> r_pair = Glib::Regex::create("\\s*:\\s*");

    bool onPropertyCreate(GdkEventButton *event);
    void onPropertyDelete(Glib::ustring path);
    bool setStyleProperty(Glib::ustring name, Glib::ustring value);

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
