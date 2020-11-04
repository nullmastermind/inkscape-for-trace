// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 *
 * Two related object to path operations:
 *
 * 1. Find a path that includes fill, stroke, and markers. Useful for finding a visual bounding box.
 * 2. Take a set of objects and find an identical visual representation using only paths.
 *
 * Copyright (C) 2020 Tavmjong Bah
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Code moved from splivarot.cpp
 *
 */

#include "path-outline.h"

#include <vector>

#include "path-chemistry.h" // Should be moved to path directory
#include "message-stack.h"  // Should be removed.
#include "selection.h"
#include "style.h"

#include "display/curve.h"  // Should be moved to path directory

#include "helper/geom.h"    // pathv_to_linear_and_cubic()

#include "livarot/LivarotDefs.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "object/sp-item.h"
#include "object/sp-marker.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"

#include "svg/svg.h"

/**
 * Given an item, find a path representing the fill and a path representing the stroke.
 * Returns true if fill path found. Item may not have a stroke in which case stroke path is empty.
 * bbox_only==true skips cleaning up the stroke path.
 * Encapsulates use of livarot.
 */
bool
item_find_paths(const SPItem *item, Geom::PathVector& fill, Geom::PathVector& stroke, bool bbox_only = false)
{
    const SPShape *shape = dynamic_cast<const SPShape*>(item);
    const SPText  *text  = dynamic_cast<const SPText*>(item);

    if (!shape && !text) {
        return false;
    }

    std::unique_ptr<SPCurve> curve;
    if (shape) {
        curve = SPCurve::copy(shape->curve());
    } else if (text) {
        curve = text->getNormalizedBpath();
    } else {
        std::cerr << "item_find_paths: item not shape or text!" << std::endl;
        return false;
    }

    if (!curve) {
        std::cerr << "item_find_paths: no curve!" << std::endl;
        return false;
    }

    if (curve->get_pathvector().empty()) {
        std::cerr << "item_find_paths: curve empty!" << std::endl;
        return false;
    }

    fill = curve->get_pathvector();

    if (!item->style) {
        // Should never happen
        std::cerr << "item_find_paths: item with no style!" << std::endl;
        return false;
    }

    if (item->style->stroke.isNone()) {
        // No stroke, no chocolate!
        return true;
    }

    // Now that we have a valid curve with stroke, do offset. We use Livarot for this as
    // lib2geom does not yet handle offsets correctly.

    // Livarot's outline of arcs is broken. So convert the path to linear and cubics only, for
    // which the outline is created correctly.
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers( fill );

    SPStyle *style = item->style;

    double stroke_width = style->stroke_width.computed;
    if (stroke_width < Geom::EPSILON) {
        // https://bugs.launchpad.net/inkscape/+bug/1244861
        stroke_width = Geom::EPSILON;
    }
    double miter = style->stroke_miterlimit.value * stroke_width;

    JoinType join;
    switch (style->stroke_linejoin.computed) {
        case SP_STROKE_LINEJOIN_MITER:
            join = join_pointy;
            break;
        case SP_STROKE_LINEJOIN_ROUND:
            join = join_round;
            break;
        default:
            join = join_straight;
            break;
    }

    ButtType butt;
    switch (style->stroke_linecap.computed) {
        case SP_STROKE_LINECAP_SQUARE:
            butt = butt_square;
            break;
        case SP_STROKE_LINECAP_ROUND:
            butt = butt_round;
            break;
        default:
            butt = butt_straight;
            break;
    }

    Path *origin = new Path; // Fill
    Path *offset = new Path;

    Geom::Affine const transform(item->transform);
    double const scale = transform.descrim();

    origin->LoadPathVector(pathv);
    offset->SetBackData(false);

    if (!style->stroke_dasharray.values.empty()) {
        // We have dashes!
        origin->ConvertWithBackData(0.005); // Approximate by polyline
        origin->DashPolylineFromStyle(style, scale, 0);
        auto bounds = Geom::bounds_fast(pathv);
        if (bounds) {
            double size = Geom::L2(bounds->dimensions());
            origin->Simplify(size * 0.000005); // Polylines to Beziers
        }
    }

    // Finally do offset!
    origin->Outline(offset, 0.5 * stroke_width, join, butt, 0.5 * miter);

    if (bbox_only) {
        stroke = offset->MakePathVector();
    } else {
        // Clean-up shape

        offset->ConvertWithBackData(1.0); // Approximate by polyline

        Shape *theShape  = new Shape;
        offset->Fill(theShape, 0); // Convert polyline to shape, step 1.

        Shape *theOffset = new Shape;
        theOffset->ConvertToShape(theShape, fill_positive); // Create an intersection free polygon (theOffset), step2.
        theOffset->ConvertToForme(origin, 1, &offset); // Turn shape into contour (stored in origin).

        stroke = origin->MakePathVector(); // Note origin was replaced above by stroke!
    }

    delete origin;
    delete offset;

    // std::cout << "    fill:   " << sp_svg_write_path(fill)   << "  count: " << fill.curveCount() << std::endl;
    // std::cout << "    stroke: " << sp_svg_write_path(stroke) << "  count: " << stroke.curveCount() << std::endl;
    return true;
}


// ======================== Item to Outline ===================== //

static
void item_to_outline_add_marker_child( SPItem const *item, Geom::Affine marker_transform, Geom::PathVector* pathv_in )
{
    Geom::Affine tr(marker_transform);
    tr = item->transform * tr;

    // note: a marker child item can be an item group!
    if (SP_IS_GROUP(item)) {
        // recurse through all childs:
        for (auto& o: item->children) {
            if ( SP_IS_ITEM(&o) ) {
                item_to_outline_add_marker_child(SP_ITEM(&o), tr, pathv_in);
            }
        }
    } else {
        Geom::PathVector* marker_pathv = item_to_outline(item);

        if (marker_pathv) {
            for (const auto & j : *marker_pathv) {
                pathv_in->push_back(j * tr);
            }
            delete marker_pathv;
        }
    }
}

static
void item_to_outline_add_marker( SPObject const *marker_object, Geom::Affine marker_transform,
                              Geom::Scale stroke_scale, Geom::PathVector* pathv_in )
{
    SPMarker const * marker = SP_MARKER(marker_object);

    Geom::Affine tr(marker_transform);
    if (marker->markerUnits == SP_MARKER_UNITS_STROKEWIDTH) {
        tr = stroke_scale * tr;
    }
    // total marker transform
    tr = marker->c2p * tr;

    SPItem const * marker_item = sp_item_first_item_child(marker_object); // why only consider the first item? can a marker only consist of a single item (that may be a group)?
    if (marker_item) {
        item_to_outline_add_marker_child(marker_item, tr, pathv_in);
    }
}


/**
 *  Returns a pathvector that is the outline of the stroked item, with markers.
 *  item must be an SPShape or an SPText.
 *  The only current use of this function has exclude_markers true! (SPShape::either_bbox).
 *  TODO: See if SPShape::either_bbox's union with markers is the same as one would get
 *  with bbox_only false.
 */
Geom::PathVector* item_to_outline(SPItem const *item, bool exclude_markers)
{
    Geom::PathVector fill;   // Used for locating markers.
    Geom::PathVector stroke; // Used for creating outline (and finding bbox).
    item_find_paths(item, fill, stroke, true); // Skip cleaning up stroke shape.

    Geom::PathVector *ret_pathv = nullptr;

    if (fill.curveCount() == 0) {
        std::cerr << "item_to_outline: fill path has no segments!" << std::endl;
        return ret_pathv;
    }

    if (stroke.size() > 0) {
        ret_pathv = new Geom::PathVector(stroke);
    } else {
        // No stroke, use fill path.
        ret_pathv = new Geom::PathVector(fill);
    }

    if (exclude_markers) {
        return ret_pathv;
    }

    const SPShape *shape = dynamic_cast<const SPShape *>(item);
    if (shape && shape->hasMarkers()) {

        SPStyle *style = shape->style;
        Geom::Scale scale(style->stroke_width.computed);

        // START marker
        for (int i = 0; i < 2; i++) {  // SP_MARKER_LOC and SP_MARKER_LOC_START
            if ( SPObject *marker_obj = shape->_marker[i] ) {
                Geom::Affine const m (sp_shape_marker_get_transform_at_start(fill.front().front()));
                item_to_outline_add_marker( marker_obj, m, scale, ret_pathv );
            }
        }

        // MID marker
        for (int i = 0; i < 3; i += 2) {  // SP_MARKER_LOC and SP_MARKER_LOC_MID
            SPObject *midmarker_obj = shape->_marker[i];
            if (!midmarker_obj) continue;
            for(Geom::PathVector::const_iterator path_it = fill.begin(); path_it != fill.end(); ++path_it) {

                // START position
                if ( path_it != fill.begin() &&
                     ! ((path_it == (fill.end()-1)) && (path_it->size_default() == 0)) ) // if this is the last path and it is a moveto-only, there is no mid marker there
                {
                    Geom::Affine const m (sp_shape_marker_get_transform_at_start(path_it->front()));
                    item_to_outline_add_marker( midmarker_obj, m, scale, ret_pathv);
                }

                // MID position
                if (path_it->size_default() > 1) {
                    Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
                    Geom::Path::const_iterator curve_it2 = ++(path_it->begin());  // outgoing curve
                    while (curve_it2 != path_it->end_default())
                    {
                        /* Put marker between curve_it1 and curve_it2.
                         * Loop to end_default (so including closing segment), because when a path is closed,
                         * there should be a midpoint marker between last segment and closing straight line segment
                         */
                        Geom::Affine const m (sp_shape_marker_get_transform(*curve_it1, *curve_it2));
                        item_to_outline_add_marker( midmarker_obj, m, scale, ret_pathv);

                        ++curve_it1;
                        ++curve_it2;
                    }
                }

                // END position
                if ( path_it != (fill.end()-1) && !path_it->empty()) {
                    Geom::Curve const &lastcurve = path_it->back_default();
                    Geom::Affine const m = sp_shape_marker_get_transform_at_end(lastcurve);
                    item_to_outline_add_marker( midmarker_obj, m, scale, ret_pathv );
                }
            }
        }

        // END marker
        for (int i = 0; i < 4; i += 3) {  // SP_MARKER_LOC and SP_MARKER_LOC_END
            if ( SPObject *marker_obj = shape->_marker[i] ) {
                /* Get reference to last curve in the path.
                 * For moveto-only path, this returns the "closing line segment". */
                Geom::Path const &path_last = fill.back();
                unsigned int index = path_last.size_default();
                if (index > 0) {
                    index--;
                }
                Geom::Curve const &lastcurve = path_last[index];

                Geom::Affine const m = sp_shape_marker_get_transform_at_end(lastcurve);
                item_to_outline_add_marker( marker_obj, m, scale, ret_pathv );
            }
        }
    }

    return ret_pathv;
}



// ========================= Stroke to Path ====================== //

static
void item_to_paths_add_marker( SPItem *context,
                               SPObject *marker_object, Geom::Affine marker_transform,
                               Geom::Scale stroke_scale,
                               Inkscape::XML::Node *g_repr, Inkscape::XML::Document *xml_doc,
                               SPDocument * doc, bool legacy)
{
    SPMarker* marker = SP_MARKER (marker_object);
    SPItem* marker_item = sp_item_first_item_child(marker_object);
    if (!marker_item) {
        return;
    }

    Geom::Affine tr(marker_transform);

    if (marker->markerUnits == SP_MARKER_UNITS_STROKEWIDTH) {
        tr = stroke_scale * tr;
    }
    // total marker transform
    tr = marker_item->transform * marker->c2p * tr;

    if (marker_item->getRepr()) {
        Inkscape::XML::Node *m_repr = marker_item->getRepr()->duplicate(xml_doc);
        g_repr->addChildAtPos(m_repr, 0);
        SPItem *marker_item = (SPItem *) doc->getObjectByRepr(m_repr);
        marker_item->doWriteTransform(tr);
        if (!legacy) {
            item_to_paths(marker_item, legacy, context);
        }
    }
}


/*
 * Find an outline that represents an item.
 * If not legacy, items are already converted to paths (see verbs.cpp).
 * If legacy, text will not be handled as it is not a shape.
 * If a new item is created it is returned.
 * If the input item is a group and that group contains a changed item, the group node is returned
 * (marking a change).
 *
 * The return value is only used externally to update a selection.
 */
Inkscape::XML::Node*
item_to_paths(SPItem *item, bool legacy, SPItem *context)
{
    char const *id = item->getRepr()->attribute("id");
    SPDocument * document = item->document;
    // flatten all paths effects
    SPLPEItem *lpeitem = SP_LPE_ITEM(item);
    if (lpeitem) {
        lpeitem->removeAllPathEffects(true);
        SPObject *elemref = document->getObjectById(id);
        if (elemref && elemref != item) {
            // If the LPE item is a shape, it is converted to a path 
            // so we need to reupdate the item
            item = dynamic_cast<SPItem *>(elemref);
        }
    }
    // if group, recurse
    SPGroup *group = dynamic_cast<SPGroup *>(item);
    if (group) {
        if (legacy) {
            return nullptr;
        }
        std::vector<SPItem*> const item_list = sp_item_group_item_list(group);
        bool did = false;
        for (auto subitem : item_list) {
            if (item_to_paths(subitem, legacy)) {
                did = true;
            }
        }
        if (did) {
            // This indicates that at least one thing was changed inside the group.
            return group->getRepr();
        } else {
            return nullptr;
        }
    }

    // As written, only shapes are handled. We bail on text early.
    SPShape* shape = dynamic_cast<SPShape *>(item);
    if (!shape) {
        return nullptr;
    }

    Geom::PathVector fill_path;
    Geom::PathVector stroke_path;
    bool status = item_find_paths(item, fill_path, stroke_path);

    if (!status) {
        // Was not a well structured shape (or text).
        return nullptr;
    }

    // The styles ------------------------

    // Copying stroke style to fill will fail for properties not defined by style attribute
    // (i.e., properties defined in style sheet or by attributes).
    SPStyle *style = item->style;
    SPCSSAttr *ncss = sp_css_attr_from_style(style, SP_STYLE_FLAG_ALWAYS);
    SPCSSAttr *ncsf = sp_css_attr_from_style(style, SP_STYLE_FLAG_ALWAYS);
    if (context) {
        SPStyle *context_style = context->style;
        SPCSSAttr *ctxt_style = sp_css_attr_from_style(context_style, SP_STYLE_FLAG_ALWAYS);
        // TODO: browsers has diferent behabiours with context on markers
        // we need to revisit in the future for best matching
        // also dont know if opacity is or want to be included in context
        gchar const *s_val   = sp_repr_css_property(ctxt_style, "stroke", nullptr);
        gchar const *f_val   = sp_repr_css_property(ctxt_style, "fill", nullptr);
        if (style->fill.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE ||
            style->fill.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) 
        {
            gchar const *fill_value = (style->fill.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE) ? s_val : f_val;
            sp_repr_css_set_property(ncss, "fill", fill_value);
            sp_repr_css_set_property(ncsf, "fill", fill_value);
        }
        if (style->stroke.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE ||
            style->stroke.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) 
        {
            gchar const *stroke_value = (style->stroke.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) ? f_val : s_val;
            sp_repr_css_set_property(ncss, "stroke", stroke_value);
            sp_repr_css_set_property(ncsf, "stroke", stroke_value);
        }
    }
    // Stroke
    
    gchar const *s_val   = sp_repr_css_property(ncss, "stroke", nullptr);
    gchar const *s_opac  = sp_repr_css_property(ncss, "stroke-opacity", nullptr);
    gchar const *f_val   = sp_repr_css_property(ncss, "fill", nullptr);
    gchar const *opacity = sp_repr_css_property(ncss, "opacity", nullptr);  // Also for markers
    gchar const *filter  = sp_repr_css_property(ncss, "filter", nullptr);   // Also for markers

    sp_repr_css_set_property(ncss, "stroke", "none");
    sp_repr_css_set_property(ncss, "stroke-width", nullptr);
    sp_repr_css_set_property(ncss, "stroke-opacity", "1.0");
    sp_repr_css_set_property(ncss, "filter", nullptr);
    sp_repr_css_set_property(ncss, "opacity", nullptr);
    sp_repr_css_unset_property(ncss, "marker-start");
    sp_repr_css_unset_property(ncss, "marker-mid");
    sp_repr_css_unset_property(ncss, "marker-end");

    // we change the stroke to fill on ncss for create the filled stroke
    sp_repr_css_set_property(ncss, "fill", s_val);
    if ( s_opac ) {
        sp_repr_css_set_property(ncss, "fill-opacity", s_opac);
    } else {
        sp_repr_css_set_property(ncss, "fill-opacity", "1.0");
    }

    
    sp_repr_css_set_property(ncsf, "stroke", "none");
    sp_repr_css_set_property(ncsf, "stroke-opacity", "1.0");
    sp_repr_css_set_property(ncss, "stroke-width", nullptr);
    sp_repr_css_set_property(ncsf, "filter", nullptr);
    sp_repr_css_set_property(ncsf, "opacity", nullptr);
    sp_repr_css_unset_property(ncsf, "marker-start");
    sp_repr_css_unset_property(ncsf, "marker-mid");
    sp_repr_css_unset_property(ncsf, "marker-end");

    // The object tree -------------------

    // Remember the position of the item
    gint pos = item->getRepr()->position();

    // Remember parent
    Inkscape::XML::Node *parent = item->getRepr()->parent();

    SPDocument * doc = item->document;
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // Create a group to put everything in.
    Inkscape::XML::Node *g_repr = xml_doc->createElement("svg:g");

    Inkscape::copy_object_properties(g_repr, item->getRepr());
    // drop copied style, children will be re-styled (stroke becomes fill)
    g_repr->removeAttribute("style");

    // Add the group to the parent, move to the saved position
    parent->addChildAtPos(g_repr, pos);

    // The stroke ------------------------
    Inkscape::XML::Node *stroke = nullptr;
    if (s_val && g_strcmp0(s_val,"none") != 0 && stroke_path.size() > 0) {
        stroke = xml_doc->createElement("svg:path");
        sp_repr_css_change(stroke, ncss, "style");

        stroke->setAttribute("d", sp_svg_write_path(stroke_path));
    }
    sp_repr_css_attr_unref(ncss);

    // The fill --------------------------
    Inkscape::XML::Node *fill = nullptr;
    if (f_val && g_strcmp0(f_val,"none") != 0 && !legacy) {
        fill = xml_doc->createElement("svg:path");
        sp_repr_css_change(fill, ncsf, "style");

        fill->setAttribute("d", sp_svg_write_path(fill_path));
    }
    sp_repr_css_attr_unref(ncsf);

    // The markers -----------------------
    Inkscape::XML::Node *markers = nullptr;
    Geom::Scale scale(style->stroke_width.computed);

    if (shape->hasMarkers()) {
        if (!legacy) {
            markers = xml_doc->createElement("svg:g");
            g_repr->addChildAtPos(markers, pos);
        } else {
            markers = g_repr;
        }

        // START marker
        for (int i = 0; i < 2; i++) {  // SP_MARKER_LOC and SP_MARKER_LOC_START
            if ( SPObject *marker_obj = shape->_marker[i] ) {
                Geom::Affine const m (sp_shape_marker_get_transform_at_start(fill_path.front().front()));
                item_to_paths_add_marker( item, marker_obj, m, scale,
                                          markers, xml_doc, doc, legacy);
            }
        }

        // MID marker
        for (int i = 0; i < 3; i += 2) {  // SP_MARKER_LOC and SP_MARKER_LOC_MID
            SPObject *midmarker_obj = shape->_marker[i];
            if (!midmarker_obj) continue; // TODO use auto below
            for(Geom::PathVector::const_iterator path_it = fill_path.begin(); path_it != fill_path.end(); ++path_it) {

                // START position
                if ( path_it != fill_path.begin() &&
                     ! ((path_it == (fill_path.end()-1)) && (path_it->size_default() == 0)) ) // if this is the last path and it is a moveto-only, there is no mid marker there
                {
                    Geom::Affine const m (sp_shape_marker_get_transform_at_start(path_it->front()));
                    item_to_paths_add_marker( item, midmarker_obj, m, scale,
                                              markers, xml_doc, doc, legacy);
                }

                // MID position
                if (path_it->size_default() > 1) {
                    Geom::Path::const_iterator curve_it1 = path_it->begin();      // incoming curve
                    Geom::Path::const_iterator curve_it2 = ++(path_it->begin());  // outgoing curve
                    while (curve_it2 != path_it->end_default()) {
                        /* Put marker between curve_it1 and curve_it2.
                         * Loop to end_default (so including closing segment), because when a path is closed,
                         * there should be a midpoint marker between last segment and closing straight line segment
                         */
                        Geom::Affine const m (sp_shape_marker_get_transform(*curve_it1, *curve_it2));
                        item_to_paths_add_marker( item, midmarker_obj, m, scale,
                                                  markers, xml_doc, doc, legacy);

                        ++curve_it1;
                        ++curve_it2;
                    }
                }

                // END position
                if ( path_it != (fill_path.end()-1) && !path_it->empty()) {
                    Geom::Curve const &lastcurve = path_it->back_default();
                    Geom::Affine const m = sp_shape_marker_get_transform_at_end(lastcurve);
                    item_to_paths_add_marker( item, midmarker_obj, m, scale,
                                              markers, xml_doc, doc, legacy);
                }
            }
        }

        // END marker
        for (int i = 0; i < 4; i += 3) {  // SP_MARKER_LOC and SP_MARKER_LOC_END
            if ( SPObject *marker_obj = shape->_marker[i] ) {
                /* Get reference to last curve in the path.
                 * For moveto-only path, this returns the "closing line segment". */
                Geom::Path const &path_last = fill_path.back();
                unsigned int index = path_last.size_default();
                if (index > 0) {
                    index--;
                }
                Geom::Curve const &lastcurve = path_last[index];

                Geom::Affine const m = sp_shape_marker_get_transform_at_end(lastcurve);
                item_to_paths_add_marker( item, marker_obj, m, scale,
                                          markers, xml_doc, doc, legacy);
            }
        }
    }

    gchar const *paint_order = sp_repr_css_property(ncss, "paint-order", nullptr);
    SPIPaintOrder temp;
    temp.read( paint_order );
    bool unique = false;
    if ((!fill && !markers) || (!fill && !stroke) || (!markers && !stroke)) {
        unique = true;
    }
    if (temp.layer[0] != SP_CSS_PAINT_ORDER_NORMAL && !legacy && !unique) {

        if (temp.layer[0] == SP_CSS_PAINT_ORDER_FILL) {
            if (temp.layer[1] == SP_CSS_PAINT_ORDER_STROKE) {
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( markers ) {
                    markers->setPosition(2);
                }
            } else {
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( markers ) {
                    markers->setPosition(1);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
            }
        } else if (temp.layer[0] == SP_CSS_PAINT_ORDER_STROKE) {
            if (temp.layer[1] == SP_CSS_PAINT_ORDER_FILL) {
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( markers ) {
                    markers->setPosition(2);
                }
            } else {
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( markers ) {
                    markers->setPosition(1);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
            }
        } else {
            if (temp.layer[1] == SP_CSS_PAINT_ORDER_STROKE) {
                if ( markers ) {
                    markers->setPosition(0);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
            } else {
                if ( markers ) {
                    markers->setPosition(0);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
            }
        }

    } else if (!unique) {
        if ( fill ) {
            g_repr->appendChild(fill);
        }
        if ( stroke ) {
            g_repr->appendChild(stroke);
        }
        if ( markers ) {
            markers->setPosition(2);
        }
    }

    bool did = false;
    if( fill || stroke || markers ) {
        did = true;
    }

    Inkscape::XML::Node *out = nullptr;

    if (!fill && !markers && did) {
        out = stroke;
    } else if (!fill && !stroke  && did) {
        out = markers;
    } else if (!markers && !stroke  && did) {
        out = fill;
    } else if(did) {
        out = g_repr;
    }

    SPCSSAttr *r_style = sp_repr_css_attr_new();
    sp_repr_css_set_property(r_style, "opacity", opacity);
    sp_repr_css_set_property(r_style, "filter", filter);
    sp_repr_css_change(out, r_style, "style");

    sp_repr_css_attr_unref(r_style);
    if (unique) {
        g_assert(out != g_repr);
        parent->addChild(out, g_repr);
        parent->removeChild(g_repr);
    }
    out->setAttribute("transform", item->getRepr()->attribute("transform"));
    out->setAttribute("id",id);
    Inkscape::GC::release(out);

    // We're replacing item, delete it.
    item->deleteObject(false);

    return out;
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
