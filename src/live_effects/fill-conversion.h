// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_FILLCONVERSION_H
#define INKSCAPE_FILLCONVERSION_H

/*
 * Copyright (C) Liam P White 2020
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

class SPShape;

namespace Inkscape {
namespace LivePathEffect {

/**
 * Prepares a SPShape's fill and stroke for use in a path effect by setting
 * the existing stroke properties into the shape's fill, and setting the
 * existing fill properties into a new linked fill item which follows the
 * old shape.
 */
void lpe_shape_convert_stroke_and_fill(SPShape *shape);

/**
 * Applies the fill of the SPShape to its stroke, sets the stroke width to the
 * provided parameter, and potentially removes a linked fill, copying its
 * properties to the fill of the shape. Essentially undoes the result of the
 * above function.
 */
void lpe_shape_revert_stroke_and_fill(SPShape *shape, double width);

}
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
