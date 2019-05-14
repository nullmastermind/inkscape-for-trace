// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_RADIAL_GRADIENT_H
#define SP_RADIAL_GRADIENT_H

/** \file
 * SPRadialGradient: SVG <radialgradient> implementtion.
 */

#include "sp-gradient.h"
#include "svg/svg-length.h"

typedef struct _cairo cairo_t;
typedef struct _cairo_pattern cairo_pattern_t;

#define SP_RADIALGRADIENT(obj) (dynamic_cast<SPRadialGradient*>((SPObject*)obj))
#define SP_IS_RADIALGRADIENT(obj) (dynamic_cast<const SPRadialGradient*>((SPObject*)obj) != NULL)

/** Radial gradient. */
class SPRadialGradient : public SPGradient {
public:
    SPRadialGradient();
    ~SPRadialGradient() override;

    SVGLength cx;
    SVGLength cy;
    SVGLength r;
    SVGLength fx;
    SVGLength fy;
    SVGLength fr; // Focus radius. Added in SVG 2

    cairo_pattern_t* pattern_new(cairo_t *ct, Geom::OptRect const &bbox, double opacity) override;

protected:
    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void set(SPAttributeEnum key, char const *value) override;
    void update(SPCtx *ctx, unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif /* !SP_RADIAL_GRADIENT_H */

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
