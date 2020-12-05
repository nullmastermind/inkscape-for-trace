// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * LPE <slice> implementation: slices a path with respect to a given line.
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *   Abhishek Sharma
 *   Jabiertxof
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilin Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpe-slice.h"
#include "2geom/affine.h"
#include "2geom/path-intersection.h"
#include "display/curve.h"
#include "helper/geom.h"
#include "path-chemistry.h"
#include "style.h"
#include "svg/path-string.h"
#include "svg/svg.h"
#include <gtkmm.h>
#include "path/path-boolop.h"

#include "object/sp-defs.h"
#include "object/sp-lpe-item.h"
#include "object/sp-path.h"
#include "object/sp-text.h"

#include "xml/sp-css-attr.h"

// this is only to flatten nonzero fillrule
#include "livarot/Path.h"
#include "livarot/Shape.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

typedef FillRule FillRuleFlatten;

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<SliceModeType> SliceModeTypeData[] = {
    { SMT_V, N_("Vertical page center"), "vertical" },
    { SMT_H, N_("Horizontal page center"), "horizontal" },
    { SMT_FREE, N_("Freely defined slice line"), "free" },
    { SMT_X, N_("X coordinate of slice line midpoint"), "X" },
    { SMT_Y, N_("Y coordinate of slice line midpoint"), "Y" }
};
static const Util::EnumDataConverter<SliceModeType>
MTConverter(SliceModeTypeData, SMT_END);


LPESlice::LPESlice(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    mode(_("Mode"), _("Set mode of transformation. Either freely defined by slice line or constrained to certain symmetry points."), "mode", MTConverter, &wr, this, SMT_FREE),
    start_point(_("Slice line start"), _("Start point of slice line"), "start_point", &wr, this, _("Adjust start point of slice line")),
    end_point(_("Slice line end"), _("End point of slice line"), "end_point", &wr, this, _("Adjust end point of slice line")),
    center_point(_("Slice line mid"), _("Center point of slice line"), "center_point", &wr, this, _("Adjust center point of slice line")),
    allow_transforms(_("Allow Transforms"), _("Allow transforms"), "allow_transforms", &wr, this, false)
{
    show_orig_path = true;
    registerParameter(&mode);
    registerParameter(&start_point);
    registerParameter(&end_point);
    registerParameter(&center_point);
    registerParameter(&allow_transforms);
    apply_to_clippath_and_mask = false;
    previous_center = Geom::Point(0,0);
    center_point.param_widget_is_visible(false);
    reset = false;
    center_horiz = false;
    center_vert = false;
    allow_transforms_prev = allow_transforms;
}

LPESlice::~LPESlice()
= default;


Gtk::Widget *
LPESlice::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    Gtk::Box *vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(2);
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter *param = *it;
            Gtk::Widget *widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
            Glib::ustring *tip = param->param_getTooltip();
            if (widg) {
                vbox->pack_start(*widg, true, true, 2);
                if (tip) {
                    widg->set_tooltip_text(*tip);
                } else {
                    widg->set_tooltip_text("");
                    widg->set_has_tooltip(false);
                }
            }
        }

        ++it;
    }
    Gtk::Box * hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,0));
    Gtk::Box * hbox2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,0));
    Gtk::Button * center_vert_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Vertical center"))));
    center_vert_button->signal_clicked().connect(sigc::mem_fun (*this,&LPESlice::centerVert));
    center_vert_button->set_size_request(110,20);
    Gtk::Button * center_horiz_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Horizontal center"))));
    center_horiz_button->signal_clicked().connect(sigc::mem_fun (*this,&LPESlice::centerHoriz));
    center_horiz_button->set_size_request(110,20);
    Gtk::Button * reset_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Reset styles"))));
    reset_button->signal_clicked().connect(sigc::mem_fun (*this,&LPESlice::resetStyles));
    reset_button->set_size_request(110,20);
    
    vbox->pack_start(*hbox, true,true,2);
    vbox->pack_start(*hbox2, true,true,2);
    hbox->pack_start(*reset_button, false, false,2);
    hbox2->pack_start(*center_vert_button, false, false,2);
    hbox2->pack_start(*center_horiz_button, false, false,2);
    if(Gtk::Widget* widg = defaultParamSet()) {
        vbox->pack_start(*widg, true, true, 2);
    }
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void
LPESlice::centerVert(){
    center_vert = true;
    refresh_widgets = true;
    writeParamsToSVG();
}

void
LPESlice::centerHoriz(){
    center_horiz = true;
    refresh_widgets = true;
    writeParamsToSVG();
}

bool 
sp_has_path_data(SPItem *item) {
    SPGroup *group = dynamic_cast<SPGroup *>(item);
    if (group) {
        std::vector< SPObject * > childs = group->childList(true);
        for (auto & child : childs) {
            SPItem *item = dynamic_cast<SPItem *>(child);
            if (sp_has_path_data(item)) {
                return true;
            }
        }
    }
    SPShape *shape = dynamic_cast<SPShape *>(item);
    if (shape) {
        SPCurve const *c = shape->curve();
        if (c && !c->is_empty()) {
            return true;
        }
        if (shape->hasPathEffectRecursive()) {
            SPCurve const *c = shape->curveBeforeLPE();
            if (c && !c->is_empty()) {
                return true;
            }
        }
    }
    return false;
}

void 
LPESlice::doAfterEffect (SPLPEItem const* lpeitem, SPCurve *curve)
{
    LPESlice *nextslice = dynamic_cast<LPESlice *>(sp_lpe_item->getNextLPE(this));
    if (!nextslice) {
        for (auto item: items) {
            SPItem *extraitem = dynamic_cast<SPItem *>(getSPDoc()->getObjectById(item.c_str()));
            if (extraitem) {
                extraitem->setHidden(true);
            }
        }
        items.clear();
        std::vector<std::pair<Geom::Line, size_t> > slicer = getSplitLines();
        split(sp_lpe_item, curve, slicer, 0);
        std::vector<Glib::ustring> newitemstmp;
        newitemstmp.assign(items.begin(), items.end());
        items.clear();
        for (auto item: newitemstmp) {
            SPItem *spitem = dynamic_cast<SPItem *>(getSPDoc()->getObjectById(item.c_str()));
            if (spitem && !sp_has_path_data(spitem)) {
                spitem->deleteObject(true);
            } else {
                items.push_back(item);
            }
        }
        reset = false;
    }
}

void
LPESlice::split(SPItem* item, SPCurve *curve, std::vector<std::pair<Geom::Line, size_t> > slicer, size_t splitindex) {
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    Glib::ustring elemref_id = Glib::ustring("slice-");
    elemref_id += Glib::ustring::format(slicer[splitindex].second);
    elemref_id += "-";
    Glib::ustring clean_id = item->getId();
    SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(item);
    if (!lpeitem) {
        return;
    }
    //First check is to allow effects on "satellittes"
    if (!lpeitem->hasPathEffectOfType(SLICE) && clean_id.find("slice-") != Glib::ustring::npos) {
        clean_id = clean_id.replace(0,6,"");
        elemref_id += clean_id;
    } else {
        elemref_id += clean_id;
    }

    items.push_back(elemref_id);

    SPObject *elemref = getSPDoc()->getObjectById(elemref_id.c_str());
    bool prevreset = reset;
    if (!elemref) {
        reset = true;
        Inkscape::XML::Node *phantom = createPathBase(item);
        phantom->setAttribute("id", elemref_id);
        Glib::ustring classes = sp_lpe_item->getId();
        classes += "-slice UnoptimicedTransforms";
        phantom->setAttribute("class", classes.c_str());
        sp_lpe_item->parent->addChild(phantom, sp_lpe_item->getRepr()->prev());
        Inkscape::GC::release(phantom);
        elemref = sp_lpe_item->getPrev();
    }
    SPItem *other = dynamic_cast<SPItem *>(elemref);
    other->setHidden(false);
    size_t nsplits = slicer.size();
    if (other && nsplits) {
        cloneD(item, other, false);
        reset = prevreset;
        splititem(item, curve, slicer[splitindex], true);
        splititem(other, nullptr, slicer[splitindex], false);
        splitindex++;
        if (nsplits > splitindex) {
            split(item, curve, slicer, splitindex);
            split(other, nullptr, slicer, splitindex);
        }
    }
}

std::vector<std::pair<Geom::Line, size_t> >
LPESlice::getSplitLines() {
    std::vector<std::pair<Geom::Line, size_t> > splitlines;
    LPESlice *prevslice = dynamic_cast<LPESlice *>(sp_lpe_item->getPrevLPE(this));
    if (prevslice) {
        splitlines = prevslice->getSplitLines();
    }
    if (isVisible()) {
        Geom::Line line_separation((Geom::Point)start_point, (Geom::Point)end_point);
        size_t index = sp_lpe_item->getLPEIndex(this);
        //splitlines.insert(splitlines.begin(), std::pair(line_separation, index));
        splitlines.emplace_back(line_separation, index);
    }
    return splitlines;
}

Inkscape::XML::Node *
LPESlice::createPathBase(SPObject *elemref) {
    SPDocument *document = getSPDoc();
    if (!document) {
        return nullptr;
    }
    Inkscape::XML::Document *xml_doc = getSPDoc()->getReprDoc();
    Inkscape::XML::Node *prev = elemref->getRepr();
    SPGroup *group = dynamic_cast<SPGroup *>(elemref);
    if (group) {
        Inkscape::XML::Node *container = xml_doc->createElement("svg:g");
        container->setAttribute("transform", prev->attribute("transform"));
        container->setAttribute("mask", prev->attribute("mask"));
        container->setAttribute("clip-path", prev->attribute("clip-path"));
        std::vector<SPItem*> const item_list = sp_item_group_item_list(group);
        Inkscape::XML::Node *previous = nullptr;
        for (auto sub_item : item_list) {
            Inkscape::XML::Node *resultnode = createPathBase(sub_item);
            container->addChild(resultnode, previous);
            previous = resultnode;
        }
        return container;
    }
    Inkscape::XML::Node *resultnode = xml_doc->createElement("svg:path");
    resultnode->setAttribute("transform", prev->attribute("transform"));
    resultnode->setAttribute("mask", prev->attribute("mask"));
    resultnode->setAttribute("clip-path", prev->attribute("clip-path"));
    return resultnode;
}

void
LPESlice::cloneD(SPObject *orig, SPObject *dest, bool is_original)
{
    if (!is_original && !g_strcmp0(sp_lpe_item->getId(), orig->getId() )) {
        is_original = true;
    }
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    SPItem *originalitem = dynamic_cast<SPItem *>(orig);
    if ( SP_IS_GROUP(orig) && SP_IS_GROUP(dest) && SP_GROUP(orig)->getItemCount() == SP_GROUP(dest)->getItemCount() ) {
        if (reset) {
            cloneStyle(orig, dest);
        }
        if (!allow_transforms) {
            auto str = sp_svg_transform_write(originalitem->transform);
            dest->getRepr()->setAttributeOrRemoveIfEmpty("transform", str);
        }
        std::vector< SPObject * > childs = orig->childList(true);
        size_t index = 0;
        for (auto & child : childs) {
            SPObject *dest_child = dest->nthChild(index);
            cloneD(child, dest_child, is_original);
            index++;
        }
        return;
    }

    SPShape * shape =  SP_SHAPE(orig);
    SPPath * path =  SP_PATH(dest);
    if (path && shape) {
        SPCurve const *c = shape->curve();
        if (c) {
            auto str = sp_svg_write_path(c->get_pathvector());
            if (str == "") {
                dest->deleteObject(true);
            } else {
                if (!is_original && shape->hasPathEffectRecursive()) {
                    dest->getRepr()->setAttribute("inkscape:original-d", str);
                } else {
                    dest->getRepr()->setAttribute("d", str);
                }
                if (!allow_transforms) {
                    auto str = sp_svg_transform_write(originalitem->transform);
                    dest->getRepr()->setAttributeOrRemoveIfEmpty("transform", str);
                }
                if (reset) {
                    cloneStyle(orig, dest);
                }
            }
        } else {
            dest->deleteObject(true);
        }
    }

    
}

static void
sp_flatten(Geom::PathVector &pathvector, FillRuleFlatten fillkind)
{
    Path *orig = new Path;
    orig->LoadPathVector(pathvector);
    Shape *theShape = new Shape;
    Shape *theRes = new Shape;
    orig->ConvertWithBackData (1.0);
    orig->Fill (theShape, 0);
    theRes->ConvertToShape (theShape, FillRule(fillkind));
    Path *originaux[1];
    originaux[0] = orig;
    Path *res = new Path;
    theRes->ConvertToForme (res, 1, originaux, true);

    delete theShape;
    delete theRes;
    char *res_d = res->svg_dump_path ();
    delete res;
    delete orig;
    pathvector  = sp_svg_read_pathv(res_d);
}

void
LPESlice::splititem(SPItem* item, SPCurve * curve, std::pair<Geom::Line, size_t> slicer, bool toggle, bool is_original) 
{
    if (!is_original && !g_strcmp0(sp_lpe_item->getId(), item->getId() )) {
        is_original = true;
    }
    Geom::Line line_separation = slicer.first;
    Geom::Point s = line_separation.initialPoint();
    Geom::Point e = line_separation.finalPoint();
    Geom::Point center = Geom::middle_point(s, e);
    SPGroup *group = dynamic_cast<SPGroup *>(item);
    if (group) {
        std::vector< SPObject * > childs = group->childList(true);
        for (auto & child : childs) {
            SPItem *dest_child = dynamic_cast<SPItem *>(child);
            // groups not need update curve
            splititem(dest_child, nullptr, slicer, toggle, is_original);
        }
        if (!is_original && group->hasPathEffectRecursive()) { 
            sp_lpe_item_update_patheffect(group, false, false);
        }
        return;
    }
    SPShape *shape = dynamic_cast<SPShape *>(item);
    SPPath *path = dynamic_cast<SPPath *>(item);
    if (shape) {
        SPCurve const *c;
        c = shape->curve();
        if (c) {
            Geom::PathVector const original_pathv = pathv_to_linear_and_cubic_beziers(c->get_pathvector());
            Geom::PathVector path_out;
            for (const auto & path_it : original_pathv) {
                if (path_it.empty()) {
                    continue;
                }
                Geom::PathVector tmp_pathvector;
                double time_start = 0.0;
                int position = 0;
                bool end_open = false;
                if (path_it.closed()) {
                    const Geom::Curve &closingline = path_it.back_closed();
                    if (!are_near(closingline.initialPoint(), closingline.finalPoint())) {
                        end_open = true;
                    }
                }
                Geom::Path original = path_it;
                if (end_open && path_it.closed()) {
                    original.close(false);
                    original.appendNew<Geom::LineSegment>( original.initialPoint() );
                    original.close(true);
                }
                double dir = line_separation.angle();
                Geom::Ray ray = line_separation.ray(0);
                double diagonal = Geom::distance(Geom::Point(boundingbox_X.min(),boundingbox_Y.min()),Geom::Point(boundingbox_X.max(),boundingbox_Y.max()));
                Geom::Rect bbox(Geom::Point(boundingbox_X.min(),boundingbox_Y.min()),Geom::Point(boundingbox_X.max(),boundingbox_Y.max()));
                double size_divider = Geom::distance(center, bbox) + diagonal;
                s = Geom::Point::polar(dir,size_divider) + center;
                e = Geom::Point::polar(dir + Geom::rad_from_deg(180),size_divider) + center;
                Geom::Path divider = Geom::Path(s);
                divider.appendNew<Geom::LineSegment>(e);
                Geom::Crossings cs = crossings(original, divider);
                std::vector<double> crossed;
                for(auto & c : cs) {
                    crossed.push_back(c.ta);
                }
                double angle = Geom::deg_from_rad(ray.angle());
                bool togleside = !(angle > 0 && angle < 180);
                std::sort(crossed.begin(), crossed.end());
                for (double time_end : crossed) {
                    if (time_start != time_end && time_end - time_start > Geom::EPSILON) {
                        Geom::Path portion = original.portion(time_start, time_end);
                        if (!portion.empty()) {
                            Geom::Point middle = portion.pointAt((double)portion.size()/2.0);
                            position = Geom::sgn(Geom::cross(e - s, middle - s));
                            if (togleside) {
                                position *= -1;
                            }
                            if (toggle) {
                                position *= -1;
                            }
                            if (position == 1) {
                                tmp_pathvector.push_back(portion);
                            }
                            portion.clear();
                        }
                    }
                    time_start = time_end;
                }
                position = Geom::sgn(Geom::cross(e - s, original.finalPoint() - s));
                if (togleside) {
                    position *= -1;
                }
                if (toggle) {
                    position *= -1;
                }
                if (cs.size()!=0 && (position == 1)) {
                    if (time_start != original.size() && original.size() - time_start > Geom::EPSILON) {
                        Geom::Path portion = original.portion(time_start, original.size());
                        if (!portion.empty()) {
                            if (!original.closed()) {
                                tmp_pathvector.push_back(portion);
                            } else {
                                if (cs.size() > 1 && tmp_pathvector.size() > 0 && tmp_pathvector[0].size() > 0 ) {
                                    tmp_pathvector[0] = tmp_pathvector[0].reversed();
                                    portion = portion.reversed();
                                    portion.setInitial(tmp_pathvector[0].finalPoint());
                                    tmp_pathvector[0].append(portion);
                                } else {
                                    tmp_pathvector.push_back(portion);
                                }
                            }
                            portion.clear();
                        }
                    }
                }
                if (cs.size() > 0 && original.closed()) {
                    for (auto &path : tmp_pathvector) {
                        if (!path.closed()) {
                            path.close();
                        }
                    }
                    sp_flatten(tmp_pathvector, fill_oddEven);
                }
                if (cs.size() == 0 && position == 1) {
                   tmp_pathvector.push_back(original);
                }
                path_out.insert(path_out.end(), tmp_pathvector.begin(), tmp_pathvector.end());
                tmp_pathvector.clear();
            }
            if (curve && is_original) {
                curve->set_pathvector(path_out);
            }
            auto cpro = SPCurve::copy(shape->curve());
            if (cpro) {
                if (!path && is_original) {
                    //shape->setCurveInsync(std::move(cpro));
                    shape->bbox_vis_cache_is_valid = false;
                    shape->bbox_geom_cache_is_valid = false;
                    cpro->set_pathvector(path_out);
                    shape->setCurveInsync(std::move(cpro));
                }
                auto str = sp_svg_write_path(path_out);
                if (!is_original && shape->hasPathEffectRecursive()) { 
                    shape->getRepr()->setAttribute("inkscape:original-d", str);
                    sp_lpe_item_update_patheffect(shape, false, false);
                } else {
                    shape->getRepr()->setAttribute("d", str);
                }
            }
        }
    }
}

void
LPESlice::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    using namespace Geom;
    original_bbox(lpeitem, false, true);
    Point point_a(boundingbox_X.max(), boundingbox_Y.min());
    Point point_b(boundingbox_X.max(), boundingbox_Y.max());
    Point point_c(boundingbox_X.middle(), boundingbox_Y.middle());
    if (center_vert) {
        center_point.param_setValue(point_c);
        end_point.param_setValue(Geom::Point(boundingbox_X.middle(), boundingbox_Y.min()));
        //force update
        start_point.param_setValue(Geom::Point(boundingbox_X.middle(), boundingbox_Y.max()),true);
        center_vert = false;
    } else if (center_horiz) {
        center_point.param_setValue(point_c);
        end_point.param_setValue(Geom::Point(boundingbox_X.max(), boundingbox_Y.middle()));
        start_point.param_setValue(Geom::Point(boundingbox_X.min(), boundingbox_Y.middle()),true);
        //force update
        center_horiz = false;
    } else {

        if (mode == SMT_Y) {
            point_a = Geom::Point(boundingbox_X.min(),center_point[Y]);
            point_b = Geom::Point(boundingbox_X.max(),center_point[Y]);
        }
        if (mode == SMT_X) {
            point_a = Geom::Point(center_point[X],boundingbox_Y.min());
            point_b = Geom::Point(center_point[X],boundingbox_Y.max());
        }
        if ((Geom::Point)start_point == (Geom::Point)end_point) {
            start_point.param_setValue(point_a);
            end_point.param_setValue(point_b);
            previous_center = Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
            center_point.param_setValue(previous_center);
            return;
        }
        if ( mode == SMT_X || mode == SMT_Y ) {
            if (!are_near(previous_center, (Geom::Point)center_point, 0.01)) {
                center_point.param_setValue(Geom::middle_point(point_a, point_b));
                end_point.param_setValue(point_b);
                start_point.param_setValue(point_a);
            } else {
                if ( mode == SMT_X ) {
                    if (!are_near(start_point[X], point_a[X], 0.01)) {
                        start_point.param_setValue(point_a);
                    }
                    if (!are_near(end_point[X], point_b[X], 0.01)) {
                        end_point.param_setValue(point_b);
                    }
                } else {  //SMT_Y
                    if (!are_near(start_point[Y], point_a[Y], 0.01)) {
                        start_point.param_setValue(point_a);
                    }
                    if (!are_near(end_point[Y], point_b[Y], 0.01)) {
                        end_point.param_setValue(point_b);
                    }
                }
            }
        } else if ( mode == SMT_FREE) {
            if (are_near(previous_center, (Geom::Point)center_point, 0.01)) {
                center_point.param_setValue(Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point));

            } else {
                Geom::Point trans = center_point - Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
                start_point.param_setValue(start_point * trans);
                end_point.param_setValue(end_point * trans);
            }
        } else if ( mode == SMT_V){
            Geom::Affine transform = i2anc_affine(SP_OBJECT(lpeitem), nullptr).inverse();
            Geom::Point sp = Geom::Point(document->getWidth().value("px")/2.0, 0) * transform;
            start_point.param_setValue(sp);
            Geom::Point ep = Geom::Point(document->getWidth().value("px")/2.0, document->getHeight().value("px")) * transform;
            end_point.param_setValue(ep);
            center_point.param_setValue(Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point));
        } else { //horizontal page
            Geom::Affine transform = i2anc_affine(SP_OBJECT(lpeitem), nullptr).inverse();
            Geom::Point sp = Geom::Point(0, document->getHeight().value("px")/2.0) * transform;
            start_point.param_setValue(sp);
            Geom::Point ep = Geom::Point(document->getWidth().value("px"), document->getHeight().value("px")/2.0) * transform;
            end_point.param_setValue(ep);
            center_point.param_setValue(Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point));
        }
    }
    if (allow_transforms_prev != allow_transforms) {
        LPESlice *nextslice = dynamic_cast<LPESlice *>(sp_lpe_item->getNextLPE(this));
        while (nextslice) {
            if (nextslice->allow_transforms != allow_transforms) {
                nextslice->allow_transforms_prev = allow_transforms;
                nextslice->allow_transforms.param_setValue(allow_transforms);
            }
            nextslice = dynamic_cast<LPESlice *>(sp_lpe_item->getNextLPE(nextslice));
        }
        LPESlice *prevslice = dynamic_cast<LPESlice *>(sp_lpe_item->getPrevLPE(this));
        while (prevslice) {
            if (prevslice->allow_transforms != allow_transforms) {
                prevslice->allow_transforms_prev = allow_transforms;
                prevslice->allow_transforms.param_setValue(allow_transforms);
            }
            prevslice = dynamic_cast<LPESlice *>(sp_lpe_item->getNextLPE(prevslice));
        }
    }
    allow_transforms_prev = allow_transforms;
}

void LPESlice::cloneStyle(SPObject *orig, SPObject *dest)
{
    dest->getRepr()->setAttribute("style", orig->getRepr()->attribute("style"));
    for (auto iter : orig->style->properties()) {
        if (iter->style_src != SPStyleSrc::UNSET) {
            auto key = iter->id();
            if (key != SPAttr::FONT && key != SPAttr::D && key != SPAttr::MARKER) {
                const gchar *attr = orig->getRepr()->attribute(iter->name().c_str());
                if (attr) {
                    dest->getRepr()->setAttribute(iter->name(), attr);
                }
            }
        }
    }
}

void
LPESlice::resetStyles(){
    LPESlice *nextslice = dynamic_cast<LPESlice *>(sp_lpe_item->getNextLPE(this));
    while (nextslice) {
        nextslice->reset = true;
        nextslice = dynamic_cast<LPESlice *>(sp_lpe_item->getNextLPE(nextslice));
    }
    reset = true;
    sp_lpe_item_update_patheffect(sp_lpe_item, false, false);
}

//TODO: Migrate the tree next function to effect.cpp/h to avoid duplication
void
LPESlice::doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/)
{
    processObjects(LPE_VISIBILITY);
}

void
LPESlice::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    //set "keep paths" hook on sp-lpe-item.cpp
    items.clear();
    Glib::ustring theclass = sp_lpe_item->getId();
    theclass += "-slice";
    for (auto item : getSPDoc()->getObjectsByClass(theclass)) {
        items.emplace_back(item->getId());
    }
    if (keep_paths) {
        processObjects(LPE_TO_OBJECTS);
        items.clear();
        return;
    }
    if (sp_lpe_item->countLPEOfType(SLICE) == 1) {
        processObjects(LPE_ERASE);
    } else {
        for (auto item: items) {
            SPItem *extraitem = dynamic_cast<SPItem *>(getSPDoc()->getObjectById(item.c_str()));
            if (extraitem) {
                extraitem->setHidden(true);
            }
        }
    }
}

void
LPESlice::doOnApply (SPLPEItem const* lpeitem)
{
    using namespace Geom;

    original_bbox(lpeitem, false, true);
    LPESlice *prevslice = dynamic_cast<LPESlice *>(sp_lpe_item->getPrevLPE(this));
    if (prevslice) {
        allow_transforms_prev = prevslice->allow_transforms;
        allow_transforms.param_setValue(prevslice->allow_transforms);
    }
    Point point_a(boundingbox_X.middle(), boundingbox_Y.min());
    Point point_b(boundingbox_X.middle(), boundingbox_Y.max());
    Point point_c(boundingbox_X.middle(), boundingbox_Y.middle());
    start_point.param_setValue(point_a, true);
    start_point.param_update_default(point_a);
    end_point.param_setValue(point_b, true);
    end_point.param_update_default(point_b);
    center_point.param_setValue(point_c, true);
    previous_center = center_point;
    if (prevslice) {
        for (auto item : prevslice->items) {
            SPItem *extraitem = dynamic_cast<SPItem *>(getSPDoc()->getObjectById(item.c_str()));
            if (extraitem) {
                extraitem->setHidden(true);
            }
            items.push_back(item);
        }
        prevslice->items.clear();
    }
}


Geom::PathVector
LPESlice::doEffect_path (Geom::PathVector const & path_in)
{
    return path_in;
}

void
LPESlice::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    using namespace Geom;
    hp_vec.clear();
    Geom::Path path;
    Geom::Point s = start_point;
    Geom::Point e = end_point;
    path.start( s );
    path.appendNew<Geom::LineSegment>( e );
    Geom::PathVector helper;
    helper.push_back(path);
    hp_vec.push_back(helper);
}

} //namespace LivePathEffect
} /* namespace Inkscape */

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
