// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_FLOOD_H
#define SEEN_NR_FILTER_FLOOD_H

/*
 * feFlood filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "display/nr-filter-primitive.h"

struct SVGICCColor;
typedef unsigned int guint32;

namespace Inkscape {
namespace Filters {

class FilterFlood : public FilterPrimitive {
public:
    FilterFlood();
    static FilterPrimitive *create();
    ~FilterFlood() override;

    void render_cairo(FilterSlot &slot) override;
    bool can_handle_affine(Geom::Affine const &) override;
    double complexity(Geom::Affine const &ctm) override;
    bool uses_background() override { return false; }
    
    virtual void set_opacity(double o);
    virtual void set_color(guint32 c);
    virtual void set_icc(SVGICCColor *icc_color);

    Glib::ustring name() override { return Glib::ustring("Flood"); }

private:
    double opacity;
    guint32 color;
    SVGICCColor *icc;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* __NR_FILTER_FLOOD_H__ */
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
