// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_GRADIENT_VECTOR_H
#define SEEN_GRADIENT_VECTOR_H

/*
 * Gradient vector selection widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2010 Jon A. Cruz
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/gradient-selector.h"

#include <gtkmm/liststore.h>
#include <sigc++/connection.h>

class SPDocument;
class SPObject;
class SPGradient;
class SPStop;

namespace Inkscape {
namespace UI {
namespace Widget {

class GradientVectorSelector : public Gtk::Box {
  private:
    bool _swatched = false;

    SPDocument *_doc = nullptr;
    SPGradient *_gr  = nullptr;

    /* Gradient vectors store */
    Glib::RefPtr<Gtk::ListStore> _store;
    Inkscape::UI::Widget::GradientSelector::ModelColumns *_columns;

    sigc::connection _gradient_release_connection;
    sigc::connection _defs_release_connection;
    sigc::connection _defs_modified_connection;
    sigc::connection _tree_select_connection;

    sigc::signal<void, SPGradient *> _signal_vector_set;

    void gradient_release(SPObject *obj);
    void defs_release(SPObject *defs);
    void defs_modified(SPObject *defs, guint flags);
    void rebuild_gui_full();

  public:
    GradientVectorSelector(SPDocument *doc, SPGradient *gradient);
    ~GradientVectorSelector() override;

    void setSwatched();
    void set_gradient(SPDocument *doc, SPGradient *gr);

    inline decltype(_columns) get_columns()  const { return _columns; }
    inline decltype(_doc)     get_document() const { return _doc; }
    inline decltype(_gr)      get_gradient() const { return _gr; }
    inline decltype(_store)   get_store()    const { return _store; }

    inline decltype(_signal_vector_set) signal_vector_set() const { return _signal_vector_set; }

    inline void set_tree_select_connection(sigc::connection &connection) { _tree_select_connection = connection; }
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

Glib::ustring gr_prepare_label (SPObject *obj);
Glib::ustring gr_ellipsize_text(Glib::ustring const &src, size_t maxlen);

#endif // SEEN_GRADIENT_VECTOR_H

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
