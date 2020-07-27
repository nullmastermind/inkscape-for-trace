// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Boolean operation live path effect
 *
 * Copyright (C) 2016-2017 Michael Soegtrop
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpe-bool.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <glibmm/i18n.h>

#include "2geom/affine.h"
#include "2geom/bezier-curve.h"
#include "2geom/path-sink.h"
#include "2geom/path.h"
#include "2geom/svg-path-parser.h"
#include "display/curve.h"
#include "helper/geom.h"
#include "inkscape.h"
#include "selection-chemistry.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"
#include "livarot/path-description.h"
#include "live_effects/lpeobject.h"
#include "object/sp-clippath.h"
#include "object/sp-defs.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "object/sp-root.h"
#include "path/path-boolop.h"
#include "path/path-util.h"
#include "snap.h"
#include "style.h"
#include "svg/svg.h"
#include "ui/tools/tool-base.h"

namespace Inkscape {
namespace LivePathEffect {

// Define an extended boolean operation type

static const Util::EnumData<LPEBool::bool_op_ex> BoolOpData[LPEBool::bool_op_ex_count] = {
    {LPEBool::bool_op_ex_union, N_("union"), "union"},
    {LPEBool::bool_op_ex_inters, N_("intersection"), "inters"},
    {LPEBool::bool_op_ex_diff, N_("difference"), "diff"},
    {LPEBool::bool_op_ex_symdiff, N_("symmetric difference"), "symdiff"},
    {LPEBool::bool_op_ex_cut, N_("division"), "cut"},
    // Note on naming of operations:
    // bool_op_cut is called "Division" in the manu, see sp_selected_path_cut
    // bool_op_slice is called "Cut path" in the menu, see sp_selected_path_slice
    {LPEBool::bool_op_ex_slice, N_("cut"), "slice"},
    {LPEBool::bool_op_ex_slice_inside, N_("cut inside"), "slice-inside"},
    {LPEBool::bool_op_ex_slice_outside, N_("cut outside"), "slice-outside"},
};

static const Util::EnumDataConverter<LPEBool::bool_op_ex> BoolOpConverter(BoolOpData, sizeof(BoolOpData) / sizeof(*BoolOpData));

static const Util::EnumData<fill_typ> FillTypeData[] = {
    { fill_oddEven, N_("even-odd"), "oddeven" },
    { fill_nonZero, N_("non-zero"), "nonzero" },
    { fill_positive, N_("positive"), "positive" },
    { fill_justDont, N_("take from object"), "from-curve" }
};

static const Util::EnumDataConverter<fill_typ> FillTypeConverter(FillTypeData, sizeof(FillTypeData) / sizeof(*FillTypeData));

LPEBool::LPEBool(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , operand_path(_("Operand path:"), _("Operand for the boolean operation"), "operand-path", &wr, this)
    , bool_operation(_("Operation:"), _("Boolean Operation"), "operation", BoolOpConverter, &wr, this, bool_op_ex_union)
    , swap_operands(_("Swap operands"), _("Swap operands (useful e.g. for difference)"), "swap-operands", &wr, this)
    , rmv_inner(
          _("Remove inner"),
          _("For cut operations: remove inner (non-contour) lines of cutting path to avoid invisible extra points"),
          "rmv-inner", &wr, this)
    , fill_type_this(_("Fill type this:"), _("Fill type (winding mode) for this path"), "filltype-this",
                     FillTypeConverter, &wr, this, fill_justDont)
    , fill_type_operand(_("Fill type operand:"), _("Fill type (winding mode) for operand path"), "filltype-operand",
                        FillTypeConverter, &wr, this, fill_justDont)
    , filter("Filter", "Previous filter", "filter", &wr, this, "", true)
{
    registerParameter(&operand_path);
    registerParameter(&bool_operation);
    registerParameter(&swap_operands);
    registerParameter(&rmv_inner);
    registerParameter(&fill_type_this);
    registerParameter(&filter);
    registerParameter(&fill_type_operand);
    show_orig_path = true;
    is_load = true;
    prev_affine = Geom::identity();
    operand = dynamic_cast<SPItem *>(operand_path.getObject());
}

LPEBool::~LPEBool() {
    doOnRemove(nullptr);
}
bool cmp_cut_position(const Path::cut_position &a, const Path::cut_position &b)
{
    return a.piece == b.piece ? a.t < b.t : a.piece < b.piece;
}

Geom::PathVector
sp_pathvector_boolop_slice_intersect(Geom::PathVector const &pathva, Geom::PathVector const &pathvb, bool inside, fill_typ fra, fill_typ frb)
{
    // This is similar to sp_pathvector_boolop/bool_op_slice, but keeps only edges inside the cutter area.
    // The code is also based on sp_pathvector_boolop_slice.
    //
    // We have two paths on input
    // - a closed area which is used to cut out pieces from a contour (called area below)
    // - a contour which is cut into pieces by the border of thr area (called contour below)
    //
    // The code below works in the following steps
    // (a) Convert the area to a shape, so that we can ask the winding number for any point
    // (b) Add both, the contour and the area to a single shape and intersect them
    // (c) Find the intersection points between area border and contour (vector toCut)
    // (d) Split the original contour at the intersection points
    // (e) check for each contour edge in combined shape if its center is inside the area - if not discard it
    // (f) create a vector of all inside edges
    // (g) convert the piece numbers to the piece numbers after applying the cuts
    // (h) fill a bool vector with information which pieces are in
    // (i) filter the descr_cmd of the result path with this bool vector
    //
    // The main inefficiency here is step (e) because I use a winding function of the area-shape which goes
    // through the complete edge list for each point I ask for, so effort is n-edges-contour * n-edges-area.
    // It is tricky to improve this without building into the livarot code.
    // One way might be to decide at the intersection points which edges touching the intersection points are
    // in by making a loop through all edges on the intersection vertex. Since this is a directed non intersecting
    // graph, this should provide sufficient information.
    // But since I anyway will change this to the new mechanism some time speed is fairly ok, I didn't look into this.


    // extract the livarot Paths from the source objects
    // also get the winding rule specified in the style
    // Livarot's outline of arcs is broken. So convert the path to linear and cubics only, for which the outline is created correctly.
    Path *contour_path = Path_for_pathvector(pathv_to_linear_and_cubic_beziers(pathva));
    Path *area_path = Path_for_pathvector(pathv_to_linear_and_cubic_beziers(pathvb));

    // Shapes from above paths
    Shape *area_shape = new Shape;
    Shape *combined_shape = new Shape;
    Shape *combined_inters = new Shape;

    // Add the area (process to intersection free shape)
    area_path->ConvertWithBackData(1.0);
    area_path->Fill(combined_shape, 1);

    // Convert this to a shape with full winding information
    area_shape->ConvertToShape(combined_shape, frb);

    // Add the contour to the combined path (just add, no winding processing)
    contour_path->ConvertWithBackData(1.0);
    contour_path->Fill(combined_shape, 0, true, false, false);

    // Intersect the area and the contour - no fill processing
    combined_inters->ConvertToShape(combined_shape, fill_justDont);

    // Result path
    Path *result_path = new Path;
    result_path->SetBackData(false);

    // Cutting positions for contour
    std::vector<Path::cut_position> toCut;

    if (combined_inters->hasBackData()) {
        // should always be the case, but ya never know
        {
            for (int i = 0; i < combined_inters->numberOfPoints(); i++) {
                if (combined_inters->getPoint(i).totalDegree() > 2) {
                    // possibly an intersection
                    // we need to check that at least one edge from the source path is incident to it
                    // before we declare it's an intersection
                    int cb = combined_inters->getPoint(i).incidentEdge[FIRST];
                    int   nbOrig = 0;
                    int   nbOther = 0;
                    int   piece = -1;
                    float t = 0.0;
                    while (cb >= 0 && cb < combined_inters->numberOfEdges()) {
                        if (combined_inters->ebData[cb].pathID == 0) {
                            // the source has an edge incident to the point, get its position on the path
                            piece = combined_inters->ebData[cb].pieceID;
                            if (combined_inters->getEdge(cb).st == i) {
                                t = combined_inters->ebData[cb].tSt;
                            } else {
                                t = combined_inters->ebData[cb].tEn;
                            }
                            nbOrig++;
                        }
                        if (combined_inters->ebData[cb].pathID == 1) {
                            nbOther++;    // the cut is incident to this point
                        }
                        cb = combined_inters->NextAt(i, cb);
                    }
                    if (nbOrig > 0 && nbOther > 0) {
                        // point incident to both path and cut: an intersection
                        // note that you only keep one position on the source; you could have degenerate
                        // cases where the source crosses itself at this point, and you wouyld miss an intersection
                        Path::cut_position cutpos;
                        cutpos.piece = piece;
                        cutpos.t = t;
                        toCut.push_back(cutpos);
                    }
                }
            }
        }
        {
            // remove the edges from the intersection polygon
            int i = combined_inters->numberOfEdges() - 1;
            for (; i >= 0; i--) {
                if (combined_inters->ebData[i].pathID == 1) {
                    combined_inters->SubEdge(i);
                } else {
                    const Shape::dg_arete &edge = combined_inters->getEdge(i);
                    const Shape::dg_point &start = combined_inters->getPoint(edge.st);
                    const Shape::dg_point &end = combined_inters->getPoint(edge.en);
                    Geom::Point mid = 0.5 * (start.x + end.x);
                    int wind = area_shape->PtWinding(mid);
                    if (wind == 0) {
                        combined_inters->SubEdge(i);
                    }
                }
            }
        }
    }

    // create a vector of pieces, which are in the intersection
    std::vector<Path::cut_position> inside_pieces(combined_inters->numberOfEdges());
    for (int i = 0; i < combined_inters->numberOfEdges(); i++) {
        inside_pieces[i].piece = combined_inters->ebData[i].pieceID;
        // Use the t middle point, this is safe to compare with values from toCut in the presence of roundoff errors
        inside_pieces[i].t = 0.5 * (combined_inters->ebData[i].tSt + combined_inters->ebData[i].tEn);
    }
    std::sort(inside_pieces.begin(), inside_pieces.end(), cmp_cut_position);

    // sort cut positions
    std::sort(toCut.begin(), toCut.end(), cmp_cut_position);

    // Compute piece ids after ConvertPositionsToMoveTo
    {
        int idIncr = 0;
        std::vector<Path::cut_position>::iterator itPiece = inside_pieces.begin();
        std::vector<Path::cut_position>::iterator itCut = toCut.begin();
        while (itPiece != inside_pieces.end()) {
            while (itCut != toCut.end() && cmp_cut_position(*itCut, *itPiece)) {
                ++itCut;
                idIncr += 2;
            }
            itPiece->piece += idIncr;
            ++itPiece;
        }
    }

    // Copy the original path to result and cut at the intersection points
    result_path->Copy(contour_path);
    result_path->ConvertPositionsToMoveTo(toCut.size(), toCut.data());   // cut where you found intersections

    // Create an array of bools which states which pieces are in
    std::vector<bool> inside_flags(result_path->descr_cmd.size(), false);
    for (auto & inside_piece : inside_pieces) {
        inside_flags[ inside_piece.piece ] = true;
        // also enable the element -1 to get the MoveTo
        if (inside_piece.piece >= 1) {
            inside_flags[ inside_piece.piece - 1 ] = true;
        }
    }

#if 0 // CONCEPT TESTING
    //Check if the inside/outside verdict is consistent - just for testing the concept
    // Retrieve the pieces
    int nParts = 0;
    Path **parts = result_path->SubPaths(nParts, false);

    // Each piece should be either fully in or fully out
    int iPiece = 0;
    for (int iPart = 0; iPart < nParts; iPart++) {
        bool andsum = true;
        bool orsum = false;
        for (int iCmd = 0; iCmd < parts[iPart]->descr_cmd.size(); iCmd++, iPiece++) {
            andsum = andsum && inside_flags[ iPiece ];
            orsum = andsum || inside_flags[ iPiece ];
        }

        if (andsum != orsum) {
            g_warning("Inconsistent inside/outside verdict for part=%d", iPart);
        }
    }
    g_free(parts);
#endif

    // iterate over the commands of a path and keep those which are inside
    int iDest = 0;
    for (int iSrc = 0; iSrc < result_path->descr_cmd.size(); iSrc++) {
        if (inside_flags[iSrc] == inside) {
            result_path->descr_cmd[iDest++] = result_path->descr_cmd[iSrc];
        } else {
            delete result_path->descr_cmd[iSrc];
        }
    }
    result_path->descr_cmd.resize(iDest);

    delete combined_inters;
    delete combined_shape;
    delete area_shape;
    delete contour_path;
    delete area_path;

    gchar *result_str = result_path->svg_dump_path();
    Geom::PathVector outres =  Geom::parse_svg_path(result_str);
    // CONCEPT TESTING g_warning( "%s", result_str );
    g_free(result_str);
    delete result_path;

    return outres;
}

// remove inner contours
Geom::PathVector
sp_pathvector_boolop_remove_inner(Geom::PathVector const &pathva, fill_typ fra)
{
    Geom::PathVector patht;
    Path *patha = Path_for_pathvector(pathv_to_linear_and_cubic_beziers(pathva));

    Shape *shape = new Shape;
    Shape *shapeshape = new Shape;
    Path *resultp = new Path;
    resultp->SetBackData(false);

    patha->ConvertWithBackData(0.1);
    patha->Fill(shape, 0);
    shapeshape->ConvertToShape(shape, fra);
    shapeshape->ConvertToForme(resultp, 1, &patha);

    delete shape;
    delete shapeshape;
    delete patha;

    gchar *result_str = resultp->svg_dump_path();
    Geom::PathVector resultpv =  Geom::parse_svg_path(result_str);
    g_free(result_str);

    delete resultp;
    return resultpv;
}

static fill_typ GetFillTyp(SPItem *item)
{
    SPCSSAttr *css = sp_repr_css_attr(item->getRepr(), "style");
    gchar const *val = sp_repr_css_property(css, "fill-rule", nullptr);
    if (val && strcmp(val, "nonzero") == 0) {
        return fill_nonZero;
    } else if (val && strcmp(val, "evenodd") == 0) {
        return fill_oddEven;
    } else {
        return fill_nonZero;
    }
}

void LPEBool::add_filter()
{
    if (operand) {
        Inkscape::XML::Node *repr = operand->getRepr();
        if (!repr) {
            return;
        }
        SPFilter *filt = operand->style->getFilter();
        if (filt && filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") != 0) {
            filter.param_setValue(filt->getId(), true);
        }
        if (!filt || (filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") != 0)) {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property(css, "filter", "url(#selectable_hidder_filter)");
            sp_repr_css_change(repr, css, "style");
            sp_repr_css_attr_unref(css);
        }
    }
}

void LPEBool::remove_filter()
{
    if (operand) {
        Inkscape::XML::Node *repr = operand->getRepr();
        if (!repr) {
            return;
        }
        SPFilter *filt = operand->style->getFilter();
        if (filt && (filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") == 0)) {
            SPCSSAttr *css = sp_repr_css_attr_new();
            Glib::ustring filtstr = filter.param_getSVGValue();
            if (filtstr != "") {
                Glib::ustring url = "url(#";
                url += filtstr;
                url += ")";
                sp_repr_css_set_property(css, "filter", url.c_str());
                // blur is removed when no item using it
                /*SPDocument *document = getSPDoc();
                SPObject * filterobj = nullptr;
                if((filterobj = document->getObjectById(filtstr))) {
                    for (auto obj:filterobj->childList(false)) {
                        if (obj) {
                            obj->deleteObject(false);
                            break;
                        }
                    }
                } */
                filter.param_setValue("");
            } else {
                sp_repr_css_unset_property(css, "filter");
            }
            sp_repr_css_change(repr, css, "style");
            sp_repr_css_attr_unref(css);
        }
    }
}

void LPEBool::doBeforeEffect(SPLPEItem const *lpeitem)
{
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    _hp.clear();
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    SPObject *elemref = nullptr;
    Inkscape::XML::Node *boolfilter = nullptr;
    if (!(elemref = document->getObjectById("selectable_hidder_filter"))) {
        boolfilter = xml_doc->createElement("svg:filter");
        boolfilter->setAttribute("id", "selectable_hidder_filter");
        boolfilter->setAttribute("width", "1");
        boolfilter->setAttribute("height", "1");
        boolfilter->setAttribute("x", "0");
        boolfilter->setAttribute("y", "0");
        boolfilter->setAttribute("style", "color-interpolation-filters:sRGB;");
        boolfilter->setAttribute("inkscape:label", "LPE boolean visibility");
        /* Create <path> */
        Inkscape::XML::Node *primitive = xml_doc->createElement("svg:feComposite");
        primitive->setAttribute("id", "boolops_hidder_primitive");
        primitive->setAttribute("result", "composite1");
        primitive->setAttribute("operator", "arithmetic");
        primitive->setAttribute("in2", "SourceGraphic");
        primitive->setAttribute("in", "BackgroundImage");
        Inkscape::XML::Node *defs = document->getDefs()->getRepr();
        defs->addChild(boolfilter, nullptr);
        Inkscape::GC::release(boolfilter);
        boolfilter->addChild(primitive, nullptr);
        Inkscape::GC::release(primitive);
    } else {
        for (auto obj : elemref->childList(false)) {
            if (obj && strcmp(obj->getId(), "boolops_hidder_primitive") != 0) {
                obj->deleteObject(true);
            }
        }
    }
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    SPItem *current_operand = dynamic_cast<SPItem *>(operand_path.getObject());
    if (!current_operand) {
        operand_path.remove_link();
        operand = nullptr;
    }
    if (current_operand && current_operand->getId()) {
        if (!(document->getObjectById(current_operand->getId()))) {
            operand_path.remove_link();
            operand = nullptr;
            current_operand = nullptr;
        }
    }
    SPLPEItem *operandlpe = dynamic_cast<SPLPEItem *>(operand_path.getObject());
    if (desktop && 
        operand && 
        sp_lpe_item &&
        desktop->getSelection()->includes(operand) && 
        desktop->getSelection()->includes(sp_lpe_item)) 
    {
        if (operandlpe && operandlpe->hasPathEffectOfType(Inkscape::LivePathEffect::EffectType::BOOL_OP)) {
            sp_lpe_item_update_patheffect(operandlpe, false, false);
        }
        desktop->getSelection()->remove(operand);
    }
    if (!current_operand) {
        if (operand) {
            remove_filter();
        }
        operand = nullptr;
    }
    
    if (current_operand && operand != current_operand) {
        if (operand) {
            remove_filter();
        }
        operand = current_operand;
        remove_filter();
        if (is_load && sp_lpe_item) {
            sp_lpe_item_update_patheffect(sp_lpe_item, true, true);
        }
    }
    if (operand) {
        if (is_visible) {
            add_filter();
        } else {
            remove_filter();
        }
    }
}

void LPEBool::transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    if (operand && !isOnClipboard()) {
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        if (desktop && !desktop->getSelection()->includes(operand)) {
            prev_affine = operand->transform * sp_item_transform_repr(sp_lpe_item).inverse() * postmul;
            operand->doWriteTransform(prev_affine);
        }
    }
}

Geom::PathVector LPEBool::get_union(SPObject *object)
{
    Geom::PathVector res;
    Geom::PathVector clippv;
    SPItem *objitem = dynamic_cast<SPItem *>(object);
    if (objitem) {
        SPObject *clip_path = objitem->getClipObject();
        if (clip_path) {
            std::vector<SPObject *> clip_path_list = clip_path->childList(true);
            if (clip_path_list.size()) {
                for (auto clip : clip_path_list) {
                    SPShape *clipshape = dynamic_cast<SPShape *>(clip);
                    if (clipshape) {
                        clippv = clipshape->curve()->get_pathvector();
                    }
                }
            }
        }
    }
    SPGroup *group = dynamic_cast<SPGroup *>(object);
    if (group) {
        std::vector<SPItem *> item_list = sp_item_group_item_list(group);
        for (auto iter : item_list) {
            if (res.empty()) {
                res = get_union(SP_OBJECT(iter));
            } else {
                res = sp_pathvector_boolop(res, get_union(SP_OBJECT(iter)), to_bool_op(bool_op_ex_union), fill_oddEven,
                                           fill_oddEven);
            }
        }
    }
    SPShape *shape = dynamic_cast<SPShape *>(object);
    if (shape) {
        fill_typ originfill = fill_oddEven;
        SPCurve *curve = shape->curve();
        if (curve) {
            if (res.empty()) {
                res = curve->get_pathvector();
            } else {
                res = sp_pathvector_boolop(res, curve->get_pathvector(), to_bool_op(bool_op_ex_union), originfill,
                                           GetFillTyp(shape));
            }
        }
        originfill = GetFillTyp(shape);
    }
    SPText *text = dynamic_cast<SPText *>(object);
    if (text) {
        std::unique_ptr<SPCurve> curve = text->getNormalizedBpath();
        if (curve) {
            if (res.empty()) {
                res = curve->get_pathvector();
            } else {
                res = sp_pathvector_boolop(res, curve->get_pathvector(), to_bool_op(bool_op_ex_union), fill_oddEven,
                                           fill_oddEven);
            }
        }
    }
    if (!clippv.empty()) {
        res = sp_pathvector_boolop(res, clippv, to_bool_op(bool_op_ex_inters), fill_oddEven, fill_oddEven);
    }
    return res;
}

void LPEBool::doEffect(SPCurve *curve)
{
    Geom::PathVector path_in = curve->get_pathvector();
    if (operand == SP_ITEM(current_shape)) {
        g_warning("operand and current shape are the same");
        operand_path.param_set_default();
        return;
    }
    if (operand_path.getObject() && operand) {
        bool_op_ex op = bool_operation.get_value();
        bool swap =  swap_operands.get_value();

        Geom::Affine current_affine = sp_lpe_item->transform;
        Geom::Affine operand_affine = operand->transform;
        Geom::PathVector operand_pv = get_union(operand);
        if (operand_pv.empty()) {
            return;
        }
        path_in *= current_affine;
        operand_pv *= operand_affine;

        Geom::PathVector path_a = swap ? path_in : operand_pv;
        Geom::PathVector path_b = swap ? operand_pv : path_in;
        _hp = path_a;
        _hp.insert(_hp.end(), path_b.begin(), path_b.end());
        _hp *= current_affine.inverse();
        fill_typ fill_this    = fill_type_this.get_value() != fill_justDont ? fill_type_this.get_value() : GetFillTyp(current_shape);
        fill_typ fill_operand = fill_type_operand.get_value() != fill_justDont ? fill_type_operand.get_value() : GetFillTyp(operand_path.getObject());

        fill_typ fill_a = swap ? fill_this : fill_operand;
        fill_typ fill_b = swap ? fill_operand : fill_this;

        if (rmv_inner.get_value()) {
            path_b = sp_pathvector_boolop_remove_inner(path_b, fill_b);
        }

        Geom::PathVector path_out;
        if (op == bool_op_ex_cut) {
            // we do in two pass because wrong sp_pathvector_boolop diference, in commnet they sugest make this to fix
            Geom::PathVector path_tmp_outside =
                sp_pathvector_boolop_slice_intersect(path_a, path_b, false, fill_a, fill_b);
            Geom::PathVector path_tmp = sp_pathvector_boolop(path_a, path_b, to_bool_op(op), fill_a, fill_b);
            for (auto pathit : path_tmp) {
                bool outside = false;
                for (auto pathit_out : path_tmp_outside) {
                    Geom::OptRect outbbox = pathit_out.boundsFast();
                    if (outbbox) {
                        (*outbbox).expandBy(1);
                        if ((*outbbox).contains(pathit.boundsFast())) {
                            outside = true;
                        }
                    }
                }
                if (!outside) {
                    path_out.push_back(pathit);
                }
            }
        } else if (op == bool_op_ex_slice) {
            // For slicing, the bool op is added to the line group which is sliced, not the cut path. This swapped order is correct            
            path_out = sp_pathvector_boolop(path_b, path_a, to_bool_op(op), fill_b, fill_a);
        } else if (op == bool_op_ex_slice_inside) {
            path_out = sp_pathvector_boolop_slice_intersect(path_a, path_b, true, fill_a, fill_b);
        } else if (op == bool_op_ex_slice_outside) {
            path_out = sp_pathvector_boolop_slice_intersect(path_a, path_b, false, fill_a, fill_b);
        } else {
            path_out = sp_pathvector_boolop(path_a, path_b, to_bool_op(op), fill_a, fill_b);
        }
        /*
        Maybe add a bit simplify?
        Geom::PathVector pathv = path_out * current_affine.inverse();
        gdouble size  = Geom::L2(pathv.boundsFast()->dimensions());
        Path* pathliv = Path_for_pathvector(pathv);
        pathliv->ConvertEvenLines(0.00002 * size);
        pathliv->Simplify(0.00002 * size);
        pathv = Geom::parse_svg_path(pathliv->svg_dump_path()); */
        curve->set_pathvector(path_out * current_affine.inverse());
    }
}

void LPEBool::addCanvasIndicators(SPLPEItem const * /*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

void LPEBool::doOnRemove(SPLPEItem const * /*lpeitem*/)
{
    // set "keep paths" hook on sp-lpe-item.cpp
    SPItem *operand = dynamic_cast<SPItem *>(operand_path.getObject());
    if (operand) {
        if (keep_paths) {
            if (is_visible) {
                operand->deleteObject(true);
            }
        } else {
            if (is_visible) {
                remove_filter();
            }
        }
    }
}

// TODO: Migrate the tree next function to effect.cpp/h to avoid duplication
void LPEBool::doOnVisibilityToggled(SPLPEItem const * /*lpeitem*/)
{
    SPItem *operand = dynamic_cast<SPItem *>(operand_path.getObject());
    if (operand) {
        if (!is_visible) {
            remove_filter();
        }
    }
}

} // namespace LivePathEffect
} /* namespace Inkscape */
