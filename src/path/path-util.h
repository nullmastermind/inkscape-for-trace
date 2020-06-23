// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path utilities.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef PATH_UTIL_H
#define PATH_UTIL_H

#include <2geom/forward.h>
#include <2geom/path.h>

#include "livarot/Path.h"

#include <memory>

class SPCurve;
class SPItem;

Path *Path_for_pathvector(Geom::PathVector const &pathv);
Path *Path_for_item(SPItem *item, bool doTransformation, bool transformFull = true);
Path *Path_for_item_before_LPE(SPItem *item, bool doTransformation, bool transformFull = true);
Geom::PathVector* pathvector_for_curve(SPItem *item, SPCurve *curve, bool doTransformation, bool transformFull, Geom::Affine extraPreAffine, Geom::Affine extraPostAffine);
std::unique_ptr<SPCurve> curve_for_item(SPItem *item);
std::unique_ptr<SPCurve> curve_for_item_before_LPE(SPItem *item);
std::optional<Path::cut_position> get_nearest_position_on_Path(Path *path, Geom::Point p, unsigned seg = 0);
Geom::Point get_point_on_Path(Path *path, int piece, double t);

#endif // PATH_UTIL_H

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
