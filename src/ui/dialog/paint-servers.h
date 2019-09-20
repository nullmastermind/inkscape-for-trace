// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Paint Servers dialog
 */
/* Authors:
 *   Valentin Ionita
 *
 * Copyright (C) 2019 Valentin Ionita
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_PAINT_SERVERS_H
#define INKSCAPE_UI_DIALOG_PAINT_SERVERS_H

#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "display/drawing.h"
#include "ui/widget/panel.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

class PaintServersColumns; // For Gtk::ListStore

/**
 * This dialog serves as a preview for different types of paint servers,
 * currently only predefined. It can set the fill or stroke of the selected
 * object to the to the paint server you select.
 *
 * Patterns and hatches are loaded from the preferences paths and displayed
 * for each document, for all documents and for the current document.
 */

class PaintServersDialog : public Inkscape::UI::Widget::Panel {

public:
    PaintServersDialog(gchar const *prefsPath = "/dialogs/paint");
    ~PaintServersDialog() override;

    static PaintServersDialog &getInstance() { return *new PaintServersDialog(); };
    PaintServersDialog(PaintServersDialog const &) = delete;
    PaintServersDialog &operator=(PaintServersDialog const &) = delete;

  private:
    static PaintServersColumns *getColumns();
    void load_sources();
    void load_document(SPDocument *document);
    void load_current_document(SPObject *, guint);
    Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(SPDocument *, Glib::ustring, Glib::ustring *);
    void on_target_changed();
    void on_document_changed();
    void on_item_activated(const Gtk::TreeModel::Path &path);
    std::vector<SPObject *> extract_elements(SPObject *item);

    const Glib::ustring ALLDOCS;
    const Glib::ustring CURRENTDOC;
    std::map<Glib::ustring, Glib::RefPtr<Gtk::ListStore>> store;
    Glib::ustring current_store;
    std::map<Glib::ustring, SPDocument *> document_map;
    SPDocument *preview_document;
    Inkscape::Drawing renderDrawing;
    Gtk::ComboBoxText *dropdown;
    Gtk::IconView *icon_view;
    SPDesktop *desktop;
    Gtk::ComboBoxText *target_dropdown;
    bool target_selected;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // SEEN INKSCAPE_UI_DIALOG_PAINT_SERVERS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
