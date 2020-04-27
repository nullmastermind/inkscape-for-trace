// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Perspective line for 3D perspectives
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "perspective-line.h"

namespace Box3D {

PerspectiveLine::PerspectiveLine (Geom::Point const &pt, Proj::Axis const axis, Persp3D const *persp) :
        Line (pt, persp->get_VP(axis).affine(), true)
{
    g_assert (persp != nullptr);

    if (!persp->get_VP(axis).is_finite()) {
        Proj::Pt2 vp(persp->get_VP(axis));
        this->set_direction(Geom::Point(vp[Proj::X], vp[Proj::Y]));
    }
    this->vp_dir = axis;
    this->persp  = persp;
}

} // namespace Box3D 
 
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
