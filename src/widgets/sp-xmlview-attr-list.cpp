/*
 * Specialization of GtkTreeView for the XML tree view
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2002 MenTaLguY
 *
 * Released under the GNU GPL; see COPYING for details
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <glibmm/i18n.h>

#include "helper/sp-marshal.h"
#include "../xml/node-event-vector.h"
#include "sp-xmlview-attr-list.h"

static void sp_xmlview_attr_list_destroy(GtkWidget * object);

static void event_attr_changed (Inkscape::XML::Node * repr, const gchar * name, const gchar * old_value, const gchar * new_value, bool is_interactive, gpointer data);

static Inkscape::XML::NodeEventVector repr_events = {
    nullptr, /* child_added */
    nullptr, /* child_removed */
    event_attr_changed,
    nullptr, /* content_changed */
    nullptr  /* order_changed */
};

GtkWidget *sp_xmlview_attr_list_new (Inkscape::XML::Node * repr)
{
    SPXMLViewAttrList * attr_list = SP_XMLVIEW_ATTR_LIST(g_object_new(SP_TYPE_XMLVIEW_ATTR_LIST, nullptr));

    attr_list->store = gtk_list_store_new (ATTR_N_COLS, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW(attr_list), GTK_TREE_MODEL(attr_list->store));

    // Attribute name column
    int colpos = 0;
    GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(attr_list), colpos, _("Name"), cell, "text", ATTR_COL_NAME, NULL);
    GtkTreeViewColumn *column = gtk_tree_view_get_column (GTK_TREE_VIEW(attr_list), colpos);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    gtk_tree_sortable_set_sort_func( GTK_TREE_SORTABLE(attr_list->store), ATTR_COL_NAME, attr_sort_name_iter, GINT_TO_POINTER(ATTR_COL_NAME), NULL);
    gtk_tree_sortable_set_sort_column_id ( GTK_TREE_SORTABLE(attr_list->store), ATTR_COL_NAME, GTK_SORT_ASCENDING);
    gtk_cell_renderer_set_padding (cell, 2, 0);
    g_object_set(cell, "editable", TRUE, NULL);
    g_signal_connect(cell, "edited", (GCallback) attr_name_edited, attr_list);

    // Attribute value column
    colpos = 1;
    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(attr_list), colpos, _("Value"), cell, "text", ATTR_COL_VALUE, NULL);
    column = gtk_tree_view_get_column (GTK_TREE_VIEW(attr_list), colpos);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_cell_renderer_set_padding (cell, 2, 0);
    g_object_set(cell, "editable", TRUE, NULL);
    g_signal_connect(cell, "edited", (GCallback) attr_value_edited, attr_list);

    // Final listview settings
    g_signal_connect( G_OBJECT(attr_list), "key-press-event", G_CALLBACK(attr_key_pressed), NULL);
    gtk_widget_set_name(GTK_WIDGET(attr_list), "attrlist");
    sp_xmlview_attr_list_set_repr (attr_list, repr);

    return GTK_WIDGET(attr_list);
}

gint
attr_sort_name_iter (GtkTreeModel *model, GtkTreeIter *iter_a, GtkTreeIter  *iter_b, gpointer /*data*/)
{
    gint ret;
    gchar *name1, *name2;

    gtk_tree_model_get(model, iter_a, ATTR_COL_NAME, &name1, -1);
    gtk_tree_model_get(model, iter_b, ATTR_COL_NAME, &name2, -1);
    if(name1[0] == 0) { return 1; }
    if(name2[0] == 0) { return -1; }
    ret = g_utf8_collate(name1, name2);
    g_free(name1);
    g_free(name2);
    return ret;
}

void
sp_xmlview_attr_list_set_repr (SPXMLViewAttrList * list, Inkscape::XML::Node * repr)
{
    if ( repr == list->repr ) return;
        if (list->repr) {
            gtk_list_store_clear(list->store);
            sp_repr_remove_listener_by_data (list->repr, list);
            Inkscape::GC::release(list->repr);
        }
        list->repr = repr;
        if (repr) {
            Inkscape::GC::anchor(repr);
            sp_repr_add_listener (repr, &repr_events, list);
            sp_repr_synthesize_events (repr, &repr_events, list);

            // Create an 'add-new' row to create new rows
            GtkTreeIter iter;
            gtk_list_store_append (list->store, &iter);
            gtk_list_store_set (list->store, &iter, ATTR_COL_NAME, "", ATTR_COL_VALUE, "", ATTR_COL_ATTR, NULL, -1);
        }
}

G_DEFINE_TYPE(SPXMLViewAttrList, sp_xmlview_attr_list, GTK_TYPE_TREE_VIEW);

void sp_xmlview_attr_list_class_init (SPXMLViewAttrListClass * klass)
{
    auto widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->destroy = sp_xmlview_attr_list_destroy;

        g_signal_new("row-value-changed",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (SPXMLViewAttrListClass, row_changed),
                      nullptr, nullptr,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);
        g_signal_new("attr-value-edited",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (SPXMLViewAttrListClass, row_changed),
                      nullptr, nullptr,
                      sp_marshal_VOID__STRING_STRING,
                      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}

void
sp_xmlview_attr_list_init (SPXMLViewAttrList * list)
{
    list->store = nullptr;
    list->repr = nullptr;
}

void sp_xmlview_attr_list_destroy(GtkWidget * object)
{
    SPXMLViewAttrList * list;

    list = SP_XMLVIEW_ATTR_LIST (object);

    g_object_unref(list->store);
    sp_xmlview_attr_list_set_repr (list, nullptr);

    GTK_WIDGET_CLASS(sp_xmlview_attr_list_parent_class)->destroy (object);
}

void sp_xmlview_attr_list_select_row_by_key(SPXMLViewAttrList * list, const gchar *name)
{
    GtkTreeIter iter;
    gboolean match = false;
    gboolean valid = gtk_tree_model_get_iter_first( GTK_TREE_MODEL(list->store), &iter );
    while ( valid ) {
        gchar *n = nullptr;
        gtk_tree_model_get (GTK_TREE_MODEL(list->store), &iter, ATTR_COL_NAME, &n, -1);
        if (!strcmp(n, name)) {
            match = true;
            break;
        }
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(list->store), &iter);
        // cppcheck-suppress nullPointer  // a string was copied in n by gtk_tree_model_get
        if (n) {
            g_free(n);
        }
    }

    if (match) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(list));
        gtk_tree_selection_select_iter(selection, &iter);
    }
}

void
attr_name_edited (GtkCellRendererText *cell,
                  gchar * path_string,
                  gchar * new_name,
                  gpointer data) {

    SPXMLViewAttrList * list = SP_XMLVIEW_ATTR_LIST (data);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL(list->store), &iter, path_string );
    if(valid) {
        gchar *old_name = nullptr;
        gchar *value = nullptr;
        gtk_tree_model_get (GTK_TREE_MODEL(list->store), &iter, ATTR_COL_NAME, &old_name, ATTR_COL_VALUE, &value, -1);
        if(old_name[0] != 0) {
            // Remove named value
            g_signal_emit_by_name(G_OBJECT (list), "attr-value-edited", old_name, nullptr );
        }
        g_signal_emit_by_name(G_OBJECT (list), "attr-value-edited", new_name, value );
    }
}

void
attr_value_edited (GtkCellRendererText *cell,
                   gchar * path_string,
                   gchar * new_value,
                   gpointer data) {

    SPXMLViewAttrList * list = SP_XMLVIEW_ATTR_LIST (data);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL(list->store), &iter, path_string );
    if(valid) {
        gchar *name = nullptr;
        gtk_tree_model_get (GTK_TREE_MODEL(list->store), &iter, ATTR_COL_NAME, &name, -1);
        if(name[0] == 0 or name[0] == '<') { return; }
        g_signal_emit_by_name(G_OBJECT (list), "attr-value-edited", name, new_value );
    }
}

gboolean
attr_key_pressed(GtkWidget *attributes, GdkEventKey *event, gpointer /*data*/)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(attributes));
    GtkTreeIter   iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
        switch (event->keyval)
        {
            case GDK_KEY_Delete:
            case GDK_KEY_KP_Delete:
                gchar *name = nullptr;
                gtk_tree_model_get (model, &iter, 0, &name, -1);
                g_signal_emit_by_name(G_OBJECT (attributes), "attr-value-edited", name, nullptr);
                if(name) g_free(name);
                return true;
        }
    }
    return false;
}

void
event_attr_changed (Inkscape::XML::Node * /*repr*/,
                    const gchar * name,
                    const gchar * /*old_value*/,
                    const gchar * new_value,
                    bool /*is_interactive*/,
                    gpointer data)
{
    gint row = -1;
    SPXMLViewAttrList * list = SP_XMLVIEW_ATTR_LIST (data);

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first( GTK_TREE_MODEL(list->store), &iter );
    gboolean match = false;
    while ( valid ) {
        gchar *n = nullptr;
        gtk_tree_model_get (GTK_TREE_MODEL(list->store), &iter, ATTR_COL_NAME, &n, -1);
        if (!strcmp(n, name)) {
            match = true;
            break;
        }
        row++;
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(list->store), &iter);
        // cppcheck-suppress nullPointer  // a string was copied in n by gtk_tree_model_get
        if (n) {
            g_free(n);
        }
    }

    if (match) {
        if (new_value) {
            gtk_list_store_set (list->store, &iter, ATTR_COL_NAME, name, ATTR_COL_VALUE, new_value, ATTR_COL_ATTR, g_quark_from_string (name), -1);
        } else {
            gtk_list_store_remove  (list->store, &iter);
        }
    } else if (new_value != nullptr) {
        gtk_list_store_append (list->store, &iter);
        gtk_list_store_set (list->store, &iter, ATTR_COL_NAME, name, ATTR_COL_VALUE, new_value, ATTR_COL_ATTR, g_quark_from_string (name), -1);
    }

    // send a "changed" signal so widget owners will know I've updated
    g_signal_emit_by_name(G_OBJECT (list), "row-value-changed", name );
}

