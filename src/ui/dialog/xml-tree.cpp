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
#include "verbs.h"

#include "object/sp-root.h"
#include "object/sp-string.h"

#include "ui/dialog-events.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/tools/tool-base.h"

#include "widgets/sp-xmlview-tree.h"

namespace {
/**
 * Set the orientation of `paned` to vertical or horizontal, and make the first child resizable
 * if vertical, and the second child resizable if horizontal.
 * @pre `paned` has two children
 */
void paned_set_vertical(Gtk::Paned &paned, bool vertical)
{
    paned.child_property_resize(*paned.get_child1()) = vertical;
    assert(paned.child_property_resize(*paned.get_child2()));
    paned.set_orientation(vertical ? Gtk::ORIENTATION_VERTICAL : Gtk::ORIENTATION_HORIZONTAL);
}
} // namespace

namespace Inkscape {
namespace UI {
namespace Dialog {

XmlTree::XmlTree()
    : DialogBase("/dialogs/xml/", SP_VERB_DIALOG_XML_EDITOR)
    , blocked(0)
    , _message_stack(nullptr)
    , _message_context(nullptr)
    , current_desktop(nullptr)
    , current_document(nullptr)
    , selected_attr(0)
    , selected_repr(nullptr)
    , tree(nullptr)
    , status("")
    , new_window(nullptr)
    , _updating(false)
    , node_box(Gtk::ORIENTATION_VERTICAL)
    , status_box(Gtk::ORIENTATION_HORIZONTAL)
{
    Gtk::Box *contents = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    status.set_halign(Gtk::ALIGN_START);
    status.set_valign(Gtk::ALIGN_CENTER);
    status.set_size_request(1, -1);
    status.set_markup("");
    status.set_line_wrap(true);
    status.get_style_context()->add_class("inksmall");
    status_box.pack_start( status, TRUE, TRUE, 0);
    contents->pack_start(_paned, true, true, 0);
    contents->set_valign(Gtk::ALIGN_FILL);
    contents->child_property_fill(_paned);

    _paned.set_vexpand(true);
    _message_stack = std::make_shared<Inkscape::MessageStack>();
    _message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(_message_stack));
    _message_changed_connection = _message_stack->connectChanged(
            sigc::bind(sigc::ptr_fun(_set_status_message), GTK_WIDGET(status.gobj())));

    /* tree view */
    tree = SP_XMLVIEW_TREE(sp_xmlview_tree_new(nullptr, nullptr, nullptr));
    gtk_widget_set_tooltip_text( GTK_WIDGET(tree), _("Drag to reorder nodes") );

    tree_toolbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);

    auto xml_element_new_icon = Gtk::manage(sp_get_icon_image("xml-element-new", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_element_new_button.set_icon_widget(*xml_element_new_icon);
    xml_element_new_button.set_label(_("New element node"));
    xml_element_new_button.set_tooltip_text(_("New element node"));
    xml_element_new_button.set_sensitive(false);
    tree_toolbar.add(xml_element_new_button);

    auto xml_text_new_icon = Gtk::manage(sp_get_icon_image("xml-text-new", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_text_new_button.set_icon_widget(*xml_text_new_icon);
    xml_text_new_button.set_label(_("New text node"));
    xml_text_new_button.set_tooltip_text(_("New text node"));
    xml_text_new_button.set_sensitive(false);
    tree_toolbar.add(xml_text_new_button);

    auto xml_node_duplicate_icon = Gtk::manage(sp_get_icon_image("xml-node-duplicate", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_node_duplicate_button.set_icon_widget(*xml_node_duplicate_icon);
    xml_node_duplicate_button.set_label(_("Duplicate node"));
    xml_node_duplicate_button.set_tooltip_text(_("Duplicate node"));
    xml_node_duplicate_button.set_sensitive(false);
    tree_toolbar.add(xml_node_duplicate_button);

    tree_toolbar.add(separator);

    auto xml_node_delete_icon = Gtk::manage(sp_get_icon_image("xml-node-delete", Gtk::ICON_SIZE_LARGE_TOOLBAR));

    xml_node_delete_button.set_icon_widget(*xml_node_delete_icon);
    xml_node_delete_button.set_label(_("Delete node"));
    xml_node_delete_button.set_tooltip_text(_("Delete node"));
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
    tree_scroller->set_overlay_scrolling(false);
    tree_scroller->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
    tree_scroller->set_shadow_type(Gtk::SHADOW_IN);
    tree_scroller->add(*Gtk::manage(Glib::wrap(GTK_WIDGET(tree))));

    node_box.pack_start(*Gtk::manage(tree_scroller));

    node_box.pack_end(status_box, false, false, 2);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool attrtoggler = prefs->getBool("/dialogs/xml/attrtoggler", true);
    bool dir = prefs->getBool("/dialogs/xml/vertical", true);
    attributes = new AttrDialog();
    _paned.set_wide_handle(true);
    _paned.pack1(node_box, false, false);
    /* attributes */
    Gtk::Box *actionsbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    actionsbox->set_valign(Gtk::ALIGN_START);
    Gtk::Label *attrtogglerlabel = Gtk::manage(new Gtk::Label(_("Show attributes")));
    attrtogglerlabel->set_margin_right(5);
    _attrswitch.get_style_context()->add_class("inkswitch");
    _attrswitch.get_style_context()->add_class("rawstyle");
    _attrswitch.property_active() = attrtoggler;
    _attrswitch.property_active().signal_changed().connect(sigc::mem_fun(*this, &XmlTree::_attrtoggler));
    attrtogglerlabel->get_style_context()->add_class("inksmall");
    actionsbox->pack_start(*attrtogglerlabel, Gtk::PACK_SHRINK);
    actionsbox->pack_start(_attrswitch, Gtk::PACK_SHRINK);
    Gtk::RadioButton::Group group;
    Gtk::RadioButton *_horizontal = Gtk::manage(new Gtk::RadioButton());
    Gtk::RadioButton *_vertical = Gtk::manage(new Gtk::RadioButton());
    _horizontal->set_image_from_icon_name(INKSCAPE_ICON("horizontal"));
    _vertical->set_image_from_icon_name(INKSCAPE_ICON("vertical"));
    _horizontal->set_group(group);
    _vertical->set_group(group);
    _vertical->set_active(dir);
    _vertical->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &XmlTree::_toggleDirection), _vertical));
    _horizontal->property_draw_indicator() = false;
    _vertical->property_draw_indicator() = false;
    actionsbox->pack_end(*_horizontal, false, false, 0);
    actionsbox->pack_end(*_vertical, false, false, 0);
    _paned.pack2(*attributes, true, false);
    paned_set_vertical(_paned, dir);
    contents->pack_start(*actionsbox, false, false, 0);
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

    set_name("XMLAndAttributesDialog");
    set_spacing(0);
    set_size_request(320, 260);
    show_all();

    int panedpos = prefs->getInt("/dialogs/xml/panedpos", 200);
    _paned.property_position() = panedpos;
    _paned.property_position().signal_changed().connect(sigc::mem_fun(*this, &XmlTree::_resized));

    tree_reset_context();
    pack_start(*Gtk::manage(contents), true, true);
}

void XmlTree::_resized()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/dialogs/xml/panedpos", _paned.property_position());
}

void XmlTree::_toggleDirection(Gtk::RadioButton *vertical)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool dir = vertical->get_active();
    prefs->setBool("/dialogs/xml/vertical", dir);
    paned_set_vertical(_paned, dir);
    prefs->setInt("/dialogs/xml/panedpos", _paned.property_position());
}

void XmlTree::_attrtoggler()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool attrtoggler = !prefs->getBool("/dialogs/xml/attrtoggler", true);
    prefs->setBool("/dialogs/xml/attrtoggler", attrtoggler);
    if (attrtoggler) {
        attributes->show();
    } else {
        attributes->hide();
    }
}

XmlTree::~XmlTree ()
{
    set_tree_desktop(nullptr);
    _message_changed_connection.disconnect();
}

void XmlTree::update()
{
    if (!_app) {
        std::cerr << "XmlTree::update(): _app is null" << std::endl;
        return;
    }

    set_tree_desktop(getDesktop());
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

    if (deferred_on_tree_select_row_id != 0) {
        g_source_destroy(g_main_context_find_source_by_id(nullptr, deferred_on_tree_select_row_id));
        deferred_on_tree_select_row_id = 0;
    }

    if (current_desktop && current_desktop->getDocument()) {
        sel_changed_connection.disconnect();
        document_replaced_connection.disconnect();

        // TODO: Why is this a document property?
        current_desktop->getDocument()->setXMLDialogSelectedObject(nullptr);
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

/**
 * Expand all parent nodes of `repr`
 */
static void expand_parents(SPXMLViewTree *tree, Inkscape::XML::Node *repr)
{
    auto parentrepr = repr->parent();
    if (!parentrepr) {
        return;
    }

    expand_parents(tree, parentrepr);

    GtkTreeIter node;
    if (sp_xmlview_tree_get_repr_node(tree, parentrepr, &node)) {
        GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree->store), &node);
        if (path) {
            gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, false);
        }
    }
}

void XmlTree::set_tree_select(Inkscape::XML::Node *repr)
{
    if (selected_repr) {
        Inkscape::GC::release(selected_repr);
    }

    selected_repr = repr;
    if (current_desktop) {
        current_desktop->getDocument()->setXMLDialogSelectedObject(nullptr);
    }
    if (repr) {
        GtkTreeIter node;

        Inkscape::GC::anchor(selected_repr);

        expand_parents(tree, repr);

        if (sp_xmlview_tree_get_repr_node(SP_XMLVIEW_TREE(tree), repr, &node)) {

            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
            gtk_tree_selection_unselect_all (selection);

            GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree->store), &node);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree), path, nullptr, TRUE, 0.66, 0.0);
            gtk_tree_selection_select_iter(selection, &node);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, false);
            gtk_tree_path_free(path);

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
       (repr->type() == Inkscape::XML::NodeType::ELEMENT_NODE ||
        repr->type() == Inkscape::XML::NodeType::TEXT_NODE ||
        repr->type() == Inkscape::XML::NodeType::COMMENT_NODE))
    {
        attributes->setRepr(repr);
    } else {
        attributes->setRepr(nullptr);
    }
}


Inkscape::XML::Node *XmlTree::get_dt_select()
{
    if (!current_desktop) {
        return nullptr;
    }
    return current_desktop->getSelection()->singleRepr();
}


/**
 * Like SPDesktop::isLayer(), but ignores SPGroup::effectiveLayerMode().
 */
static bool isRealLayer(SPObject const *object)
{
    auto group = dynamic_cast<SPGroup const *>(object);
    return group && group->layerMode() == SPGroup::LAYER;
}

void XmlTree::set_dt_select(Inkscape::XML::Node *repr)
{
    if (!current_desktop) {
        return;
    }

    Inkscape::Selection *selection = current_desktop->getSelection();

    SPObject *object;
    if (repr) {
        while ( ( repr->type() != Inkscape::XML::NodeType::ELEMENT_NODE )
                && repr->parent() )
        {
            repr = repr->parent();
        } // end of while loop

        object = current_desktop->getDocument()->getObjectByRepr(repr);
    } else {
        object = nullptr;
    }

    blocked++;

    if (!object || !in_dt_coordsys(*object)) {
        // object not on canvas
    } else if (isRealLayer(object)) {
        current_desktop->setCurrentLayer(object);
    } else {
        if (SP_IS_GROUP(object->parent)) {
            current_desktop->setCurrentLayer(object->parent);
        }

        selection->set(SP_ITEM(object));
    }

    current_desktop->getDocument()->setXMLDialogSelectedObject(object);

    blocked--;

} // end of set_dt_select()


void XmlTree::on_tree_select_row(GtkTreeSelection *selection, gpointer data)
{
    XmlTree *self = static_cast<XmlTree *>(data);

    if (self->blocked || !self->current_desktop) {
        return;
    }

    // Defer the update after all events have been processed. Allows skipping
    // of invalid intermediate selection states, like the automatic next row
    // selection after `gtk_tree_store_remove`.
    if (self->deferred_on_tree_select_row_id == 0) {
        self->deferred_on_tree_select_row_id = //
            g_idle_add(XmlTree::deferred_on_tree_select_row, data);
    }
}

gboolean XmlTree::deferred_on_tree_select_row(gpointer data)
{
    XmlTree *self = static_cast<XmlTree *>(data);

    self->deferred_on_tree_select_row_id = 0;

    GtkTreeIter   iter;
    GtkTreeModel *model;

    if (self->selected_repr) {
        Inkscape::GC::release(self->selected_repr);
        self->selected_repr = nullptr;
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->tree));

    if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
        // Nothing selected, update widgets
        self->propagate_tree_select(nullptr);
        self->set_dt_select(nullptr);
        self->on_tree_unselect_row_disable();
        return FALSE;
    }

    Inkscape::XML::Node *repr = sp_xmlview_tree_node_get_repr(model, &iter);
    g_assert(repr != nullptr);


    self->selected_repr = repr;
    Inkscape::GC::anchor(self->selected_repr);

    self->propagate_tree_select(self->selected_repr);

    self->set_dt_select(self->selected_repr);

    self->tree_reset_context();

    self->on_tree_select_row_enable(&iter);

    return FALSE;
}


void XmlTree::after_tree_move(SPXMLViewTree * /*tree*/, gpointer value, gpointer data)
{
    XmlTree *self = static_cast<XmlTree *>(data);
    guint val = GPOINTER_TO_UINT(value);

    if (val) {
        DocumentUndo::done(self->current_document, SP_VERB_DIALOG_XML_EDITOR,
                           Q_("Undo History / XML dialog|Drag XML subtree"));
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
    if (repr->type() == Inkscape::XML::NodeType::ELEMENT_NODE) {
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

            if (prev && (prev->type() == Inkscape::XML::NodeType::ELEMENT_NODE)) {
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
                               Q_("Undo History / XML dialog|Create new element node"));
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
                       Q_("Undo History / XML dialog|Create new text node"));

    set_tree_select(text);
    set_dt_select(text);
}

void XmlTree::cmd_duplicate_node()
{
    g_assert(selected_repr != nullptr);

    Inkscape::XML::Node *parent = selected_repr->parent();
    Inkscape::XML::Node *dup = selected_repr->duplicate(parent->document());
    parent->addChild(dup, selected_repr);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR, Q_("Undo History / XML dialog|Duplicate node"));

    GtkTreeIter node;

    if (sp_xmlview_tree_get_repr_node(SP_XMLVIEW_TREE(tree), dup, &node)) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        gtk_tree_selection_select_iter(selection, &node);
    }
}

void XmlTree::cmd_delete_node()
{
    g_assert(selected_repr != nullptr);

    current_document->setXMLDialogSelectedObject(nullptr);

    Inkscape::XML::Node *parent = selected_repr->parent();

    sp_repr_unparent(selected_repr);

    if (parent) {
        auto parentobject = current_document->getObjectByRepr(parent);
        if (parentobject) {
            parentobject->requestDisplayUpdate(SP_OBJECT_CHILD_MODIFIED_FLAG);
        }
    }

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR, Q_("Undo History / XML dialog|Delete node"));
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

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR, Q_("Undo History / XML dialog|Raise node"));

    set_tree_select(selected_repr);
    set_dt_select(selected_repr);
}



void XmlTree::cmd_lower_node()
{
    g_assert(selected_repr != nullptr);

    g_return_if_fail(selected_repr->next() != nullptr);
    Inkscape::XML::Node *parent = selected_repr->parent();

    parent->changeOrder(selected_repr, selected_repr->next());

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR, Q_("Undo History / XML dialog|Lower node"));

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
    g_return_if_fail(prev->type() == Inkscape::XML::NodeType::ELEMENT_NODE);

    Inkscape::XML::Node* ref = nullptr;
    if (prev->firstChild()) {
        for( ref = prev->firstChild() ; ref->next() ; ref = ref->next() ){};
    }

    parent->removeChild(repr);
    prev->addChild(repr, ref);

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR, Q_("Undo History / XML dialog|Indent node"));
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

    DocumentUndo::done(current_document, SP_VERB_DIALOG_XML_EDITOR, Q_("Undo History / XML dialog|Unindent node"));

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
    while (SP_IS_ITEM(child)) {
        SPObject const * const parent = child->parent;
        if (parent == nullptr) {
            g_assert(SP_IS_ROOT(child));
            if (child == &item) {
                // item is root
                return false;
            }
            return true;
        }
        child = parent;
    }
    g_assert(!SP_IS_ROOT(child));
    return false;
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
