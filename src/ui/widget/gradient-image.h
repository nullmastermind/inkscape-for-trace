// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_GRADIENT_IMAGE_H
#define SEEN_SP_GRADIENT_IMAGE_H

/**
 * A simple gradient preview
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/refptr.h>
#include <gtkmm/widget.h>

class SPGradient;
class SPObject;
class SPStop;

namespace Gdk {
    class Pixbuf;
}

#include <sigc++/connection.h>

namespace Inkscape {
namespace UI {
namespace Widget {
class GradientImage : public Gtk::Widget {
  private:
    SPGradient *_gradient;

    sigc::connection _release_connection;
    sigc::connection _modified_connection;

    void gradient_release(SPObject *obj);
    void gradient_modified(SPObject *obj, guint flags);
    void update();
    void size_request(GtkRequisition *requisition) const;

  protected:
    void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
    void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

  public:
    GradientImage(SPGradient *gradient);
    ~GradientImage() override;

    void set_gradient(SPGradient *gr);
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

GdkPixbuf *sp_gradient_to_pixbuf (SPGradient *gr, int width, int height);
Glib::RefPtr<Gdk::Pixbuf> sp_gradient_to_pixbuf_ref (SPGradient *gr, int width, int height);
Glib::RefPtr<Gdk::Pixbuf> sp_gradstop_to_pixbuf_ref (SPStop     *gr, int width, int height);

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
