// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_COMPOSITE_H
#define SEEN_NR_FILTER_COMPOSITE_H

/*
 * feComposite filter effect renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/filters/composite.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"

namespace Inkscape {
namespace Filters {

class FilterComposite : public FilterPrimitive {
public:
    FilterComposite();
    static FilterPrimitive *create();
    ~FilterComposite() override;

    void render_cairo(FilterSlot &) override;
    bool can_handle_affine(Geom::Affine const &) override;
    double complexity(Geom::Affine const &ctm) override;

    void set_input(int input) override;
    void set_input(int input, int slot) override;

    void set_operator(FeCompositeOperator op);
    void set_arithmetic(double k1, double k2, double k3, double k4);

    Glib::ustring name() override { return Glib::ustring("Composite"); }

private:
    FeCompositeOperator op;
    double k1, k2, k3, k4;
    int _input2;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* __NR_FILTER_COMPOSITE_H__ */
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
