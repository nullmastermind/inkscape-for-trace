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
#include "live_effects/lpe-mirror_symmetry.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"
#include "sp-defs.h"
#include "helper/geom.h"
#include "2geom/path-intersection.h"
#include "2geom/affine.h"
#include "uri.h"
#include "uri-references.h"
#include "knotholder.h"
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

std::vector<Glib::ustring> ms_elements;
SPObject * ms_container;

namespace MS {

class KnotHolderEntityCenterMirrorSymmetry : public LPEKnotHolderEntity {
public:
    KnotHolderEntityCenterMirrorSymmetry(LPEMirrorSymmetry *effect) : LPEKnotHolderEntity(effect){};
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
    virtual Geom::Point knot_get() const;
};

class KnotHolderEntityStartMirrorSymmetry : public LPEKnotHolderEntity {
public:
    KnotHolderEntityStartMirrorSymmetry(LPEMirrorSymmetry *effect) : LPEKnotHolderEntity(effect){};
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
    virtual Geom::Point knot_get() const;
};

class KnotHolderEntityEndMirrorSymmetry : public LPEKnotHolderEntity {
public:
    KnotHolderEntityEndMirrorSymmetry(LPEMirrorSymmetry *effect) : LPEKnotHolderEntity(effect){};
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
    virtual Geom::Point knot_get() const;
};

} // namespace MS

LPEMirrorSymmetry::LPEMirrorSymmetry(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    mode(_("Mode"), _("Symmetry move mode"), "mode", MTConverter, &wr, this, MT_FREE),
    discard_orig_path(_("Discard original path?"), _("Check this to only keep the mirrored part of the path"), "discard_orig_path", &wr, this, false),
    fuse_paths(_("Fuse paths"), _("Fuse original and the reflection into a single path"), "fuse_paths", &wr, this, false),
    oposite_fuse(_("Opposite fuse"), _("Picks the other side of the mirror as the original"), "oposite_fuse", &wr, this, false),
    split(_("Split elements"), _("Split elements, this allow gradients and other paints"), "split", &wr, this, false),
    split_sensitive(_("Make split selectable"), _("Allow select splited elements"), "split", &wr, this, false),
    split_style(_("Make splits with style"), _("Allow Change colors to split"), "split", &wr, this, false),
    start_point(_("Start mirror line"), _("Start mirror line"), "start_point", &wr, this, "Adjust the start of mirroring"),
    end_point(_("End mirror line"), _("End mirror line"), "end_point", &wr, this, "Adjust end of mirroring")
{
    show_orig_path = true;
    registerParameter(&mode);
    registerParameter( &discard_orig_path);
    registerParameter( &fuse_paths);
    registerParameter( &oposite_fuse);
    registerParameter( &split);
    registerParameter( &split_sensitive);
    registerParameter( &split_style);
    registerParameter( &start_point);
    registerParameter( &end_point);
    apply_to_clippath_and_mask = true;
}

LPEMirrorSymmetry::~LPEMirrorSymmetry()
{
}

void

LPEMirrorSymmetry::doBeforeEffect (SPLPEItem const* lpeitem)
{
    using namespace Geom;
    original_bbox(lpeitem);
    Point point_a(boundingbox_X.max(), boundingbox_Y.min());
    Point point_b(boundingbox_X.max(), boundingbox_Y.max());
    Point point_c(boundingbox_X.max(), boundingbox_Y.middle());
    if (mode == MT_Y) {
        point_a = Geom::Point(boundingbox_X.min(),center_point[Y]);
        point_b = Geom::Point(boundingbox_X.max(),center_point[Y]);
    }
    if (mode == MT_X) {
        point_a = Geom::Point(center_point[X],boundingbox_Y.min());
        point_b = Geom::Point(center_point[X],boundingbox_Y.max());
    }
    line_separation.setPoints(point_a, point_b);
    if ( mode == MT_X || mode == MT_Y ) {
        start_point.param_setValue(point_a);
        end_point.param_setValue(point_b);
        center_point = Geom::middle_point(point_a, point_b);
    } else if ( mode == MT_FREE) {
        if(!are_near(previous_center,center_point, 0.01)) {
            Geom::Point trans = center_point - previous_center;
            start_point.param_setValue(start_point * trans);
            end_point.param_setValue(end_point * trans);
            line_separation.setPoints(start_point, end_point);
        } else {
            center_point = Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
            line_separation.setPoints(start_point, end_point);
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
            center_point = Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
            line_separation.setPoints(start_point, end_point);
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
            center_point = Geom::middle_point((Geom::Point)start_point, (Geom::Point)end_point);
            line_separation.setPoints(start_point, end_point);
        }
    }
    previous_center = center_point;
    if (split) {
        SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
        ms_container = dynamic_cast<SPObject *>(splpeitem->parent);
        SPDocument * doc = SP_ACTIVE_DOCUMENT;
        Inkscape::XML::Node *root = splpeitem->document->getReprRoot();
        Inkscape::XML::Node *root_origin = doc->getReprRoot();
        if (root_origin != root) {
            return;
        }
        Geom::Point point_a(line_separation.initialPoint());
        Geom::Point point_b(line_separation.finalPoint());

        Geom::Translate m1(point_a[0], point_a[1]);
        double hyp = Geom::distance(point_a, point_b);
        double cos = 0;
        double sin = 0;
        if (hyp > 0) {
            cos = (point_b[0] - point_a[0]) / hyp;
            sin = (point_b[1] - point_a[1]) / hyp;
        }
        Geom::Affine m2(cos, -sin, sin, cos, 0.0, 0.0);
        Geom::Scale sca(1.0, -1.0);

        Geom::Affine m = m1.inverse() * m2;
        m = m * sca;
        m = m * m2.inverse();
        m = m * m1;
        createPhantom(splpeitem , (Glib::ustring) "origin-clon-", split_style);
        createClone(splpeitem , m, (Glib::ustring) "clon-");
    }
}

void
LPEMirrorSymmetry::createPhantom(SPLPEItem *origin, Glib::ustring id, bool styling)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        id = id + (Glib::ustring)this->getRepr()->attribute("id");
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + id).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *phantom = origin->getRepr()->duplicate(xml_doc);
        if (elemref = SVGElemRef->getObject()) {
            elemref->deleteObject();
        }
        if (styling) {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property (css, "fill", NULL);
            sp_repr_css_set_property (css, "stroke",NULL);
            Glib::ustring css_str;
            sp_repr_css_write_string(css,css_str);
            sp_repr_css_change_recursive (phantom, css, css_str.c_str());
        }
        phantom->setAttribute("id", id);
        phantom->setAttribute("inkscape:path-effect", NULL);
        SP_OBJECT(desktop->getDocument()->getDefs()->appendChildRepr(phantom));
        Inkscape::GC::release(phantom);
        ms_elements.push_back(id);
    }
}

void
LPEMirrorSymmetry::createClone(SPLPEItem *origin, Geom::Affine transform, Glib::ustring id)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        id = id + (Glib::ustring)this->getRepr()->attribute("id");
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + id).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *use = NULL;
        use = xml_doc->createElement("svg:use");
        use->setAttribute("transform" , sp_svg_transform_write(transform));
        use->setAttribute("xlink:href", ((Glib::ustring)"#origin-" + id).c_str());
        use->setAttribute("width", "100%");
        use->setAttribute("height", "100%");
        use->setAttribute("id", id);
        use->setAttribute("x", "0");
        use->setAttribute("y", "0");
        if (split_sensitive) {
            use->setAttribute("sodipodi:insensitive" , "false");
        } else {
            use->setAttribute("sodipodi:insensitive" , "true");
        }
        if (elemref) {
            elemref->deleteObject();
        }
        elemref = ms_container->appendChildRepr(use);
        Inkscape::GC::release(use);
        ms_elements.push_back(id);
    }
}


void
LPEMirrorSymmetry::doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/)
{
    processObjects(LPE_VISIBILITY);
}

void 
LPEMirrorSymmetry::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    if (!erase_extra_objects) {
        processObjects(LPE_TO_OBJECTS);
        ms_elements.clear();
        return;
    }
    processObjects(LPE_ERASE);
}

void 
LPEMirrorSymmetry::processObjects(LpeAction lpe_action)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        for (std::vector<Glib::ustring>::iterator el_it = ms_elements.begin(); 
             el_it != ms_elements.end();++el_it) {
            Glib::ustring id = *el_it;
            Inkscape::URI SVGElem_uri(((Glib::ustring)"#" +  id).c_str());
            Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
            SVGElemRef->attach(SVGElem_uri);
            SPObject *elemref = NULL;
            if (elemref = SVGElemRef->getObject()) {
                switch (lpe_action){
                case LPE_TO_OBJECTS:
                    elemref->getRepr()->setAttribute("sodipodi:insensitive", NULL);
                break;
                case LPE_ERASE:
                    elemref->deleteObject();
                break;
                default: //LPE_VISIBILITY
                    if (!this->isVisible()) {
                        elemref->getRepr()->setAttribute("style", "display:none");
                    } else {
                        elemref->getRepr()->setAttribute("style", NULL);
                    }
                break;
                }
            }
        }
        if (lpe_action == LPE_ERASE) {
            ms_elements.clear();
        }
    }
}


void
LPEMirrorSymmetry::transform_multiply(Geom::Affine const& postmul, bool set)
{
    center_point *= postmul;
    previous_center = center_point;
    // cycle through all parameters. Most parameters will not need transformation, but path and point params do.
    for (std::vector<Parameter *>::iterator it = param_vector.begin(); it != param_vector.end(); ++it) {
        Parameter * param = *it;
        param->param_transform_multiply(postmul, set);
    }
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
    center_point = point_c;
    previous_center = center_point;
}


Geom::PathVector
LPEMirrorSymmetry::doEffect_path (Geom::PathVector const & path_in)
{
    if (split && !fuse_paths) {
        return path_in;
    }
    Geom::PathVector const original_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    Geom::PathVector path_out;
    
    if (!discard_orig_path && !fuse_paths) {
        path_out = pathv_to_linear_and_cubic_beziers(path_in);
    }

    Geom::Point point_a(line_separation.initialPoint());
    Geom::Point point_b(line_separation.finalPoint());

    Geom::Translate m1(point_a[0], point_a[1]);
    double hyp = Geom::distance(point_a, point_b);
    double cos = 0;
    double sin = 0;
    if (hyp > 0) {
        cos = (point_b[0] - point_a[0]) / hyp;
        sin = (point_b[1] - point_a[1]) / hyp;
    }
    Geom::Affine m2(cos, -sin, sin, cos, 0.0, 0.0);
    Geom::Scale sca(1.0, -1.0);

    Geom::Affine m = m1.inverse() * m2;
    m = m * sca;
    m = m * m2.inverse();
    m = m * m1;

    if (fuse_paths && (!discard_orig_path || split )) {
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
            double dir = line_separation.angle();
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
                            if (split) {
                                portion.close();
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
                        if (split) {
                        
                        } else {
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
                tmp_path.push_back(original * m);
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

void
LPEMirrorSymmetry::addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop, SPItem *item)
{
    SPKnotShapeType knot_shape =  SP_KNOT_SHAPE_CIRCLE;
    SPKnotModeType knot_mode = SP_KNOT_MODE_XOR;
    guint32 knot_color = 0x0000ff00;
    {
        KnotHolderEntity *c = new MS::KnotHolderEntityCenterMirrorSymmetry(this);
        c->create( desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN,
                   _("Adjust the center"), knot_shape, knot_mode, knot_color );
        knotholder->add(c);
    }
};

namespace MS {

using namespace Geom;

void
KnotHolderEntityCenterMirrorSymmetry::knot_set(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    LPEMirrorSymmetry* lpe = dynamic_cast<LPEMirrorSymmetry *>(_effect);
    Geom::Point const s = snap_knot_position(p, state);
    lpe->center_point = s;

    // FIXME: this should not directly ask for updating the item. It should write to SVG, which triggers updating.
    sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, true);
}

Geom::Point
KnotHolderEntityCenterMirrorSymmetry::knot_get() const
{
    LPEMirrorSymmetry const *lpe = dynamic_cast<LPEMirrorSymmetry const*>(_effect);
    return lpe->center_point;
}

} // namespace CR

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
