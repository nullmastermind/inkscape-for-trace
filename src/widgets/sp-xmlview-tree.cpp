// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Specialization of GtkTreeView for the XML tree view
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2002 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>

#include "xml/node-event-vector.h"
#include "sp-xmlview-tree.h"

namespace {
struct NodeData {
	SPXMLViewTree * tree;
	GtkTreeRowReference  *rowref;
	Inkscape::XML::Node * repr;
    bool expanded = false; //< true if tree view has been expanded to this node
    bool dragging = false;

    NodeData(SPXMLViewTree *tree, GtkTreeIter *node, Inkscape::XML::Node *repr);
    ~NodeData();
};

// currently dragged node
Inkscape::XML::Node *dragging_repr = nullptr;
} // namespace

enum { STORE_TEXT_COL = 0, STORE_DATA_COL, STORE_N_COLS };

static void sp_xmlview_tree_destroy(GtkWidget * object);

static NodeData *sp_xmlview_tree_node_get_data(GtkTreeModel *model, GtkTreeIter *iter);

static void add_node(SPXMLViewTree *tree, GtkTreeIter *parent, GtkTreeIter *before, Inkscape::XML::Node *repr);

static void element_child_added (Inkscape::XML::Node * repr, Inkscape::XML::Node * child, Inkscape::XML::Node * ref, gpointer data);
static void element_attr_changed (Inkscape::XML::Node * repr, const gchar * key, const gchar * old_value, const gchar * new_value, bool is_interactive, gpointer data);
static void element_child_removed (Inkscape::XML::Node * repr, Inkscape::XML::Node * child, Inkscape::XML::Node * ref, gpointer data);
static void element_order_changed (Inkscape::XML::Node * repr, Inkscape::XML::Node * child, Inkscape::XML::Node * oldref, Inkscape::XML::Node * newref, gpointer data);
static void element_name_changed (Inkscape::XML::Node* repr, gchar const* oldname, gchar const* newname, gpointer data);
static void element_attr_or_name_change_update(Inkscape::XML::Node* repr, NodeData* data);

static void text_content_changed (Inkscape::XML::Node * repr, const gchar * old_content, const gchar * new_content, gpointer data);
static void comment_content_changed (Inkscape::XML::Node * repr, const gchar * old_content, const gchar * new_content, gpointer data);
static void pi_content_changed (Inkscape::XML::Node * repr, const gchar * old_content, const gchar * new_content, gpointer data);

static gboolean ref_to_sibling (NodeData *node, Inkscape::XML::Node * ref, GtkTreeIter *);
static gboolean repr_to_child (NodeData *node, Inkscape::XML::Node * repr, GtkTreeIter *);
static GtkTreeRowReference *tree_iter_to_ref(SPXMLViewTree *, GtkTreeIter *);
static gboolean tree_ref_to_iter (SPXMLViewTree * tree, GtkTreeIter* iter, GtkTreeRowReference  *ref);

static gboolean search_equal_func(GtkTreeModel *, gint column, const gchar *key, GtkTreeIter *, gpointer search_data);
static gboolean foreach_func(GtkTreeModel *, GtkTreePath *, GtkTreeIter *, gpointer user_data);

static void on_row_changed(GtkTreeModel *, GtkTreePath *, GtkTreeIter *, gpointer user_data);
static void on_drag_begin(GtkWidget *, GdkDragContext *, gpointer userdata);
static void on_drag_end(GtkWidget *, GdkDragContext *, gpointer userdata);
static gboolean do_drag_motion(GtkWidget *, GdkDragContext *, gint x, gint y, guint time, gpointer user_data);

static const Inkscape::XML::NodeEventVector element_repr_events = {
        element_child_added,
        element_child_removed,
        element_attr_changed,
        nullptr, /* content_changed */
        element_order_changed,
        element_name_changed
};

static const Inkscape::XML::NodeEventVector text_repr_events = {
        nullptr, /* child_added */
        nullptr, /* child_removed */
        nullptr, /* attr_changed */
        text_content_changed,
        nullptr  /* order_changed */,
        nullptr  /* element_name_changed */
};

static const Inkscape::XML::NodeEventVector comment_repr_events = {
        nullptr, /* child_added */
        nullptr, /* child_removed */
        nullptr, /* attr_changed */
        comment_content_changed,
        nullptr  /* order_changed */,
        nullptr  /* element_name_changed */
};

static const Inkscape::XML::NodeEventVector pi_repr_events = {
        nullptr, /* child_added */
        nullptr, /* child_removed */
        nullptr, /* attr_changed */
        pi_content_changed,
        nullptr  /* order_changed */,
        nullptr  /* element_name_changed */
};

/**
 * Get an iterator to the first child of `data`
 * @param data handle which references a row
 * @param[out] child_iter On success: valid iterator to first child
 * @return False if the node has no children
 */
static bool get_first_child(NodeData *data, GtkTreeIter *child_iter)
{
    GtkTreeIter iter;
    return tree_ref_to_iter(data->tree, &iter, data->rowref) &&
           gtk_tree_model_iter_children(GTK_TREE_MODEL(data->tree->store), child_iter, &iter);
}

/**
 * @param iter First dummy row on that level
 * @pre all rows on the same level are dummies
 * @pre iter is valid
 * @post iter is invalid
 * @post level is empty
 */
static void remove_dummy_rows(GtkTreeStore *store, GtkTreeIter *iter)
{
    do {
        g_assert(nullptr == sp_xmlview_tree_node_get_data(GTK_TREE_MODEL(store), iter));
        gtk_tree_store_remove(store, iter);
    } while (gtk_tree_store_iter_is_valid(store, iter));
}

static gboolean on_test_expand_row( //
    GtkTreeView *tree_view,         //
    GtkTreeIter *iter,              //
    GtkTreePath *path,              //
    gpointer)
{
    auto tree = SP_XMLVIEW_TREE(tree_view);
    auto model = GTK_TREE_MODEL(tree->store);

    GtkTreeIter childiter;
    bool has_children = gtk_tree_model_iter_children(model, &childiter, iter);
    g_assert(has_children);

    if (sp_xmlview_tree_node_get_repr(model, &childiter) == nullptr) {
        NodeData *data = sp_xmlview_tree_node_get_data(model, iter);

        remove_dummy_rows(tree->store, &childiter);

        // insert real rows
        data->expanded = true;
        sp_repr_synthesize_events(data->repr, &element_repr_events, data);
    }

    return false;
}

GtkWidget *sp_xmlview_tree_new(Inkscape::XML::Node * repr, void * /*factory*/, void * /*data*/)
{
    SPXMLViewTree *tree = SP_XMLVIEW_TREE(g_object_new (SP_TYPE_XMLVIEW_TREE, nullptr));

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(tree), FALSE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW(tree), TRUE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW(tree), TRUE);
    gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(tree), search_equal_func, nullptr, nullptr);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("", renderer, "text", STORE_TEXT_COL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    gtk_cell_renderer_set_padding (renderer, 2, 0);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    sp_xmlview_tree_set_repr (tree, repr);

    g_signal_connect(GTK_TREE_VIEW(tree), "drag-begin", G_CALLBACK(on_drag_begin), tree);
    g_signal_connect(GTK_TREE_VIEW(tree), "drag-end", G_CALLBACK(on_drag_end), tree);
    g_signal_connect(GTK_TREE_VIEW(tree), "drag-motion",  G_CALLBACK(do_drag_motion), tree);
    g_signal_connect(GTK_TREE_VIEW(tree), "test-expand-row", G_CALLBACK(on_test_expand_row), nullptr);

    return GTK_WIDGET(tree);
}

G_DEFINE_TYPE(SPXMLViewTree, sp_xmlview_tree, GTK_TYPE_TREE_VIEW);

void sp_xmlview_tree_class_init(SPXMLViewTreeClass * klass)
{
    auto widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->destroy = sp_xmlview_tree_destroy;
    
    // Signal for when a tree drag and drop has completed
    g_signal_new (  "tree_move",
        G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_FIRST,
        0,
        nullptr, nullptr,
        g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1,
        G_TYPE_UINT);
}

void
sp_xmlview_tree_init (SPXMLViewTree * tree)
{
	tree->repr = nullptr;
	tree->blocked = 0;
}

void sp_xmlview_tree_destroy(GtkWidget * object)
{
	SPXMLViewTree * tree = SP_XMLVIEW_TREE (object);

	sp_xmlview_tree_set_repr (tree, nullptr);

	GTK_WIDGET_CLASS(sp_xmlview_tree_parent_class)->destroy (object);
}

/*
 * Add a new row to the tree
 */
void
add_node (SPXMLViewTree * tree, GtkTreeIter *parent, GtkTreeIter *before, Inkscape::XML::Node * repr)
{
	const Inkscape::XML::NodeEventVector * vec;

	g_assert (tree != nullptr);

    if (before && !gtk_tree_store_iter_is_valid(tree->store, before)) {
        before = nullptr;
    }

	GtkTreeIter iter;
    gtk_tree_store_insert_before (tree->store, &iter, parent, before);

    if (!gtk_tree_store_iter_is_valid(tree->store, &iter)) {
        return;
    }

    if (!repr) {
        // no need to store any data
        return;
    }

    auto data = new NodeData(tree, &iter, repr);

    g_assert (data != nullptr);

    gtk_tree_store_set(tree->store, &iter, STORE_DATA_COL, data, -1);

	if ( repr->type() == Inkscape::XML::TEXT_NODE ) {
		vec = &text_repr_events;
	} else if ( repr->type() == Inkscape::XML::COMMENT_NODE ) {
		vec = &comment_repr_events;
	} else if ( repr->type() == Inkscape::XML::PI_NODE ) {
		vec = &pi_repr_events;
	} else if ( repr->type() == Inkscape::XML::ELEMENT_NODE ) {
		vec = &element_repr_events;
	} else {
		vec = nullptr;
	}

	if (vec) {
		/* cheat a little to get the text updated on nodes without id */
        if (repr->type() == Inkscape::XML::ELEMENT_NODE && repr->attribute("id") == nullptr) {
			element_attr_changed (repr, "id", nullptr, nullptr, false, data);
		}
		sp_repr_add_listener (repr, vec, data);
		sp_repr_synthesize_events (repr, vec, data);
	}
}

static gboolean remove_all_listeners(GtkTreeModel *model, GtkTreePath *, GtkTreeIter *iter, gpointer)
{
    NodeData *data = sp_xmlview_tree_node_get_data(model, iter);
    delete data;
    return false;
}

NodeData::NodeData(SPXMLViewTree *tree, GtkTreeIter *iter, Inkscape::XML::Node *repr)
    : tree(tree)
    , rowref(tree_iter_to_ref(tree, iter))
    , repr(repr)
{
    if (repr) {
        Inkscape::GC::anchor(repr);
    }
}

NodeData::~NodeData()
{
    if (repr) {
        sp_repr_remove_listener_by_data(repr, this);
        Inkscape::GC::release(repr);
    }
    gtk_tree_row_reference_free(rowref);
}

void element_child_added (Inkscape::XML::Node * /*repr*/, Inkscape::XML::Node * child, Inkscape::XML::Node * ref, gpointer ptr)
{
    NodeData *data = static_cast<NodeData *>(ptr);
    GtkTreeIter before;

    if (data->tree->blocked) return;

    if (!ref_to_sibling (data, ref, &before)) {
        return;
    }

    GtkTreeIter data_iter;
    tree_ref_to_iter(data->tree, &data_iter,  data->rowref);

    if (!data->expanded) {
        auto model = GTK_TREE_MODEL(data->tree->store);
        GtkTreeIter childiter;
        if (!gtk_tree_model_iter_children(model, &childiter, &data_iter)) {
            // no children yet, add a dummy
            child = nullptr;
        } else if (sp_xmlview_tree_node_get_repr(model, &childiter) == nullptr) {
            // already has a dummy child
            return;
        }
    }

    add_node (data->tree, &data_iter, &before, child);
}

void element_attr_changed(
        Inkscape::XML::Node* repr, gchar const* key,
        gchar const* /*old_value*/, gchar const* /*new_value*/, bool /*is_interactive*/,
        gpointer ptr)
{
    if (0 != strcmp (key, "id") && 0 != strcmp (key, "inkscape:label"))
        return;
    element_attr_or_name_change_update(repr, static_cast<NodeData*>(ptr));
}

void element_name_changed(
        Inkscape::XML::Node* repr,
        gchar const* /*oldname*/, gchar const* /*newname*/, gpointer ptr)
{
    element_attr_or_name_change_update(repr, static_cast<NodeData*>(ptr));
}

void element_attr_or_name_change_update(Inkscape::XML::Node* repr, NodeData* data)
{
    if (data->tree->blocked) {
        return;
    }

    gchar const* node_name = repr->name();
    gchar const* id_value = repr->attribute("id");
    gchar const* label_value = repr->attribute("inkscape:label");
    gchar* display_text;

    if (id_value && label_value) {
        display_text = g_strdup_printf ("<%s id=\"%s\" inkscape:label=\"%s\">", node_name, id_value, label_value);
    } else if (id_value) {
        display_text = g_strdup_printf ("<%s id=\"%s\">", node_name, id_value);
    } else if (label_value) {
        display_text = g_strdup_printf ("<%s inkscape:label=\"%s\">", node_name, label_value);
    } else {
        display_text = g_strdup_printf ("<%s>", node_name);
    }

    GtkTreeIter iter;
    if (tree_ref_to_iter(data->tree, &iter,  data->rowref)) {
        gtk_tree_store_set (GTK_TREE_STORE(data->tree->store), &iter, STORE_TEXT_COL, display_text, -1);
    }

    g_free(display_text);
}

void element_child_removed(Inkscape::XML::Node *repr, Inkscape::XML::Node *child, Inkscape::XML::Node * /*ref*/,
                           gpointer ptr)
{
    NodeData *data = static_cast<NodeData *>(ptr);

    if (data->tree->blocked) return;

    GtkTreeIter iter;
    if (repr_to_child(data, child, &iter)) {
        delete sp_xmlview_tree_node_get_data(GTK_TREE_MODEL(data->tree->store), &iter);
        gtk_tree_store_remove(data->tree->store, &iter);
    } else if (!repr->firstChild() && get_first_child(data, &iter)) {
        // remove dummy when all children gone
        remove_dummy_rows(data->tree->store, &iter);
    } else {
        return;
    }

#ifndef GTK_ISSUE_2510_IS_FIXED
    // https://gitlab.gnome.org/GNOME/gtk/issues/2510
    gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree)));
#endif
}

void element_order_changed(Inkscape::XML::Node * /*repr*/, Inkscape::XML::Node * child, Inkscape::XML::Node * /*oldref*/, Inkscape::XML::Node * newref, gpointer ptr)
{
    NodeData *data = static_cast<NodeData *>(ptr);
    GtkTreeIter before, node;

    if (data->tree->blocked) return;

    ref_to_sibling (data, newref, &before);
    repr_to_child (data, child, &node);

    if (gtk_tree_store_iter_is_valid(data->tree->store, &before)) {
        gtk_tree_store_move_before (data->tree->store, &node, &before);
    } else {
        repr_to_child (data, newref, &before);
        gtk_tree_store_move_after (data->tree->store, &node, &before);
    }
}

Glib::ustring sp_remove_newlines_and_tabs(Glib::ustring val)
{
    int pos;
    Glib::ustring newlinesign = "␤";
    Glib::ustring tabsign = "⇥";
    while ((pos = val.find("\r\n")) != std::string::npos) {
        val.erase(pos, 2);
        val.insert(pos, newlinesign);
    }
    while ((pos = val.find('\n')) != std::string::npos) {
        val.erase(pos, 1);
        val.insert(pos, newlinesign);
    }
    while ((pos = val.find('\t')) != std::string::npos) {
        val.erase(pos, 1);
        val.insert(pos, tabsign);
    }
    return val;
}

void text_content_changed(Inkscape::XML::Node * /*repr*/, const gchar * /*old_content*/, const gchar * new_content, gpointer ptr)
{
    NodeData *data = static_cast<NodeData *>(ptr);

    if (data->tree->blocked) return;

    gchar *label = g_strdup_printf ("\"%s\"", new_content);
    Glib::ustring nolinecontent = label;
    nolinecontent = sp_remove_newlines_and_tabs(nolinecontent);

    GtkTreeIter iter;
    if (tree_ref_to_iter(data->tree, &iter,  data->rowref)) {
        gtk_tree_store_set(GTK_TREE_STORE(data->tree->store), &iter, STORE_TEXT_COL, nolinecontent.c_str(), -1);
    }

    g_free (label);
}

void comment_content_changed(Inkscape::XML::Node * /*repr*/, const gchar * /*old_content*/, const gchar *new_content, gpointer ptr)
{
    NodeData *data = static_cast<NodeData*>(ptr);

    if (data->tree->blocked) return;

    gchar *label = g_strdup_printf ("<!--%s-->", new_content);
    Glib::ustring nolinecontent = label;
    nolinecontent = sp_remove_newlines_and_tabs(nolinecontent);

    GtkTreeIter iter;
    if (tree_ref_to_iter(data->tree, &iter,  data->rowref)) {
        gtk_tree_store_set(GTK_TREE_STORE(data->tree->store), &iter, STORE_TEXT_COL, nolinecontent.c_str(), -1);
    }
    g_free (label);
}

void pi_content_changed(Inkscape::XML::Node *repr, const gchar * /*old_content*/, const gchar *new_content, gpointer ptr)
{
    NodeData *data = static_cast<NodeData *>(ptr);

    if (data->tree->blocked) return;

    gchar *label = g_strdup_printf ("<?%s %s?>", repr->name(), new_content);
    Glib::ustring nolinecontent = label;
    nolinecontent = sp_remove_newlines_and_tabs(nolinecontent);

    GtkTreeIter iter;
    if (tree_ref_to_iter(data->tree, &iter,  data->rowref)) {
        gtk_tree_store_set(GTK_TREE_STORE(data->tree->store), &iter, STORE_TEXT_COL, nolinecontent.c_str(), -1);
    }
    g_free (label);
}

/*
 * Save the source path on drag start, will need it in on_row_changed() when moving a row
 */
void on_drag_begin(GtkWidget *, GdkDragContext *, gpointer userdata)
{
    SPXMLViewTree *tree = static_cast<SPXMLViewTree *>(userdata);
    if (!tree) {
        return;
    }

    GtkTreeModel *model = nullptr;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        NodeData *data = sp_xmlview_tree_node_get_data(model, &iter);
        if (data) {
            data->dragging = true;
            dragging_repr = data->repr;
        }
    }
}

/**
 * Finalize what happended in `on_row_changed` and clean up what was set up in `on_drag_begin`
 */
void on_drag_end(GtkWidget *, GdkDragContext *, gpointer userdata)
{
    if (!dragging_repr)
        return;

    auto tree = static_cast<SPXMLViewTree *>(userdata);
    auto selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    bool failed = false;

    GtkTreeIter iter;
    if (sp_xmlview_tree_get_repr_node(tree, dragging_repr, &iter)) {
        NodeData *data = sp_xmlview_tree_node_get_data(GTK_TREE_MODEL(tree->store), &iter);

        if (data && data->dragging) {
            // dragging flag was not cleared in `on_row_changed`, this indicates a failed drag
            data->dragging = false;
            failed = true;
        } else {
            // Reselect the dragged row
            gtk_tree_selection_select_iter(selection, &iter);
        }
    } else {
#ifndef GTK_ISSUE_2510_IS_FIXED
        // https://gitlab.gnome.org/GNOME/gtk/issues/2510
        gtk_tree_selection_unselect_all(selection);
#endif
    }

    dragging_repr = nullptr;

    if (!failed) {
        // Signal that a drag and drop has completed successfully
        g_signal_emit_by_name(G_OBJECT(tree), "tree_move", GUINT_TO_POINTER(1));
    }
}

/*
 * Main drag & drop function
 * Get the old and new paths, and change the Inkscape::XML::Node repr's
 */
void on_row_changed(GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
    NodeData *data = sp_xmlview_tree_node_get_data(tree_model, iter);

    if (!data || !data->dragging) {
        return;
    }
    data->dragging = false;

    SPXMLViewTree *tree = SP_XMLVIEW_TREE(user_data);

    gtk_tree_row_reference_free(data->rowref);
    data->rowref = tree_iter_to_ref(tree, iter);

    GtkTreeIter new_parent;
    if (!gtk_tree_model_iter_parent(tree_model, &new_parent, iter)) {
        //No parent of drop location
        return;
    }

    Inkscape::XML::Node *repr = sp_xmlview_tree_node_get_repr(tree_model, iter);
    Inkscape::XML::Node *before_repr = nullptr;

    // Find the sibling node before iter
    GtkTreeIter before_iter = *iter;
    if (gtk_tree_model_iter_previous(tree_model, &before_iter)) {
        before_repr = sp_xmlview_tree_node_get_repr(tree_model, &before_iter);
    }

    // Drop onto oneself causes assert in changeOrder() below, ignore
    if (repr == before_repr)
        return;

    auto repr_old_parent = repr->parent();
    auto repr_new_parent = sp_xmlview_tree_node_get_repr(tree_model, &new_parent);

    tree->blocked++;

    if (repr_old_parent == repr_new_parent) {
        repr_old_parent->changeOrder(repr, before_repr);
    } else {
        repr_old_parent->removeChild(repr);
        repr_new_parent->addChild(repr, before_repr);
    }

    NodeData *data_new_parent = sp_xmlview_tree_node_get_data(tree_model, &new_parent);
    if (data_new_parent && data_new_parent->expanded) {
        // Reselect the dragged row in `on_drag_end` instead of here, because of
        // https://gitlab.gnome.org/GNOME/gtk/-/issues/2510
    } else {
        // convert to dummy node
        delete data;
        gtk_tree_store_set(tree->store, iter, STORE_DATA_COL, nullptr, -1);
    }

    tree->blocked--;
}

/*
 * Set iter to ref or node data's child with the same repr or first child
 */
gboolean ref_to_sibling (NodeData *data, Inkscape::XML::Node *repr, GtkTreeIter *iter)
{
	if (repr) {
		if (!repr_to_child (data, repr, iter)) {
            return false;
        }
        gtk_tree_model_iter_next (GTK_TREE_MODEL(data->tree->store), iter);
	} else {
	    GtkTreeIter data_iter;
	    if (!tree_ref_to_iter(data->tree, &data_iter,  data->rowref)) {
	        return false;
	    }
	    gtk_tree_model_iter_children(GTK_TREE_MODEL(data->tree->store), iter, &data_iter);
	}
	return true;
}

/*
 * Set iter to the node data's child with the same repr
 */
gboolean repr_to_child (NodeData *data, Inkscape::XML::Node * repr, GtkTreeIter *iter)
{
    GtkTreeIter data_iter;
    GtkTreeModel *model = GTK_TREE_MODEL(data->tree->store);
    gboolean valid = false;

    if (!tree_ref_to_iter(data->tree, &data_iter, data->rowref)) {
        return false;
    }

    /*
     * The node we are looking for is likely to be the last one, so check it first.
     */
    gint n_children = gtk_tree_model_iter_n_children (model, &data_iter);
    if (n_children > 1) {
        valid = gtk_tree_model_iter_nth_child (model, iter, &data_iter, n_children-1);
        if (valid && sp_xmlview_tree_node_get_repr (model, iter) == repr) {
            //g_message("repr_to_child hit %d", n_children);
            return valid;
        }
    }

    valid = gtk_tree_model_iter_children(model, iter, &data_iter);
    while (valid && sp_xmlview_tree_node_get_repr (model, iter) != repr) {
        valid = gtk_tree_model_iter_next(model, iter);
	}

    return valid;
}

/*
 * Get a matching GtkTreeRowReference for a GtkTreeIter
 */
GtkTreeRowReference  *tree_iter_to_ref (SPXMLViewTree * tree, GtkTreeIter* iter)
{
    GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree->store), iter);
    GtkTreeRowReference  *ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(tree->store), path);
    gtk_tree_path_free(path);

    return ref;
}

/*
 * Get a matching GtkTreeIter for a GtkTreeRowReference
 */
gboolean tree_ref_to_iter (SPXMLViewTree * tree, GtkTreeIter* iter, GtkTreeRowReference  *ref)
{
    GtkTreePath* path = gtk_tree_row_reference_get_path(ref);
    if (!path) {
        return false;
    }
    gboolean const valid = //
        gtk_tree_model_get_iter(GTK_TREE_MODEL(tree->store), iter, path);
    gtk_tree_path_free(path);

    return valid;
}

/*
 * Disable drag and drop target on : root node and non-element nodes
 */
gboolean do_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data)
{
    GtkTreePath *path = nullptr;
    GtkTreeViewDropPosition pos;
    gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW(widget), x, y, &path, &pos);

    int action = 0;

    if (!dragging_repr) {
        goto finally;
    }

    if (path) {
        SPXMLViewTree *tree = SP_XMLVIEW_TREE(user_data);
        GtkTreeIter iter;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(tree->store), &iter, path);
        auto repr = sp_xmlview_tree_node_get_repr(GTK_TREE_MODEL(tree->store), &iter);

        bool const drop_into = pos != GTK_TREE_VIEW_DROP_BEFORE && //
                               pos != GTK_TREE_VIEW_DROP_AFTER;

        // 0. don't drop on self (also handled by on_row_changed but nice to not have drop highlight for it)
        if (repr == dragging_repr) {
            goto finally;
        }

        // 1. only xml elements can have children
        if (drop_into && repr->type() != Inkscape::XML::ELEMENT_NODE) {
            goto finally;
        }

        // 3. elements must be at least children of the root <svg:svg> element
        if (gtk_tree_path_get_depth(path) < 2) {
            goto finally;
        }

        // 4. drag node specific limitations
        {
            // nodes which can't be re-parented (because the document holds pointers to them which must stay valid)
            static GQuark const CODE_sodipodi_namedview = g_quark_from_static_string("sodipodi:namedview");
            static GQuark const CODE_svg_defs = g_quark_from_static_string("svg:defs");

            bool const no_reparenting = dragging_repr->code() == CODE_sodipodi_namedview || //
                                        dragging_repr->code() == CODE_svg_defs;

            if (no_reparenting && (drop_into || dragging_repr->parent() != repr->parent())) {
                goto finally;
            }
        }

        action = GDK_ACTION_MOVE;
    }

finally:
    if (action == 0) {
        // remove drop highlight
        gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget), nullptr, pos /* ignored */);
    }

    gtk_tree_path_free(path);
    gdk_drag_status (context, (GdkDragAction)action, time);

    return (action == 0);
}

/*
 * Set the tree selection and scroll to the row with the given repr
 */
void
sp_xmlview_tree_set_repr (SPXMLViewTree * tree, Inkscape::XML::Node * repr)
{
    if ( tree->repr == repr ) return;

    if (tree->store) {
        gtk_tree_view_set_model(GTK_TREE_VIEW(tree), nullptr);
        gtk_tree_model_foreach(GTK_TREE_MODEL(tree->store), remove_all_listeners, nullptr);
        g_object_unref(tree->store);
        tree->store = nullptr;
    }

    if (tree->repr) {
        Inkscape::GC::release(tree->repr);
    }
    tree->repr = repr;
    if (repr) {
        tree->store = gtk_tree_store_new(STORE_N_COLS, G_TYPE_STRING, G_TYPE_POINTER);

        Inkscape::GC::anchor(repr);
        add_node(tree, nullptr, nullptr, repr);

        // Set the tree model here, after all data is inserted
        gtk_tree_view_set_model (GTK_TREE_VIEW(tree), GTK_TREE_MODEL(tree->store));
        g_signal_connect(G_OBJECT(tree->store), "row-changed", G_CALLBACK(on_row_changed), tree);

        GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW(tree), path);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(tree), path, nullptr, true, 0.5, 0.0);
        gtk_tree_path_free(path);
    }
}

/*
 * Return the node data at a given GtkTreeIter position
 */
NodeData *sp_xmlview_tree_node_get_data(GtkTreeModel *model, GtkTreeIter *iter)
{
    NodeData *data = nullptr;
    gtk_tree_model_get(model, iter, STORE_DATA_COL, &data, -1);
    return data;
}

/*
 * Return the repr at a given GtkTreeIter position
 */
Inkscape::XML::Node *
sp_xmlview_tree_node_get_repr (GtkTreeModel *model, GtkTreeIter * iter)
{
    NodeData *data = sp_xmlview_tree_node_get_data(model, iter);
    return data ? data->repr : nullptr;
}

struct IterByReprData {
    const Inkscape::XML::Node *repr; //< in
    GtkTreeIter *iter;               //< out
};

/*
 * Find a GtkTreeIter position in the tree by repr
 * @return True if the node was found
 */
gboolean
sp_xmlview_tree_get_repr_node (SPXMLViewTree * tree, Inkscape::XML::Node * repr, GtkTreeIter *iter)
{
    iter->stamp = 0; // invalidate iterator
    IterByReprData funcdata = { repr, iter };
    gtk_tree_model_foreach(GTK_TREE_MODEL(tree->store), foreach_func, &funcdata);
    return iter->stamp != 0;
}

gboolean foreach_func(GtkTreeModel *model, GtkTreePath * /*path*/, GtkTreeIter *iter, gpointer user_data)
{
    auto funcdata = static_cast<IterByReprData *>(user_data);
    if (sp_xmlview_tree_node_get_repr(model, iter) == funcdata->repr) {
        *funcdata->iter = *iter;
        return TRUE;
    }

    return FALSE;
}

/*
 * Callback function for string searches in the tree
 * Return a match on any substring
 */
gboolean search_equal_func(GtkTreeModel *model, gint /*column*/, const gchar *key, GtkTreeIter *iter, gpointer /*search_data*/)
{
    gchar *text = nullptr;
    gtk_tree_model_get(model, iter, STORE_TEXT_COL, &text, -1);

    gboolean match = (strstr(text, key) != nullptr);

    g_free(text);

    return !match;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4:fileencoding=utf-8 :
