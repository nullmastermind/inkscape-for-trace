// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MARKER_SELECTOR_NEW_H
#define SEEN_SP_MARKER_SELECTOR_NEW_H

/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Maximilian Albert <maximilian.albert> (gtkmm-ification)
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

#include <sigc++/signal.h>

#include "document.h"
#include "inkscape.h"

#include "display/drawing.h"
#include "scrollprotected.h"

class SPMarker;

namespace Gtk {

class Container;
class Adjustment;
}

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * ComboBox derived class for selecting stroke markers.
 */
class MarkerComboBox : public ScrollProtected<Gtk::ComboBox> {
    using parent_type = ScrollProtected<Gtk::ComboBox>;

public:
    MarkerComboBox(gchar const *id, int loc);
    ~MarkerComboBox() override;

    void setDocument(SPDocument *);

    sigc::signal<void> changed_signal;

    void set_current(SPObject *marker);
    void set_selected(const gchar *name, gboolean retry=true);
    const gchar *get_active_marker_uri();
    bool update() { return updating; };
    gchar const *get_id() { return combo_id; };
    void update_marker_image(gchar const *mname);
    int get_loc() { return loc; };

private:


    Glib::RefPtr<Gtk::ListStore> marker_store;
    gchar const *combo_id;
    int loc;
    bool updating;
    guint markerCount;
    SPDocument *doc = nullptr;
    SPDocument *sandbox;
    Gtk::CellRendererPixbuf image_renderer;

    class MarkerColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<const gchar *> marker;   // ustring doesn't work here on windows due to unicode
        Gtk::TreeModelColumn<gboolean> stock;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> pixbuf;
        Gtk::TreeModelColumn<gboolean> history;
        Gtk::TreeModelColumn<gboolean> separator;

        MarkerColumns() {
            add(label); add(stock);  add(marker);  add(history); add(separator); add(pixbuf);
        }
    };
    MarkerColumns marker_columns;

    void init_combo();
    void set_history(Gtk::TreeModel::Row match_row);
    void sp_marker_list_from_doc(SPDocument *source,  gboolean history);
    std::vector <SPMarker*> get_marker_list (SPDocument *source);
    void add_markers (std::vector<SPMarker *> const& marker_list, SPDocument *source,  gboolean history);
    void remove_markers (gboolean history);
    SPDocument *ink_markers_preview_doc ();
    Glib::RefPtr<Gdk::Pixbuf> create_marker_image(unsigned psize, gchar const *mname,
                       SPDocument *source, Inkscape::Drawing &drawing, unsigned /*visionkey*/);

    /*
     * Callbacks for drawing the combo box
     */
    static gboolean separator_cb (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

    static void handleDefsModified(MarkerComboBox *self);

    void refreshHistory();

    sigc::connection modified_connection;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape
#endif // SEEN_SP_MARKER_SELECTOR_NEW_H

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
