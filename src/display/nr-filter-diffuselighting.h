// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_DIFFUSELIGHTING_H
#define SEEN_NR_FILTER_DIFFUSELIGHTING_H

/*
 * feDiffuseLighting renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "display/nr-light-types.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"

class SPFeDistantLight;
class SPFePointLight;
class SPFeSpotLight;
struct SVGICCColor;
typedef unsigned int guint32;

namespace Inkscape {
namespace Filters {

class FilterDiffuseLighting : public FilterPrimitive {
public:
    FilterDiffuseLighting();
    static FilterPrimitive *create();
    ~FilterDiffuseLighting() override;
    void render_cairo(FilterSlot &slot) override;
    virtual void set_icc(SVGICCColor *icc_color);
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) override;
    double complexity(Geom::Affine const &ctm) override;

    union {
        SPFeDistantLight *distant;
        SPFePointLight *point;
        SPFeSpotLight *spot;
    } light;
    LightType light_type;
    double diffuseConstant;
    double surfaceScale;
    guint32 lighting_color;

    Glib::ustring name() override { return Glib::ustring("Diffuse Lighting"); }

private:
    SVGICCColor *icc;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* SEEN_NR_FILTER_DIFFUSELIGHTING_H */
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
