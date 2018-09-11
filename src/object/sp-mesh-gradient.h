// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SP_MESH_GRADIENT_H
#define SP_MESH_GRADIENT_H

/** \file
 * SPMeshGradient: SVG <meshgradient> implementation.
 */

#include "svg/svg-length.h"
#include "sp-gradient.h"

#define SP_MESHGRADIENT(obj) (dynamic_cast<SPMeshGradient*>((SPObject*)obj))
#define SP_IS_MESHGRADIENT(obj) (dynamic_cast<const SPMeshGradient*>((SPObject*)obj) != NULL)

/** Mesh gradient. */
class SPMeshGradient : public SPGradient {
public:
    SPMeshGradient();
    ~SPMeshGradient() override;

    SVGLength x;  // Upper left corner of meshgradient
    SVGLength y;  // Upper right corner of mesh
    SPMeshType type;
    bool type_set;
    cairo_pattern_t* pattern_new(cairo_t *ct, Geom::OptRect const &bbox, double opacity) override;

protected:
    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void set(SPAttributeEnum key, char const *value) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif /* !SP_MESH_GRADIENT_H */

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
