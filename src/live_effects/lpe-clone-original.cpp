/*
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-clone-original.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"
#include <boost/algorithm/string.hpp>
#include "xml/sp-css-attr.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPECloneOriginal::LPECloneOriginal(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    linked_item(_("Linked Item:"), _("Item from which to take the original data"), "linked_item", &wr, this),
    preserve_position(_("Preserve position"), _("Preserve position"), "preserve_position", &wr, this, false),
    attributes("Attributes linked", "Attributes linked", "attributes", &wr, this,""),
    style_attributes("Style attributes linked", "Style attributes linked", "style_attributes", &wr, this,"")
{
    registerParameter(&linked_item);
    registerParameter(&attributes);
    registerParameter(&style_attributes);
    registerParameter(&preserve_position);
    attributes.param_hide_canvas_text();
    style_attributes.param_hide_canvas_text();
    apply_to_clippath_and_mask = true;
}

void
LPECloneOriginal::cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * first_attribute, ...) 
{
    va_list args;
    va_start(args, first_attribute);
    SPDocument * document = SP_ACTIVE_DOCUMENT;
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
    SPShape * shape_origin =  SP_SHAPE(origin);
    SPShape * shape_dest =  SP_SHAPE(dest);
    for (const char* att = first_attribute; att != NULL; att = va_arg(args, const char*)) {
        std::vector<std::string> elems;
        boost::split(elems, att, boost::is_any_of(","));
        for (std::vector<std::string>::const_iterator atts = elems.begin();
            atts != elems.end(); ++atts) {
            const char* attribute = (*atts).c_str();
            if ( std::strcmp(attribute, "transform") == 0 ) {
                Geom::Affine affine_dest = Geom::identity();
                sp_svg_transform_read(SP_ITEM(dest)->getAttribute("transform"), &affine_dest);
                dest->getRepr()->setAttribute(attribute, origin->getRepr()->attribute(attribute));
                if (preserve_position) {
                    Geom::Affine affine_origin = Geom::identity();
                    sp_svg_transform_read(SP_ITEM(origin)->getAttribute("transform"), &affine_origin);
                    SP_ITEM(dest)->transform = Geom::Translate(affine_dest.translation()) * Geom::Translate(affine_origin.translation()).inverse() * affine_origin;
                }
            } else if ( shape_dest && shape_origin && live && (std::strcmp(attribute, "d") == 0 || std::strcmp(attribute, "inkscape:original-d") == 0)) {
                SPCurve *c = NULL;
                if (std::strcmp(attribute, "d") == 0) {
                    c = shape_origin->getCurve();
                } else {
                    c = shape_origin->getCurveBeforeLPE();
                }
                if (c) {
                    Geom::PathVector c_pv = c->get_pathvector();
                    if (preserve_position) {
                        Geom::OptRect orig_bbox = SP_ITEM(origin)->geometricBounds();
                        if (orig_bbox) {
                            c_pv *= Geom::Translate(Geom::Point(boundingbox_X.min(), boundingbox_Y.min()) - (*orig_bbox).corner(0));
                        }
                    }
                    c->set_pathvector(c_pv);
                    shape_dest->setCurveInsync(c, TRUE);
                    dest->getRepr()->setAttribute(attribute, sp_svg_write_path(c_pv));
                    c->unref();
                } else {
                    dest->getRepr()->setAttribute(attribute, NULL);
                }
            } else {
                dest->getRepr()->setAttribute(attribute, origin->getRepr()->attribute(attribute));
            }
        }
    }
    va_end(args);
}

void
LPECloneOriginal::cloneStyleAttrbutes(SPObject *origin, SPObject *dest, const char * first_attribute, ...) 
{
    va_list args;
    va_start(args, first_attribute);
    
    if ( SP_IS_GROUP(origin) && SP_IS_GROUP(dest) && SP_GROUP(origin)->getItemCount() == SP_GROUP(dest)->getItemCount() ) {
        std::vector< SPObject * > childs = origin->childList(true);
        size_t index = 0;
        for (std::vector<SPObject * >::iterator obj_it = childs.begin(); 
             obj_it != childs.end(); ++obj_it) {
            SPObject *dest_child = dest->nthChild(index); 
            cloneStyleAttrbutes(*obj_it, dest_child, first_attribute, args); 
            index++;
        }
    }
    SPCSSAttr *css_origin = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css_origin, origin->getRepr()->attribute("style"));
    SPCSSAttr *css_dest = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css_dest, dest->getRepr()->attribute("style"));
    for (const char* att = first_attribute; att != NULL; att = va_arg(args, const char*)) {
        std::vector<std::string> elems;
        boost::split(elems, att, boost::is_any_of(","));
        for (std::vector<std::string>::const_iterator atts = elems.begin();
            atts != elems.end(); ++atts) {
            const char* attribute = (*atts).c_str();
            const char* origin_attribute = sp_repr_css_property(css_origin, attribute, "");
            if (origin_attribute == "") {
                sp_repr_css_set_property (css_dest, attribute, NULL);
            } else {
                sp_repr_css_set_property (css_dest, attribute, origin_attribute);
            }
        }
        Glib::ustring css_str;
        sp_repr_css_write_string(css_dest,css_str);
        dest->getRepr()->setAttribute("style", css_str.c_str());
    }
    va_end(args);
}

void
LPECloneOriginal::doBeforeEffect (SPLPEItem const* lpeitem){
    original_bbox(lpeitem);
    if (linked_item.linksToItem() && sp_lpe_item) {
        cloneAttrbutes(linked_item.getObject(), SP_OBJECT(sp_lpe_item), true, attributes.param_getSVGValue(), NULL); //NULL required
        cloneStyleAttrbutes(linked_item.getObject(), SP_OBJECT(sp_lpe_item), style_attributes.param_getSVGValue(), NULL); //NULL required
        SPShape * shape = dynamic_cast<SPShape *>(sp_lpe_item);
        if(shape){
            this->setSPCurve(shape->getCurve());
        }
    }
}

void
LPECloneOriginal::doOnApply(SPLPEItem const* lpeitem){
    Glib::ustring attributes_value("d,transform");
    attributes.param_setValue(attributes_value);
    attributes.write_to_SVG();
    Glib::ustring style_attributes_value("opacity,border-width");
    style_attributes.param_setValue(style_attributes_value);
    style_attributes.write_to_SVG();
}

void
LPECloneOriginal::doAfterEffect (SPLPEItem const* lpeitem){
}

LPECloneOriginal::~LPECloneOriginal()
{

}

void LPECloneOriginal::doEffect (SPCurve * curve)
{
//    std::vector<std::string> elems;
//    const char * attrs = attributes.param_getSVGValue();
//    boost::split(elems, attrs, boost::is_any_of(","));
//    bool has_d = false;
//    for (std::vector<std::string>::const_iterator atts = elems.begin();
//        atts != elems.end(); ++atts) {
//        const char* attribute = (*atts).c_str();
//        if (std::strcmp(attribute, "d") == 0) {
//            has_d = true;
//        }
//    }
//    if (linked_item.linksToItem() && has_d) {
//        curve->reset();
//    }
    curve->set_pathvector(pathvector_before_effect);
}

} // namespace LivePathEffect
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
