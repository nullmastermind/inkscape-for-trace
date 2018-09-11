// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_SPECULARLIGHTING_H
#define SEEN_NR_FILTER_SPECULARLIGHTING_H

/*
 * feSpecularLighting renderer
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

class SPFeDistantLight;
class SPFePointLight;
class SPFeSpotLight;
struct SVGICCColor;
typedef unsigned int guint32;

namespace Inkscape {
namespace Filters {

class FilterSlot;

class FilterSpecularLighting : public FilterPrimitive {
public:
    FilterSpecularLighting();
    static FilterPrimitive *create();
    ~FilterSpecularLighting() override;

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
    double surfaceScale;
    double specularConstant;
    double specularExponent;
    guint32 lighting_color;

    Glib::ustring name() override { return Glib::ustring("Specular Lighting"); }

private:
    SVGICCColor *icc;
};

} /* namespace Filters */
} /* namespace Inkscape */

#endif /* __NR_FILTER_SPECULARLIGHTING_H__ */
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
