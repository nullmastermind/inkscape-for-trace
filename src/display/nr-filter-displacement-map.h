// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_DISPLACEMENT_MAP_H
#define SEEN_NR_FILTER_DISPLACEMENT_MAP_H

/*
 * feDisplacementMap filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/filters/displacementmap.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"

namespace Inkscape {
namespace Filters {

class FilterDisplacementMap : public FilterPrimitive {
public:
    FilterDisplacementMap();
    static FilterPrimitive *create();
    ~FilterDisplacementMap() override;

    void render_cairo(FilterSlot &slot) override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) override;
    double complexity(Geom::Affine const &ctm) override;

    void set_input(int slot) override;
    void set_input(int input, int slot) override;
    virtual void set_scale(double s);
    virtual void set_channel_selector(int s, FilterDisplacementMapChannelSelector channel);

    Glib::ustring name() override { return Glib::ustring("Displacement Map"); }

private:
    double scale;
    int _input2;
    unsigned Xchannel, Ychannel;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* __NR_FILTER_DISPLACEMENT_MAP_H__ */
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
