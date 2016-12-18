/** \file
 * LPE <mirror_symmetry> implementation: mirrors a path with respect to a given line.
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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm.h>
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpe-mirror_symmetry.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"
#include "sp-defs.h"
#include "helper/geom.h"
#include "2geom/path-intersection.h"
#include "2geom/affine.h"
#include "helper/geom.h"
#include "uri.h"
#include "uri-references.h"
#include "path-chemistry.h"
#include "knotholder.h"
#include "style.h"
#include "xml/sp-css-attr.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<ModeType> ModeTypeData[MT_END] = {
    { MT_V, N_("Vertical Page Center"), "vertical" },
    { MT_H, N_("Horizontal Page Center"), "horizontal" },
    { MT_FREE, N_("Free from reflection line"), "free" },
    { MT_X, N_("X from middle knot"), "X" },
    { MT_Y, N_("Y from middle knot"), "Y" }
};
static const Util::EnumDataConverter<ModeType>
MTConverter(ModeTypeData, MT_END);

LPEMirrorSymmetry::LPEMirrorSymmetry(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    mode(_("Mode"), _("Symmetry move mode"), "mode", MTConverter, &wr, this, MT_FREE),
    split_gap(_("Gap on split"), _("Gap on split"), "split_gap", &wr, this, 0),
    discard_orig_path(_("Discard original path?"), _("Check this to only keep the mirrored part of the path"), "discard_orig_path", &wr, this, false),
    fuse_paths(_("Fuse paths"), _("Fuse original and the reflection into a single path"), "fuse_paths", &wr, this, false),
    oposite_fuse(_("Opposite fuse"), _("Picks the other side of the mirror as the original"), "oposite_fuse", &wr, this, false),
    split_elements(_("Split elements"), _("Split elements, this allow gradients and other paints. Group result to apply nested"), "split_elements", &wr, this, false),
    start_point(_("Start mirror line"), _("Start mirror line"), "start_point", &wr, this, _("Adjust the start of mirroring")),
    end_point(_("End mirror line"), _("End mirror line"), "end_point", &wr, this, _("Adjust end of mirroring")),
    center_point(_("Center mirror line"), _("Center mirror line"), "center_point", &wr, this, _("Adjust center of mirroring")),
    id_origin("hidden", "hidden store the id of the first LPEItem", "id_origin", &wr, this,"")
{
    show_orig_path = true;
    registerParameter(&mode);
    registerParameter(&split_gap);
    registerParameter(&discard_orig_path);
    registerParameter(&fuse_paths);
    registerParameter(&oposite_fuse);
    registerParameter(&split_elements);
    registerParameter(&start_point);
    registerParameter(&end_point);
    registerParameter(&center_point);
    registerParameter(&id_origin);
    id_origin.param_hide_canvas_text();
    split_gap.param_set_range(-999999.0, 999999.0);
    split_gap.param_set_increments(0.1, 0.1);
    split_gap.param_set_digits(2);
    apply_to_clippath_and_mask = true;
    previous_center = Geom::Point(0,0);
    other = NULL;
    last_transform = Geom::identity();
}

LPEMirrorSymmetry::~LPEMirrorSymmetry()
{
}

void
LPEMirrorSymmetry::doAfterEffect (SPLPEItem const* lpeitem)
{
    last_transform = Geom::identity();
    if (split_elements) {
        if (discard_orig_path) {
            discard_orig_path.param_setValue(false);
            discard_orig_path.write_to_SVG();
            std::cout << _("You can't discard original paths on split elements");
        }
        container = dynamic_cast<SPObject *>(sp_lpe_item->parent);
        SPDocument * doc = SP_ACTIVE_DOCUMENT;
        Inkscape::XML::Node *root = sp_lpe_item->document->getReprRoot();
        Inkscape::XML::Node *root_origin = doc->getReprRoot();
        if (root_origin != root) {
            return;
        }
        Geom::Line ls((Geom::Point)start_point, (Geom::Point)end_point);
        Geom::Affine m = Geom::reflection (ls.vector(), (Geom::Point)start_point);
        //        Geom::Point gap(split_gap,0);
//        Geom::Translate m1(point_a[0], point_a[1]);
//        double hyp = Geom::distance(point_a, point_b);
//        double cos = 0;
//        double sin = 0;
//        if (hyp > 0) {
//            cos = (point_b[0] - point_a[0]) / hyp;
//            sin = (point_b[1] - point_a[1]) / hyp;
//        }
//        Geom::Affine m2(cos, -sin, sin, cos, 0.0, 0.0);
//        Geom::Point dir = unit_vector(point_b - point_a);
//        Geom::Point offset = (point_a + point_b)/2 + Geom::rot90(dir.ccw()) * split_gap;
//        line_separation *= Geom::Translate(offset);
//        Geom::Scale sca(1.0, -1.0);
//        m = m1.inverse() * m2;
//        m = m * sca;
//        m = m * m2.inverse();
//        m = m * m1;
        m = m * sp_lpe_item->transform;
//        if (std::strcmp(sp_lpe_item->getId(), origin) != 0) {
            createMirror(m);
//        } else {
//            createMirror(sp_lpe_item, m, mirror);
//        }
    } else {
        processObjects(LPE_ERASE);
        elements.clear();
        other = NULL;
    }
}

void
LPEMirrorSymmetry::doBeforeEffect (SPLPEItem const* lpeitem)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::Selection *sel = desktop->getSelection();
        if ( sel && !sel->isEmpty()) {
            SPItem *item = sel->singleItem();
            if (item) {
                if(std::strcmp(sp_lpe_item->getId(),item->getId()) != 0) {
                    return;
                }
            }
        }
    }
    using namespace Geom;
    original_bbox(lpeitem);
    Point point_a(boundingbox_X.max(), boundingbox_Y.min());
    Point point_b(boundingbox_X.max(), boundingbox_Y.max());
    if (mode == MT_Y) {
        point_a = Geom::Point(boundingbox_X.min(),center_point[Y]);
        point_b = Geom::Point(boundingbox_X.max(),center_point[Y]);
    }
    if (mode == MT_X) {
        point_a = Geom::Point(center_point[X],boundingbox_Y.min());
        point_b = Geom::Point(center_point[X],boundingbox_Y.max());
    }
    if (Geom::are_near((Geom::Point)start_point, (Geom::Point)end_point, 0.01)) {
        start_point.param_setValue(point_a);
        end_point.param_setValue(point_b);
    }
    if ( mode == MT_X || mode == MT_Y ) {
        start_point.param_setValue(point_a);
        end_point.param_setValue(point_b);
    } else if ( mode == MT_FREE) {
        if(!are_near(previous_center,center_point, 0.01)) {
            Geom::Point trans = center_point - previous_center;
            start_point.param_setValue(start_point * trans);
            end_point.param_setValue(end_point * trans);
        }
    } else if ( mode == MT_V){
        if(SP_ACTIVE_DESKTOP){
            SPDocument * doc = SP_ACTIVE_DESKTOP->getDocument();
            Geom::Rect view_box_rect = doc->getViewBox();
            Geom::Point sp = Geom::Point(view_box_rect.width()/2.0, 0);
            sp *= i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(SP_ACTIVE_DESKTOP->currentLayer()->parent)) .inverse();
            start_point.param_setValue(sp);
            Geom::Point ep = Geom::Point(view_box_rect.width()/2.0, view_box_rect.height());
            ep *= i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(SP_ACTIVE_DESKTOP->currentLayer()->parent)) .inverse();
            end_point.param_setValue(ep);
        }
    } else { //horizontal page
        if(SP_ACTIVE_DESKTOP){
            SPDocument * doc = SP_ACTIVE_DESKTOP->getDocument();
            Geom::Rect view_box_rect = doc->getViewBox();
            Geom::Point sp = Geom::Point(0, view_box_rect.height()/2.0);
            sp *= i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(SP_ACTIVE_DESKTOP->currentLayer()->parent)) .inverse();
            start_point.param_setValue(sp);
            Geom::Point ep = Geom::Point(view_box_rect.width(), view_box_rect.height()/2.0);
            ep *= i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(SP_ACTIVE_DESKTOP->currentLayer()->parent)) .inverse();
            end_point.param_setValue(ep);
        }
    }
    
    previous_center = Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
    if (!are_near(previous_center, center_point,0.01)) {
        center_point.param_setValue(previous_center);
    }
}

//void
//LPEMirrorSymmetry::cloneAttrbutes(Inkscape::XML::Node * origin, Inkscape::XML::Node * dest, const char * first_attribute, ...) 
//{
//    va_list args;
//    va_start(args, first_attribute);

//    if ( origin->name() == "svg:g" && origin->childCount() == dest->childCount() ) {
//        Inkscape::XML::Node * node_it = origin->firstChild();
//        size_t index = 0;
//        while (node_it != origin->lastChild()) {
//            cloneAttrbutes(node_it, dest->nthChild(index), first_attribute, live, args); 
//            node_it = node_it->next(); 
//            index++;
//        }
//    }
//    while(const char * att = va_arg(args, const char *)) {
//        dest->setAttribute(att,origin->attribute(att));
//    }
//    va_end(args);
//}

void
LPEMirrorSymmetry::cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * first_attribute, ...) 
{
    va_list args;
    va_start(args, first_attribute);
    
    if ( SP_IS_GROUP(origin) && SP_IS_GROUP(dest) && SP_GROUP(origin)->getItemCount() == SP_GROUP(dest)->getItemCount() ) {
        std::vector< SPObject * > childs = origin->childList(true);
        size_t index = 0;
        for (std::vector<SPObject * >::iterator obj_it = childs.begin(); 
             obj_it != childs.end(); ++obj_it) {
            SPObject *dest_child = dest->nthChild(index); 
            cloneAttrbutes(*obj_it, dest_child, live, first_attribute, args); 
            index++;
        }
    }
    for (const char* att = first_attribute; att != NULL; att = va_arg(args, const char*)) {
        SPShape * shape =  SP_SHAPE(origin);
        if (shape) {
            if ( live && (att == "d" || att == "inkscape:original-d")) {
                SPCurve *c = NULL;
                if (att == "d") {
                    c = shape->getCurve();
                } else {
                    c = shape->getCurveBeforeLPE();
                }
                if (c) {
                    dest->getRepr()->setAttribute(att,sp_svg_write_path(c->get_pathvector()));
                    c->reset();
                    g_free(c);
                } else {
                    dest->getRepr()->setAttribute(att,NULL);
                }
            } else {
                dest->getRepr()->setAttribute(att,origin->getRepr()->attribute(att));
            }
        }
    }
    va_end(args);
}

void
LPEMirrorSymmetry::createMirror(Geom::Affine transform)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        const char * id_origin_char = id_origin.param_getSVGValue();
        const char * mirror = g_strdup(Glib::ustring("mirror-").append(id_origin_char).c_str());
        const char * elemref_id;
        if (std::strcmp(sp_lpe_item->getId(), mirror) == 0) {
            elemref_id = id_origin_char;
        } else {
            elemref_id = mirror;
        }
        elements.clear();
        elements.push_back(id_origin_char);
        elements.push_back(mirror);
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::URI SVGElem_uri(Glib::ustring("#").append(elemref_id).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref= NULL;
        Inkscape::XML::Node *phantom = NULL;
        if (elemref = SVGElemRef->getObject()) {
            phantom = elemref->getRepr();
        } else {
            phantom = sp_lpe_item->getRepr()->duplicate(xml_doc);
        }
        phantom->setAttribute("id", elemref_id);
        if (!elemref) {
            elemref = container->appendChildRepr(phantom);
            Inkscape::GC::release(phantom);
        }
        cloneAttrbutes(SP_OBJECT(sp_lpe_item), elemref, true, "inkscape:original-d", "d", "inkscape:path-effect", NULL); //NULL required
        //transform *= last_transform;
        //if (transform != Geom::identity()) {
            if (elemref_id == mirror) {
                elemref->getRepr()->setAttribute("transform" , sp_svg_transform_write(transform));
            } else {
                sp_lpe_item->getRepr()->setAttribute("transform" , sp_svg_transform_write(transform));
            }
        //}
        if (elemref->parent != container) {
            Inkscape::XML::Node *copy = phantom->duplicate(xml_doc);
            copy->setAttribute("id", elemref_id);
            other = container->appendChildRepr(copy);
            Inkscape::GC::release(copy);
            elemref->deleteObject();
        } else {
            other = elemref;
        }
        
    }
}

Gtk::Widget *
LPEMirrorSymmetry::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(Effect::newWidget()));
    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(2);

    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter * param = *it;
            if (param->param_key == "id_origin") {
                ++it;
                continue;
            }
            Gtk::Widget * widg = param->param_newWidget();
            Glib::ustring * tip = param->param_getTooltip();
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
    this->upd_params = false;
    return dynamic_cast<Gtk::Widget *>(vbox);
}

//TODO: Migrate the tree next function to effect.cpp/h to avoid duplication
void
LPEMirrorSymmetry::doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/)
{
    processObjects(LPE_VISIBILITY);
}

void 
LPEMirrorSymmetry::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    //unset "erase_extra_objects" hook on sp-lpe-item.cpp
    if (!erase_extra_objects) {
        processObjects(LPE_TO_OBJECTS);
        return;
    }
    processObjects(LPE_ERASE);
}

void 
LPEMirrorSymmetry::processObjects(LpeAction lpe_action)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        for (std::vector<const char *>::iterator el_it = elements.begin(); 
             el_it != elements.end(); ++el_it) {
            const char * id = *el_it;
            if (!id || strlen(id) == 0) {
                return;
            }
            Inkscape::URI SVGElem_uri(Glib::ustring("#").append(id).c_str());
            Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
            SVGElemRef->attach(SVGElem_uri);
            SPObject *elemref = NULL;
            if (elemref = SVGElemRef->getObject()) {
                Inkscape::XML::Node * elemnode = elemref->getRepr();
                std::vector<SPItem*> item_list;
                item_list.push_back(SP_ITEM(elemref));
                std::vector<Inkscape::XML::Node*> item_to_select;
                std::vector<SPItem*> item_selected;
                SPCSSAttr *css;
                Glib::ustring css_str;
                switch (lpe_action){
                case LPE_TO_OBJECTS:
                    if (elemnode->attribute("inkscape:path-effect")) {
                        sp_item_list_to_curves(item_list, item_selected, item_to_select);
                    }
                    elemnode->setAttribute("sodipodi:insensitive", NULL);
                    break;

                case LPE_ERASE:
                    //if (std::strcmp(elemref->getId(),sp_lpe_item->getId()) != 0) {
                        elemref->deleteObject();
                    //}
                    break;

                case LPE_VISIBILITY:
                    css = sp_repr_css_attr_new();
                    sp_repr_css_attr_add_from_string(css, elemref->getRepr()->attribute("style"));
                    if (!this->isVisible()/* && std::strcmp(elemref->getId(),sp_lpe_item->getId()) != 0*/) {
                        css->setAttribute("display", "none");
                    } else {
                        css->setAttribute("display", NULL);
                    }
                    sp_repr_css_write_string(css,css_str);
                    elemnode->setAttribute("style", css_str.c_str());
                    break;

                default:
                    break;
                }
            }
        }
        if (lpe_action == LPE_ERASE || lpe_action == LPE_TO_OBJECTS) {
            elements.clear();
        }
    }
}


void
LPEMirrorSymmetry::transform_multiply(Geom::Affine const& postmul, bool set)
{
    // cycle through all parameters. Most parameters will not need transformation, but path and point params do.
    for (std::vector<Parameter *>::iterator it = param_vector.begin(); it != param_vector.end(); ++it) {
        Parameter * param = *it;
        param->param_transform_multiply(postmul, set);
    }
    previous_center = Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
//    Geom::Affine m = Geom::identity();
//    m *= sp_lpe_item->transform;
//    m *= postmul;
//    sp_lpe_item->transform = m;
    last_transform *= postmul;
    sp_lpe_item_update_patheffect(sp_lpe_item, false, false);
//    if (other) {
//        sp_lpe_item_update_patheffect(SP_LPE_ITEM(other), false, false);
//    }
}

void
LPEMirrorSymmetry::doOnApply (SPLPEItem const* lpeitem)
{
    using namespace Geom;

    original_bbox(lpeitem);

    Point point_a(boundingbox_X.max(), boundingbox_Y.min());
    Point point_b(boundingbox_X.max(), boundingbox_Y.max());
    Point point_c(boundingbox_X.max(), boundingbox_Y.middle());
    start_point.param_setValue(point_a);
    start_point.param_update_default(point_a);
    end_point.param_setValue(point_b);
    end_point.param_update_default(point_b);
    center_point.param_setValue(point_c);
    previous_center = center_point;
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
    if (!lpeitem->hasPathEffectOfType(this->effectType(), false) ){ //first applied not ready yet
        id_origin.param_setValue(lpeitem->getRepr()->attribute("id"));
        id_origin.write_to_SVG();
    }
}


Geom::PathVector
LPEMirrorSymmetry::doEffect_path (Geom::PathVector const & path_in)
{
    if (split_elements && !fuse_paths) {
        return path_in;
    }
    Geom::PathVector const original_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    Geom::PathVector path_out;
    
    if (!discard_orig_path && !fuse_paths) {
        path_out = pathv_to_linear_and_cubic_beziers(path_in);
    }

    Geom::Line ls((Geom::Point)start_point, (Geom::Point)end_point);
    Geom::Affine m = Geom::reflection (ls.vector(), (Geom::Point)start_point);

    if (fuse_paths && !discard_orig_path) {
        for (Geom::PathVector::const_iterator path_it = original_pathv.begin();
             path_it != original_pathv.end(); ++path_it) 
        {
            if (path_it->empty()) {
                continue;
            }
            Geom::PathVector tmp_path;
            double time_start = 0.0;
            int position = 0;
            bool end_open = false;
            if (path_it->closed()) {
                const Geom::Curve &closingline = path_it->back_closed();
                if (!are_near(closingline.initialPoint(), closingline.finalPoint())) {
                    end_open = true;
                }
            }
            Geom::Path original = *path_it;
            if (end_open && path_it->closed()) {
                original.close(false);
                original.appendNew<Geom::LineSegment>( original.initialPoint() );
                original.close(true);
            }
            Geom::Point s = start_point;
            Geom::Point e = end_point;
            double dir = ls.angle();
            double diagonal = Geom::distance(Geom::Point(boundingbox_X.min(),boundingbox_Y.min()),Geom::Point(boundingbox_X.max(),boundingbox_Y.max()));
            Geom::Rect bbox(Geom::Point(boundingbox_X.min(),boundingbox_Y.min()),Geom::Point(boundingbox_X.max(),boundingbox_Y.max()));
            double size_divider = Geom::distance(center_point, bbox) + diagonal;
            s = Geom::Point::polar(dir,size_divider) + center_point;
            e = Geom::Point::polar(dir + Geom::rad_from_deg(180),size_divider) + center_point;
            Geom::Path divider = Geom::Path(s);
            divider.appendNew<Geom::LineSegment>(e);
            Geom::Crossings cs = crossings(original, divider);
            std::vector<double> crossed;
            for(unsigned int i = 0; i < cs.size(); i++) {
                crossed.push_back(cs[i].ta);
            }
            std::sort(crossed.begin(), crossed.end());
            for (unsigned int i = 0; i < crossed.size(); i++) {
                double time_end = crossed[i];
                if (time_start != time_end && time_end - time_start > Geom::EPSILON) {
                    Geom::Path portion = original.portion(time_start, time_end);
                    if (!portion.empty()) {
                        Geom::Point middle = portion.pointAt((double)portion.size()/2.0);
                        position = Geom::sgn(Geom::cross(e - s, middle - s));
                        if (!oposite_fuse) {
                            position *= -1;
                        }
                        if (position == 1) {
                            Geom::Path mirror = portion.reversed() * m;
                            if (split_elements) {
                                if(i!=0 || original.closed()) {
                                    portion.close();
                                }
                            } else {
                                mirror.setInitial(portion.finalPoint());
                                portion.append(mirror);
                                if(i!=0) {
                                    portion.setFinal(portion.initialPoint());
                                    portion.close();
                                }
                            }
                            tmp_path.push_back(portion);
                        }
                        portion.clear();
                    }
                }
                time_start = time_end;
            }
            position = Geom::sgn(Geom::cross(e - s, original.finalPoint() - s));
            if (!oposite_fuse) {
                position *= -1;
            }
            if (cs.size()!=0 && position == 1) {
                if (time_start != original.size() && original.size() - time_start > Geom::EPSILON) {
                    Geom::Path portion = original.portion(time_start, original.size());
                    if (!portion.empty()) {
                        portion = portion.reversed();
                        if (!split_elements) {
                            Geom::Path mirror = portion.reversed() * m;
                            mirror.setInitial(portion.finalPoint());
                            portion.append(mirror);
                            portion = portion.reversed();
                        }
                        if (!original.closed()) {
                            tmp_path.push_back(portion);
                        } else {
                            if (cs.size() > 1 && tmp_path.size() > 0 && tmp_path[0].size() > 0 ) {
                                portion.setFinal(tmp_path[0].initialPoint());
                                portion.setInitial(tmp_path[0].finalPoint());
                                tmp_path[0].append(portion);
                            } else {
                                tmp_path.push_back(portion);
                            }
                            tmp_path[0].close();
                        }
                        portion.clear();
                    }
                }
            }
            if (cs.size() == 0 && position == 1) {
                tmp_path.push_back(original);
                if ( !split_elements) {
                    tmp_path.push_back(original * m);
                }
            }
            path_out.insert(path_out.end(), tmp_path.begin(), tmp_path.end());
            tmp_path.clear();
        }
    } else if (!fuse_paths || discard_orig_path) {
        for (size_t i = 0; i < original_pathv.size(); ++i) {
            path_out.push_back(original_pathv[i] * m);
        }
    }

    return path_out;
}

void
LPEMirrorSymmetry::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
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
