// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * XML editor.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   David Turner
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2006 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include "xml-tree.h"

#include <glibmm/i18n.h>
#include <memory>


#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "message-context.h"
#include "message-stack.h"
#include "shortcuts.h"
#include "verbs.h"

#include "object/sp-root.h"
#include "object/sp-string.h"

#include "ui/dialog-events.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/tools/tool-base.h"

#include "widgets/sp-xmlview-tree.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

XmlTree::XmlTree() :
    UI::Widget::Panel("/dialogs/xml/", SP_VERB_DIALOG_XML_EDITOR),
    blocked (0),
    _message_stack (nullptr),
    _message_context (nullptr),
    current_desktop (nullptr),
    current_document (nullptr),
    selected_attr (0),
    selected_repr (nullptr),
    tree (nullptr),
    status (""),
    tree_toolbar(),
    xml_element_new_button ( _("New element node")),
    xml_text_new_button ( _("New text node")),
    xml_node_delete_button ( Q_("nodeAsInXMLdialogTooltip|Delete node")),
    xml_node_duplicate_button ( _("Duplicate node")),
    unindent_node_button(),
    indent_node_button(),
    raise_node_button(),
    lower_node_button(),
    new_window(nullptr)
{

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (!desktop) {
        return;
    }
    flowbox_content = Gtk::manage(new Inkscape::UI::Widget::InkFlowBox("XMLFlow"));

    Gtk::Box *contents = _getContents();
    contents->set_spacing(0);
    contents->set_size_request(320, 260);

    status.set_halign(Gtk::ALIGN_START);
    status.set_valign(Gtk::ALIGN_CENTER);
    status.set_size_request(1, -1);
    status.set_markup("");
    status.set_line_wrap(true);
    status_box.pack_start( status, TRUE, TRUE, 0);

    contents->pack_start(*flowbox_content, true, true, 0);

    _message_stack = std::make_shared<Inkscape::MessageStack>();
    _message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(_message_stack));
    _message_changed_connection = _message_stack->connectChanged(
            sigc::bind(sigc::ptr_fun(_set_status_message), GTK_WIDGET(status.gobj())));

    /* tree view */
    flowbox_content->insert(&node_box, _("_Nodes"), FLOWBOX_PAGE_NODES, true, -1);
    tree = SP_XMLVIEW_TREE(sp_xmlview_tree_new(nullptr, nullptr, nullptr));
    gtk_widget_set_tooltip_text( GTK_WIDGET(tree), _("Drag to reorder nodes") );

    tree_toolbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);

    auto xml_element_new_icon = Gtk::manage(sp_get_icon_image("xml-element-new", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_element_new_button.set_icon_widget(*xml_element_new_icon);
    xml_element_new_button.set_tooltip_text(_("New element node"));
    xml_element_new_button.set_sensitive(false);
    tree_toolbar.add(xml_element_new_button);

    auto xml_text_new_icon = Gtk::manage(sp_get_icon_image("xml-text-new", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_text_new_button.set_icon_widget(*xml_text_new_icon);
    xml_text_new_button.set_tooltip_text(_("New text node"));
    xml_text_new_button.set_sensitive(false);
    tree_toolbar.add(xml_text_new_button);

    auto xml_node_duplicate_icon = Gtk::manage(sp_get_icon_image("xml-node-duplicate", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_node_duplicate_button.set_icon_widget(*xml_node_duplicate_icon);
    xml_node_duplicate_button.set_tooltip_text(_("Duplicate node"));
    xml_node_duplicate_button.set_sensitive(false);
    tree_toolbar.add(xml_node_duplicate_button);

    tree_toolbar.add(separator);

    auto xml_node_delete_icon = Gtk::manage(sp_get_icon_image("xml-node-delete", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_node_delete_button.set_icon_widget(*xml_node_delete_icon);
    xml_node_delete_button.set_tooltip_text(Q_("nodeAsInXMLdialogTooltip|Delete node"));
    xml_node_delete_button.set_sensitive(false);
    tree_toolbar.add(xml_node_delete_button);

    tree_toolbar.add(separator2);

    auto format_indent_less_icon = Gtk::manage(sp_get_icon_image("format-indent-less", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    unindent_node_button.set_icon_widget(*format_indent_less_icon);
    unindent_node_button.set_label(_("Unindent node"));
    unindent_node_button.set_tooltip_text(_("Unindent node"));
    unindent_node_button.set_sensitive(false);
    tree_toolbar.add(unindent_node_button);

    auto format_indent_more_icon = Gtk::manage(sp_get_icon_image("format-indent-more", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    indent_node_button.set_icon_widget(*format_indent_more_icon);
    indent_node_button.set_label(_("Indent node"));
    indent_node_button.set_tooltip_text(_("Indent node"));
    indent_node_button.set_sensitive(false);
    tree_toolbar.add(indent_node_button);

    auto go_up_icon = Gtk::manage(sp_get_icon_image("go-up", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    raise_node_button.set_icon_widget(*go_up_icon);
    raise_node_button.set_label(_("Raise node"));
    raise_node_button.set_tooltip_text(_("Raise node"));
    raise_node_button.set_sensitive(false);
    tree_toolbar.add(raise_node_button);

    auto go_down_icon = Gtk::manage(sp_get_icon_image("go-down", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    lower_node_button.set_icon_widget(*go_down_icon);
    lower_node_button.set_label(_("Lower node"));
    lower_node_button.set_tooltip_text(_("Lower node"));
    lower_node_button.set_sensitive(false);
    tree_toolbar.add(lower_node_button);

    node_box.pack_start(tree_toolbar, FALSE, TRUE, 0);

    Gtk::ScrolledWindow *tree_scroller = new Gtk::ScrolledWindow();
    tree_scroller->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
    tree_scroller->set_shadow_type(Gtk::SHADOW_IN);
    tree_scroller->add(*Gtk::manage(Glib::wrap(GTK_WIDGET(tree))));

    node_box.pack_start(*tree_scroller);

    node_box.pack_end(status_box, false, false, 2);

    /* attributes */
    attributes = new AttrDialog;
    attr_box.pack_start(*attributes);
    flowbox_content->insert(&attr_box, _("_Attributes"), FLOWBOX_PAGE_ATTRS, false, 200);

    /* Signal handlers */
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
    g_signal_connect (G_OBJECT(selection), "changed", G_CALLBACK (on_tree_select_row), this);
    g_signal_connect_after( G_OBJECT(tree), "tree_move", G_CALLBACK(after_tree_move), this);

    xml_element_new_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_new_element_node));
    xml_text_new_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_new_text_node));
    xml_node_duplicate_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_duplicate_node));
    xml_node_delete_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_delete_node));
    unindent_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_unindent_node));
    indent_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_indent_node));
    raise_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_raise_node));
    lower_node_button.signal_clicked().connect(sigc::mem_fun(*this, &XmlTree::cmd_lower_node));

    styles = new StyleDialog;
    styles_box.pack_start(*styles);
    flowbox_content->insert(&styles_box, _("_Styles"), FLOWBOX_PAGE_STYLES, false, 200);

    selectors = new SelectorDialog;
    selectors_box.pack_start(*selectors);
    flowbox_content->insert(&selectors_box, _("S_electors"), FLOWBOX_PAGE_SELECTORS, false, 200);

    desktopChangeConn = deskTrack.connectDesktopChanged( sigc::mem_fun(*this, &XmlTree::set_tree_desktop) );
    deskTrack.connect(GTK_WIDGET(gobj()));

    /* initial show/hide */
    show_all();
    tree_reset_context();

    g_assert(desktop != nullptr);
    set_tree_desktop(desktop);

}

void XmlTree::present()
{
    set_tree_select(get_dt_select());

    UI::Widget::Panel::present();
}

XmlTree::~XmlTree ()
{
    set_tree_desktop(nullptr);

    _message_changed_connection.disconnect();
    _message_context = nullptr;
    _message_stack = nullptr;
    _message_changed_connection.~connection();
}

void XmlTree::setDesktop(SPDesktop *desktop)
{
    Panel::setDesktop(desktop);
    deskTrack.setBase(desktop);
}

/**
 * Sets the XML status bar when the tree is selected.
 */
void XmlTree::tree_reset_context()
{
    _message_context->set(Inkscape::NORMAL_MESSAGE,
                          _("<b>Click</b> to select nodes, <b>drag</b> to rearrange."));
}


void XmlTree::set_tree_desktop(SPDesktop *desktop)
{
    if ( desktop == current_desktop ) {
        return;
    }

    if (current_desktop) {
        sel_changed_connection.disconnect();
        document_replaced_connection.disconnect();
    }
    current_desktop = desktop;
    if (desktop) {
        sel_changed_connection = desktop->getSelection()->connectChanged(sigc::hide(sigc::mem_fun(this, &XmlTree::on_desktop_selection_changed)));
        document_replaced_connection = desktop->connectDocumentReplaced(sigc::mem_fun(this, &XmlTree::on_document_replaced));

        set_tree_document(desktop->getDocument());
    } else {
        set_tree_document(nullptr);
    }

} // end of set_tree_desktop()


void XmlTree::set_tree_document(SPDocument *document)
{
    if (document == current_document) {
        return;
    }

    if (current_document) {
        document_uri_set_connection.disconnect();
    }
    current_document = document;
    if (current_document) {

        document_uri_set_connection = current_document->connectURISet(sigc::bind(sigc::ptr_fun(&on_document_uri_set), current_document));
        on_document_uri_set( current_document->getDocumentURI(), current_document );
        set_tree_repr(current_document->getReprRoot());
    } else {
        set_tree_repr(nullptr);
    }
}



void XmlTree::set_tree_repr(Inkscape::XML::Node *repr)
{
    if (repr == selected_repr) {
        return;
    }

    sp_xmlview_tree_set_repr(tree, repr);
    if (repr) {
        set_tree_select(get_dt_select());
    } else {
        set_tree_select(nullptr);
    }

    propagate_tree_select(selected_repr);

}



void XmlTree::set_tree_select(Inkscape::XML::Node *repr)
{
    if (selected_repr) {
        Inkscape::GC::release(selected_repr);
    }

    selected_repr = repr;
    if (repr) {
        GtkTreeIter node;

        Inkscape::GC::anchor(selected_repr);

        if (sp_xmlview_tree_get_repr_node(SP_XMLVIEW_TREE(tree), repr, &node)) {

            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
            gtk_tree_selection_unselect_all (selection);

            GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree->store), &node);
            gtk_tree_view_expand_to_path (GTK_TREE_VIEW(tree), path);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree), path, nullptr, TRUE, 0.66, 0.0);
            gtk_tree_path_free(path);

            gtk_tree_selection_select_iter(selection, &node);

        } else {
            g_message("XmlTree::set_tree_select : Couldn't find repr node");
        }
    } else {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        gtk_tree_selection_unselect_all (selection);

        on_tree_unselect_row_disable();
    }
    propagate_tree_select(repr);
}



void XmlTree::propagate_tree_select(Inkscape::XML::Node *repr)
{
    if (repr && 
       (repr->type() == Inkscape::XML::ELEMENT_NODE ||
        repr->type() == Inkscape::XML::TEXT_NODE ||
        repr->type() == Inkscape::XML::COMMENT_NODE)) 
    {
        attributes->setRepr(repr);
        //styles->setRepr(repr);
        // selectors->setRepr(repr);
    } else {
        attributes->setRepr(nullptr);
        //styles->setRepr(nullptr);
        // selectors->setRepr(nullptr);
    }
}


Inkscape::XML::Node *XmlTree::get_dt_select()
{
    if (!current_desktop) {
        return nullptr;
    }
    return current_desktop->getSelection()->singleRepr();
}



void XmlTree::set_dt_select(Inkscape::XML::Node *repr)
{
    if (!current_desktop) {
        return;
    }

    Inkscape::Selection *selection = current_desktop->getSelection();

    SPObject *object;
    if (repr) {
        while ( ( repr->type() != Inkscape::XML::ELEMENT_NODE )
                && repr->parent() )
        {
            repr = repr->parent();
        } // end of while loop

        object = current_desktop->getDocument()->getObjectByRepr(repr);
    } else {
        object = nullptr;
    }

    blocked++;
    if ( object && in_dt_coordsys(*object)
         && !(SP_IS_STRING(object) ||
                SP_IS_ROOT(object)     ) )
    {
            /* We cannot set selection to root or string - they are not items and selection is not
             * equipped to deal with them */
            selection->set(SP_ITEM(object));
    }
    blocked--;

} // end of set_dt_select()


void XmlTree::on_tree_select_row(GtkTreeSelection *selection, gpointer data)
{
    XmlTree *self = static_cast<XmlTree *>(data);

    GtkTreeIter   iter;
    GtkTreeModel *model;

    if (self->selected_repr) {
        Inkscape::GC::release(self->selected_repr);
        self->selected_repr = nullptr;
    }


    if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
        // Nothing selected, update widgets
        self->propagate_tree_select(nullptr);
        self->set_dt_select(nullptr);
        self->on_tree_unselect_row_disable();
        return;
    }

    Inkscape::XML::Node *repr = sp_xmlview_tree_node_get_repr(model, &iter);
    g_assert(repr != nullptr);


    self->selected_repr = repr;
    Inkscape::GC::anchor(self->selected_repr);

    self->propagate_tree_select(self->selected_repr);

    self->set_dt_select(self->selected_repr);

    self->tree_reset_context();

    self->on_tree_select_row_enable(&iter);
}


void XmlTree::after_tree_move(SPXMLViewTree * /*tree*/, gpointer value, gpointer data)
{
    XmlTree *self = static_cast<XmlTree *>(data);
    guint val = GPOINTER_TO_UINT(value);

    if (val) {
        DocumentUndo::done(self->current_document, SP_VERB_DIALOG_XML_EDITOR,
                                   _("Drag XML subtree"));
    } else {
        //DocumentUndo::cancel(self->current_document);
        /*
         * There was a problem with drag & drop,
         * data is probably not synchronized, so reload the tree
         */
        SPDocument *document = self->current_document;
        self->set_tree_document(nullptr);
        self->set_tree_document(document);
    }
}

void XmlTree::_set_status_message(Inkscape::MessageType /*type*/, const gchar *message, GtkWidget *widget)
{
    if (widget) {
        gtk_label_set_markup(GTK_LABEL(widget), message ? message : "");
    }
}

void XmlTree::on_tree_select_row_enable(GtkTreeIter *node)
{
    if (!node) {
        return;
    }

    Inkscape::XML::Node *repr = sp_xmlview_tree_node_get_repr(GTK_TREE_MODEL(tree->store), node);
    Inkscape::XML::Node *parent=repr->parent();

    //on_tree_select_row_enable_if_mutable
    xml_node_duplicate_button.set_sensitive(xml_tree_node_mutable(node));
    xml_node_delete_button.set_sensitive(xml_tree_node_mutable(node));

    //on_tree_select_row_enable_if_element
    if (repr->type() == Inkscape::XML::ELEMENT_NODE) {
        xml_element_new_button.set_sensitive(true);
        xml_text_new_button.set_sensitive(true);

    } else {
        xml_element_new_button.set_sensitive(false);
        xml_text_new_button.set_sensitive(false);
    }

    //on_tree_select_row_enable_if_has_grandparent
    {
        GtkTreeIter parent;
        if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(tree->store), &parent, node)) {
            GtkTreeIter grandparent;
            if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(tree->store), &grandparent, &parent)) {
                unindent_node_button.set_sensitive(true);
            } else {
                unindent_node_button.set_sensitive(false);
            }
        } else {
            unindent_node_button.set_sensitive(false);
        }
    }
    // on_tree_select_row_enable_if_indentable
    gboolean indentable = FALSE;

    if (xml_tree_node_mutable(node)) {
        Inkscape::XML::Node *prev;

        if ( parent && repr != parent->firstChild() ) {
            g_assert(parent->firstChild());

            // skip to the child just before the current repr
            for ( prev = parent->firstChild() ;
                  prev && prev->next() != repr ;
                  prev = prev->next() ){};

            if (prev && (prev->type() == Inkscape::XML::ELEMENT_NODE)) {
                indentable = TRUE;
            }
        }
    }

    indent_node_button.set_sensitive(indentable);

    //on_tree_select_row_enable_if_not_first_child
    {
        if ( parent && repr != parent->firstChild() ) {
            raise_node_button.set_sensitive(true);
        } else {
            raise_node_button.set_sensitive(false);
        }
    }

    //on_tree_select_row_enable_if_not_last_child
    {
        if ( parent && (parent->parent() && repr->next())) {
            lower_node_button.set_sensitive(true);
        } else {
            lower_node_button.set_sensitive(false);
        }
    }
}


gboolean XmlTree::xml_tree_node_mutable(GtkTreeIter *node)
{
    // top-level is immutable, obviously
    GtkTreeIter parent;
    if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(tree->store), &parent, node)) {
        return false;
    }


    // if not in base level (where namedview, defs, etc go), we're mutable
    GtkTreeIter child;
    if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(tree->store), &child, &parent)) {
        return true;
    }

    Inkscape::XML::Node *repr;
    repr = sp_xmlview_tree_node_get_repr(GTK_TREE_MODEL(tree->store), node);
    g_assert(repr);

    // don't let "defs" or "namedview" disappear
    if ( !strcmp(repr->name(),"svg:defs") ||
         !strcmp(repr->name(),"sodipodi:namedview") ) {
        return false;
    }

    // everyone else is okay, I guess.  :)
    return true;
}



void XmlTree::on_tree_unselect_row_disable()
{
    xml_text_new_button.set_sensitive(false);
    xml_element_new_button.set_sensitive(false);
    xml_node_delete_button.set_sensitive(false);
    xml_node_duplicate_button.set_sensitive(false);
    unindent_node_button.set_sensitive(false);
    indent_node_button.set_sensitive(false);
    raise_node_button.set_sensitive(false);
    lower_node_button.set_sensitive(false);
}

void XmlTree::onCreateNameChanged()
{
    Glib::ustring text = name_entry->get_text();
    /* TODO: need to do checking a little more rigorous than this */
    create_button->set_sensitive(!text.empty());
}

void XmlTree::on_desktop_selection_changed()
{
    if (!blocked++) {
        Inkscape::XML::Node *node = get_dt_select();
        set_tree_select(node);
    }
    blocked--;
}

void XmlTree::on_document_replaced(SPDesktop *dt, SPDocument *doc)
{
    if (current_desktop)
        sel_changed_connection.disconnect();

    sel_changed_connection = dt->getSelection()->connectChanged(sigc::hide(sigc::mem_fun(this, &XmlTree::on_desktop_selection_changed)));
    set_tree_document(doc);
}

void XmlTree::on_document_uri_set(gchar const * /*uri*/, SPDocument * /*document*/)
{
/*
 * Seems to be no way to set the title on a docked dialog
    gchar title[500];
    sp_ui_dialog_title_string(Inkscape::Verb::get(SP_VERB_DIALOG_XML_EDITOR), title);
    gchar *t = g_strdup_printf("%s: %s", document->getName(), title);
    //gtk_window_set_title(GTK_WINDOW(dlg), t);
    g_free(t);
*/
}

gboolean XmlTree::quit_on_esc (GtkWidget *w, GdkEventKey *event, GObject */*tbl*/)
{
    switch (Inkscape::UI::Tools::get_latin_keyval (event)) {
        case GDK_KEY_Escape: // defocus
            gtk_widget_destroy(w);
            return TRUE;
        case GDK_KEY_Return: // create
        case GDK_KEY_KP_Enter:
            gtk_widget_destroy(w);
            return TRUE;
    }
    return FALSE;
}

void XmlTree::cmd_new_element_node()
{
    Gtk::Dialog dialog;
    Gtk::Entry entry;

    dialog.get_content_area()->pack_start(entry);
    dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Create", Gtk::RESPONSE_OK);
    dialog.show_all();

    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        Glib::ustring new_name = entry.get_text();
        if (!new_name.empty()) {
            Inkscape::XML::Document *xml_doc = current_document->getReprDoc();
            Inkscape::XML::Node *new_repr;
            new_repr = xml_doc->createElement(new_name.c_str());
            Inkscape::GC::release(new_repr);
            selected_repr->appendChild(new_repr);
            set_tree_select(new_repr);
            set_dt_select(new_repr);

            DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                               _("Create new element node"));
        }
    }
} // end of cmd_new_element_node()


void XmlTree::cmd_new_text_node()
{
    g_assert(selected_repr != nullptr);

    Inkscape::XML::Document *xml_doc = current_document->getReprDoc();
    Inkscape::XML::Node *text = xml_doc->createTextNode("");
    selected_repr->appendChild(text);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       _("Create new text node"));

    set_tree_select(text);
    set_dt_select(text);
}

void XmlTree::cmd_duplicate_node()
{
    g_assert(selected_repr != nullptr);

    Inkscape::XML::Node *parent = selected_repr->parent();
    Inkscape::XML::Node *dup = selected_repr->duplicate(parent->document());
    parent->addChild(dup, selected_repr);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       _("Duplicate node"));

    GtkTreeIter node;

    if (sp_xmlview_tree_get_repr_node(SP_XMLVIEW_TREE(tree), dup, &node)) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        gtk_tree_selection_select_iter(selection, &node);
    }
}

void XmlTree::cmd_delete_node()
{
    g_assert(selected_repr != nullptr);
    sp_repr_unparent(selected_repr);

    reinterpret_cast<SPObject *>(current_desktop->currentLayer())->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       Q_("nodeAsInXMLinHistoryDialog|Delete node"));
}

void XmlTree::cmd_raise_node()
{
    g_assert(selected_repr != nullptr);


    Inkscape::XML::Node *parent = selected_repr->parent();
    g_return_if_fail(parent != nullptr);
    g_return_if_fail(parent->firstChild() != selected_repr);

    Inkscape::XML::Node *ref = nullptr;
    Inkscape::XML::Node *before = parent->firstChild();
    while (before && (before->next() != selected_repr)) {
        ref = before;
        before = before->next();
    }

    parent->changeOrder(selected_repr, ref);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       _("Raise node"));

    set_tree_select(selected_repr);
    set_dt_select(selected_repr);
}



void XmlTree::cmd_lower_node()
{
    g_assert(selected_repr != nullptr);

    g_return_if_fail(selected_repr->next() != nullptr);
    Inkscape::XML::Node *parent = selected_repr->parent();

    parent->changeOrder(selected_repr, selected_repr->next());

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       _("Lower node"));

    set_tree_select(selected_repr);
    set_dt_select(selected_repr);
}

void XmlTree::cmd_indent_node()
{
    Inkscape::XML::Node *repr = selected_repr;
    g_assert(repr != nullptr);

    Inkscape::XML::Node *parent = repr->parent();
    g_return_if_fail(parent != nullptr);
    g_return_if_fail(parent->firstChild() != repr);

    Inkscape::XML::Node* prev = parent->firstChild();
    while (prev && (prev->next() != repr)) {
        prev = prev->next();
    }
    g_return_if_fail(prev != nullptr);
    g_return_if_fail(prev->type() == Inkscape::XML::ELEMENT_NODE);

    Inkscape::XML::Node* ref = nullptr;
    if (prev->firstChild()) {
        for( ref = prev->firstChild() ; ref->next() ; ref = ref->next() ){};
    }

    parent->removeChild(repr);
    prev->addChild(repr, ref);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       _("Indent node"));
    set_tree_select(repr);
    set_dt_select(repr);

} // end of cmd_indent_node()



void XmlTree::cmd_unindent_node()
{
    Inkscape::XML::Node *repr = selected_repr;
    g_assert(repr != nullptr);

    Inkscape::XML::Node *parent = repr->parent();
    g_return_if_fail(parent);
    Inkscape::XML::Node *grandparent = parent->parent();
    g_return_if_fail(grandparent);

    parent->removeChild(repr);
    grandparent->addChild(repr, parent);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR,
                       _("Unindent node"));
    set_tree_select(repr);
    set_dt_select(repr);

} // end of cmd_unindent_node()

/** Returns true iff \a item is suitable to be included in the selection, in particular
    whether it has a bounding box in the desktop coordinate system for rendering resize handles.

    Descendents of <defs> nodes (markers etc.) return false, for example.
*/
bool XmlTree::in_dt_coordsys(SPObject const &item)
{
    /* Definition based on sp_item_i2doc_affine. */
    SPObject const *child = &item;
    g_return_val_if_fail(child != nullptr, false);
    for(;;) {
        if (!SP_IS_ITEM(child)) {
            return false;
        }
        SPObject const * const parent = child->parent;
        if (parent == nullptr) {
            break;
        }
        child = parent;
    }
    g_assert(SP_IS_ROOT(child));
    /* Relevance: Otherwise, I'm not sure whether to return true or false. */
    return true;
}

}
}
}

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
