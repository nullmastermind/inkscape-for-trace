/*
 * Inkscape Widget Utilities
 *
 * Authors:
 *   Bryce W. Harrington <brycehar@bryceharrington.org>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2003 Bryce W. Harrington
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <cstring>
#include <string>

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/grid.h>

#include "selection.h"

#include "spw-utilities.h"

/**
 * Creates a label widget with the given text, at the given col, row
 * position in the table.
 */
Gtk::Label * spw_label(Gtk::Grid *table, const gchar *label_text, int col, int row, Gtk::Widget* target)
{
  Gtk::Label *label_widget = new Gtk::Label();
  g_assert(label_widget != nullptr);
  if (target != nullptr)
  {
    label_widget->set_text_with_mnemonic(label_text);
	label_widget->set_mnemonic_widget(*target);
  }
  else
  {
    label_widget->set_text(label_text);
  }
  label_widget->show();

  label_widget->set_hexpand();
  label_widget->set_halign(Gtk::ALIGN_FILL);
  label_widget->set_valign(Gtk::ALIGN_CENTER);

  #if GTK_CHECK_VERSION(3,12,0)
  label_widget->set_margin_start(4);
  label_widget->set_margin_end(4);
  #else
  label_widget->set_margin_left(4);
  label_widget->set_margin_right(4);
  #endif

  table->attach(*label_widget, col, row, 1, 1);

  return label_widget;
}

/**
 * Creates a horizontal layout manager with 4-pixel spacing between children
 * and space for 'width' columns.
 */
Gtk::HBox * spw_hbox(Gtk::Grid * table, int width, int col, int row)
{
  /* Create a new hbox with a 4-pixel spacing between children */
  Gtk::HBox *hb = new Gtk::HBox(false, 4);
  g_assert(hb != nullptr);
  hb->show();
  hb->set_hexpand();
  hb->set_halign(Gtk::ALIGN_FILL);
  hb->set_valign(Gtk::ALIGN_CENTER);
  table->attach(*hb, col, row, width, 1);

  return hb;
}

static void
sp_set_font_size_recursive (GtkWidget *w, gpointer font)
{
	guint size = GPOINTER_TO_UINT (font);

        auto css_provider = gtk_css_provider_new();

        const double pt_size = size / static_cast<double>(PANGO_SCALE);
        std::ostringstream css_data;
        css_data << "GtkWidget {\n"
                 << "  font-size: " << pt_size << "pt;\n"
                 << "}\n";

        gtk_css_provider_load_from_data(css_provider,
                                        css_data.str().c_str(),
                                        -1, nullptr);

        auto style_context = gtk_widget_get_style_context(w);
        gtk_style_context_add_provider(style_context,
                                       GTK_STYLE_PROVIDER(css_provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_USER);

	if (GTK_IS_CONTAINER(w)) {
		gtk_container_foreach (GTK_CONTAINER(w), (GtkCallback) sp_set_font_size_recursive, font);
	}

        g_object_unref(css_provider);
}

void
sp_set_font_size (GtkWidget *w, guint font)
{
	sp_set_font_size_recursive (w, GUINT_TO_POINTER(font));
}

void
sp_set_font_size_smaller (GtkWidget *w)
{
	PangoContext *pc = gtk_widget_get_pango_context (w);
	PangoFontDescription* pfd = pango_context_get_font_description (pc);
	guint size = pango_font_description_get_size (pfd);
	sp_set_font_size_recursive (w, GUINT_TO_POINTER((int) (0.8*size)));
}

/**
 * Finds the descendant of w which has the data with the given key and returns the data, or NULL if there's none.
 */
gpointer sp_search_by_data_recursive(GtkWidget *w, gpointer key)
{
	gpointer r = nullptr;

	if (w && G_IS_OBJECT(w)) {
		r = g_object_get_data(G_OBJECT(w), (gchar *) key);
	}
	if (r) return r;

	if (GTK_IS_CONTAINER(w)) {
            std::vector<Gtk::Widget*> children = Glib::wrap(GTK_CONTAINER(w))->get_children();
		for (auto i:children) {
			r = sp_search_by_data_recursive(GTK_WIDGET(i->gobj()), key);
			if (r) return r;
		}
	}

	return nullptr;
}

/**
 * Returns the descendant of w which has the given key and value pair, or NULL if there's none.
 */
GtkWidget *sp_search_by_value_recursive(GtkWidget *w, gchar *key, gchar *value)
{
	gchar *r = nullptr;

	if (w && G_IS_OBJECT(w)) {
		r = (gchar *) g_object_get_data(G_OBJECT(w), key);
	}
	if (r && !strcmp (r, value)) return w;

	if (GTK_IS_CONTAINER(w)) {
                std::vector<Gtk::Widget*> children = Glib::wrap(GTK_CONTAINER(w))->get_children();
		for (auto i:children) {
			GtkWidget *child = sp_search_by_value_recursive(GTK_WIDGET(i->gobj()), key, value);
			if (child) return child;
		}
	}

	return nullptr;
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
