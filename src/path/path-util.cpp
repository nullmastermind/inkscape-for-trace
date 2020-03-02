// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path utilities.
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
#endif

#include <vector>

#include "path-util.h"

#include "text-editing.h"

#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "object/sp-flowtext.h"
#include "object/sp-image.h"
#include "object/sp-marker.h"
#include "object/sp-path.h"
#include "object/sp-text.h"

// derived from Path_for_item
Path *
Path_for_pathvector(Geom::PathVector const &epathv)
{
    /*std::cout << "converting to Livarot path" << std::endl;

    Geom::SVGPathWriter wr;
    wr.feed(epathv);
    std::cout << wr.str() << std::endl;*/

    Path *dest = new Path;
    dest->LoadPathVector(epathv);
    return dest;
}

Path *
Path_for_item(SPItem *item, bool doTransformation, bool transformFull)
{
    SPCurve *curve = curve_for_item(item);

    if (curve == nullptr)
        return nullptr;

    Geom::PathVector *pathv = pathvector_for_curve(item, curve, doTransformation, transformFull, Geom::identity(), Geom::identity());
    curve->unref();

    /*std::cout << "converting to Livarot path" << std::endl;

    Geom::SVGPathWriter wr;
    if (pathv) {
        wr.feed(*pathv);
    }
    std::cout << wr.str() << std::endl;*/

    Path *dest = new Path;
    dest->LoadPathVector(*pathv);    
    delete pathv;

    /*gchar *str = dest->svg_dump_path();
    std::cout << "After conversion:\n" << str << std::endl;
    g_free(str);*/

    return dest;
}

/**
 * Obtains an item's Path before the LPE stack has been applied.
 */
Path *
Path_for_item_before_LPE(SPItem *item, bool doTransformation, bool transformFull)
{
    SPCurve *curve = curve_for_item_before_LPE(item);

    if (curve == nullptr)
        return nullptr;
    
    Geom::PathVector *pathv = pathvector_for_curve(item, curve, doTransformation, transformFull, Geom::identity(), Geom::identity());
    curve->unref();
    
    Path *dest = new Path;
    dest->LoadPathVector(*pathv);
    delete pathv;

    return dest;
}

/* 
 * NOTE: Returns empty pathvector if curve == NULL
 * TODO: see if calling this method can be optimized. All the pathvector copying might be slow.
 */
Geom::PathVector*
pathvector_for_curve(SPItem *item, SPCurve *curve, bool doTransformation, bool transformFull, Geom::Affine extraPreAffine, Geom::Affine extraPostAffine)
{
    if (curve == nullptr)
        return nullptr;

    Geom::PathVector *dest = new Geom::PathVector;    
    *dest = curve->get_pathvector(); // Make a copy; must be freed by the caller!
    
    if (doTransformation) {
        if (transformFull) {
            *dest *= extraPreAffine * item->i2doc_affine() * extraPostAffine;
        } else {
            *dest *= extraPreAffine * (Geom::Affine)item->transform * extraPostAffine;
        }
    } else {
        *dest *= extraPreAffine * extraPostAffine;
    }

    return dest;
}

/**
 * Obtains an item's curve. For SPPath, it is the path *before* LPE. For SPShapes other than path, it is the path *after* LPE.
 * So the result is somewhat ill-defined, and probably this method should not be used... See curve_for_item_before_LPE.
 */
SPCurve* curve_for_item(SPItem *item)
{
    if (!item) 
        return nullptr;
    
    SPCurve *curve = nullptr;
    if (SP_IS_SHAPE(item)) {
        if (SP_IS_PATH(item)) {
            curve = SP_PATH(item)->getCurveForEdit();
        } else {
            curve = SP_SHAPE(item)->getCurve();
        }
    }
    else if (SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item))
    {
        curve = te_get_layout(item)->convertToCurves();
    }
    else if (SP_IS_IMAGE(item))
    {
    curve = SP_IMAGE(item)->get_curve();
    }
    
    return curve; // do not forget to unref the curve at some point!
}

/**
 * Obtains an item's curve *before* LPE.
 * The returned SPCurve should be unreffed by the caller.
 */
SPCurve* curve_for_item_before_LPE(SPItem *item)
{
    if (!item) 
        return nullptr;
    
    SPCurve *curve = nullptr;
    if (SP_IS_SHAPE(item)) {
        curve = SP_SHAPE(item)->getCurveForEdit();
    }
    else if (SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item))
    {
        curve = te_get_layout(item)->convertToCurves();
    }
    else if (SP_IS_IMAGE(item))
    {
        curve = SP_IMAGE(item)->get_curve();
    }
    
    return curve; // do not forget to unref the curve at some point!
}

boost::optional<Path::cut_position> get_nearest_position_on_Path(Path *path, Geom::Point p, unsigned seg)
{
    //get nearest position on path
    Path::cut_position pos = path->PointToCurvilignPosition(p, seg);
    return pos;
}

Geom::Point get_point_on_Path(Path *path, int piece, double t)
{
    Geom::Point p;
    path->PointAt(piece, t, p);
    return p;
}


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
