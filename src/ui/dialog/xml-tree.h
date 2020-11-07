// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This is XML tree editor, which allows direct modifying of all elements
 *   of Inkscape document, including foreign ones.
 *//*
 * Authors: see git history
 * Lauris Kaplinski, 2000
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_DIALOGS_XML_TREE_H
#define SEEN_UI_DIALOGS_XML_TREE_H

#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/paned.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/switch.h>
#include <gtkmm/textview.h>
#include <gtkmm/toolbar.h>
#include <memory>

#include "message.h"
#include "ui/dialog/attrdialog.h"
#include "ui/dialog/dialog-base.h"

class SPDesktop;
class SPObject;
struct SPXMLViewAttrList;
struct SPXMLViewContent;
struct SPXMLViewTree;

namespace Inkscape {
class MessageStack;
class MessageContext;

namespace XML {
class Node;
}

namespace UI {
namespace Dialog {

/**
 * A dialog widget to view and edit the document xml
 *
 */

class XmlTree : public DialogBase
{
public:
    XmlTree ();
    ~XmlTree () override;

    static XmlTree &getInstance() { return *new XmlTree(); }

private:

    /**
     * Is invoked by the desktop tracker when the desktop changes.
     */
    void set_tree_desktop(SPDesktop *desktop);

    /**
     * Is invoked when the document changes
     */
    void set_tree_document(SPDocument *document);

    /**
     * Select a node in the xml tree
     */
    void set_tree_repr(Inkscape::XML::Node *repr);

    /**
     * Sets the XML status bar when the tree is selected.
     */
    void tree_reset_context();

    /**
     * Is the selected tree node editable
     */
    gboolean xml_tree_node_mutable(GtkTreeIter *node);

    /**
     * Callback to close the add dialog on Escape key
     */
    static gboolean quit_on_esc (GtkWidget *w, GdkEventKey *event, GObject */*tbl*/);

    /**
     * Select a node in the xml tree
     */
    void set_tree_select(Inkscape::XML::Node *repr);

    /**
     * Set the attribute list to match the selected node in the tree
     */
    void propagate_tree_select(Inkscape::XML::Node *repr);

    /**
      * Find the current desktop selection
      */
    Inkscape::XML::Node *get_dt_select();

    /**
      * Select the current desktop selection
      */
    void set_dt_select(Inkscape::XML::Node *repr);

    /**
      * Callback for a node in the tree being selected
      */
    static void on_tree_select_row(GtkTreeSelection *selection, gpointer data);
    /**
     * Callback for deferring the `on_tree_select_row` response in order to
     * skip invalid intermediate selection states. In particular,
     * `gtk_tree_store_remove` makes an undesired selection that we will
     * immediately revert and don't want to an early response for.
     */
    static gboolean deferred_on_tree_select_row(gpointer);
    /// Event source ID for the last scheduled `deferred_on_tree_select_row` event.
    guint deferred_on_tree_select_row_id = 0;

    /**
      * Callback when a node is moved in the tree
      */
    static void after_tree_move(SPXMLViewTree *tree, gpointer value, gpointer data);

    /**
      * Callback for when an attribute is edited.
      */
    //static void on_attr_edited(SPXMLViewAttrList *attributes, const gchar * name, const gchar * value, gpointer /*data*/);

    /**
      * Callback for when attribute list values change
      */
    //static void on_attr_row_changed(SPXMLViewAttrList *attributes, const gchar * name, gpointer data);

    /**
      * Enable widgets based on current selections
      */
    void on_tree_select_row_enable(GtkTreeIter *node);
    void on_tree_unselect_row_disable();
    void on_tree_unselect_row_hide();
    void on_attr_unselect_row_disable();

    void onNameChanged();
    void onCreateNameChanged();

    /**
      * Callbacks for changes in desktop selection and current document
      */
    void on_desktop_selection_changed();
    void on_document_replaced(SPDesktop *dt, SPDocument *document);
    static void on_document_uri_set(gchar const *uri, SPDocument *document);

    static void _set_status_message(Inkscape::MessageType type, const gchar *message, GtkWidget *dialog);

    /**
      * Callbacks for toolbar buttons being pressed
      */
    void cmd_new_element_node();
    void cmd_new_text_node();
    void cmd_duplicate_node();
    void cmd_delete_node();
    void cmd_raise_node();
    void cmd_lower_node();
    void cmd_indent_node();
    void cmd_unindent_node();

    void _attrtoggler();
    void _toggleDirection(Gtk::RadioButton *vertical);
    void _resized();
    bool in_dt_coordsys(SPObject const &item);

    /**
     * Can be invoked for setting the desktop. Currently not used.
     */
    void update() override;

    /**
     * Flag to ensure only one operation is performed at once
     */
    gint blocked;

    bool _updating;
    /**
     * Status bar
     */
    std::shared_ptr<Inkscape::MessageStack> _message_stack;
    std::unique_ptr<Inkscape::MessageContext> _message_context;

    /**
     * Signal handlers
     */
    sigc::connection _message_changed_connection;
    sigc::connection document_replaced_connection;
    sigc::connection document_uri_set_connection;
    sigc::connection sel_changed_connection;

    /**
     * Current document and desktop this dialog is attached to
     */
    SPDesktop *current_desktop;
    SPDocument *current_document;

    gint selected_attr;
    Inkscape::XML::Node *selected_repr;

    /* XmlTree Widgets */
    SPXMLViewTree *tree;
    //SPXMLViewAttrList *attributes;
    AttrDialog *attributes;
    Gtk::Box *_attrbox;

    /* XML Node Creation pop-up window */
    Gtk::Entry *name_entry;
    Gtk::Button *create_button;
    Gtk::Paned _paned;

    Gtk::Box node_box;
    Gtk::Box status_box;
    Gtk::Switch _attrswitch;
    Gtk::Label status;
    Gtk::Toolbar tree_toolbar;
    Gtk::ToolButton xml_element_new_button;
    Gtk::ToolButton xml_text_new_button;
    Gtk::ToolButton xml_node_delete_button;
    Gtk::SeparatorToolItem separator;
    Gtk::ToolButton xml_node_duplicate_button;
    Gtk::SeparatorToolItem separator2;
    Gtk::ToolButton unindent_node_button;
    Gtk::ToolButton indent_node_button;
    Gtk::ToolButton raise_node_button;
    Gtk::ToolButton lower_node_button;

    GtkWidget *new_window;
};

}
}
}

#endif

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
