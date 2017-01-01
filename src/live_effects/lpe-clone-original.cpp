/*
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-clone-original.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"
#include "sp-clippath.h"
#include "sp-mask.h"
#include "xml/sp-css-attr.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPECloneOriginal::LPECloneOriginal(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    linked_path("LEGACY FALLBACK", "LEGACY FALLBACK", "linkedpath", &wr, this),
    linked_item(_("Linked Item:"), _("Item from which to take the original data"), "linked_item", &wr, this),
    scale(_("Scale %"), _("Scale item %"), "scale", &wr, this, 100.0),
    preserve_position(_("Preserve position"), _("Preserve position"), "preserve_position", &wr, this, false),
    use_center(_("Relative center of element"), _("Relative center of element"), "use_center", &wr, this, true),
    attributes("Attributes linked", "Attributes linked", "attributes", &wr, this,""),
    style_attributes("Style attributes linked", "Style attributes linked", "style_attributes", &wr, this,"")
{
    registerParameter(&linked_path);
    registerParameter(&linked_item);
    registerParameter(&scale);
    registerParameter(&attributes);
    registerParameter(&style_attributes);
    registerParameter(&preserve_position);
    registerParameter(&use_center);
    scale.param_set_range(0.01, 999999.0);
    scale.param_set_increments(1, 1);
    scale.param_set_digits(2);
    attributes.param_hide_canvas_text();
    style_attributes.param_hide_canvas_text();
    preserve_position_changed = preserve_position;
    preserve_affine = Geom::identity();
}

bool hasLinkedTransform( const char * attributes) {
    gchar ** attarray = g_strsplit(attributes, ",", 0);
    gchar ** iter = attarray;
    bool has_linked_transform = false;
    while (*iter != NULL) {
        const char* attribute = (*iter);
        if ( std::strcmp(attribute, "transform") == 0 ) {
            has_linked_transform = true;
        }
        iter++;
    }
    return has_linked_transform;
}

void
LPECloneOriginal::cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * attributes, const char * style_attributes, bool root) 
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if ( SP_IS_GROUP(origin) && SP_IS_GROUP(dest) && SP_GROUP(origin)->getItemCount() == SP_GROUP(dest)->getItemCount() ) {
        std::vector< SPObject * > childs = origin->childList(true);
        size_t index = 0;
        for (std::vector<SPObject * >::iterator obj_it = childs.begin(); 
             obj_it != childs.end(); ++obj_it) {
            SPObject *dest_child = dest->nthChild(index); 
            cloneAttrbutes((*obj_it), dest_child, live, attributes, style_attributes, false); 
            index++;
        }
    }
    //Attributes
    SPShape * shape_origin =  SP_SHAPE(origin);
    SPPath * path_origin =  SP_PATH(origin);
    SPShape * shape_dest =  SP_SHAPE(dest);
    SPMask *mask_origin = SP_ITEM(origin)->mask_ref->getObject();
    SPMask *mask_dest = SP_ITEM(dest)->mask_ref->getObject();
    if(mask_origin && mask_dest) {
        std::vector<SPObject*> mask_list = mask_origin->childList(true);
        std::vector<SPObject*> mask_list_dest = mask_dest->childList(true);
        if (mask_list.size() == mask_list_dest.size()) {
            size_t i = 0;
            for ( std::vector<SPObject*>::const_iterator iter=mask_list.begin();iter!=mask_list.end();++iter) {
                SPObject * mask_data = *iter;
                SPObject * mask_dest_data = mask_list_dest[i];
                cloneAttrbutes(mask_data, mask_dest_data, live, attributes, style_attributes, false);
                i++;
            }
        }
    }
    SPClipPath *clippath_origin = SP_ITEM(origin)->clip_ref->getObject();
    SPClipPath *clippath_dest = SP_ITEM(dest)->clip_ref->getObject();
    if(clippath_origin && clippath_dest) {
        std::vector<SPObject*> clippath_list = clippath_origin->childList(true);
        std::vector<SPObject*> clippath_list_dest = clippath_dest->childList(true);
        if (clippath_list.size() == clippath_list_dest.size()) {
            size_t i = 0;
            for ( std::vector<SPObject*>::const_iterator iter=clippath_list.begin();iter!=clippath_list.end();++iter) {
                SPObject * clippath_data = *iter;
                SPObject * clippath_dest_data = clippath_list_dest[i];
                cloneAttrbutes(clippath_data, clippath_dest_data, live, attributes, style_attributes, false);
                i++;
            }
        }
    }
    gchar ** attarray = g_strsplit(attributes, ",", 0);
    gchar ** iter = attarray;
    Geom::Affine affine_dest = Geom::identity();
    Geom::Affine affine_origin = Geom::identity();
    Geom::Affine affine_previous = Geom::identity();
    sp_svg_transform_read(SP_ITEM(dest)->getAttribute("transform"), &affine_dest);
    sp_svg_transform_read(SP_ITEM(origin)->getAttribute("transform"), &affine_origin);
    while (*iter != NULL) {
        const char* attribute = (*iter);
        if ( std::strcmp(attribute, "transform") == 0 ) {
            if (preserve_position) {
                Geom::Affine dest_affine = Geom::identity();
                if (root) {
                    dest_affine *= affine_origin;
                    if (preserve_affine == Geom::identity()) {
                        dest_affine *= Geom::Translate(affine_dest.translation());
                    }
                    dest_affine *= Geom::Translate(affine_origin.translation()).inverse();
                    dest_affine *= Geom::Translate(preserve_affine.translation());
                    affine_previous = preserve_affine;
                    preserve_affine = Geom::identity();
                    SP_ITEM(dest)->getRepr()->setAttribute("transform",sp_svg_transform_write(dest_affine));
                }
            } else {
                SP_ITEM(dest)->getRepr()->setAttribute("transform",sp_svg_transform_write(affine_origin));
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
                Geom::OptRect orig_bbox = SP_ITEM(origin)->geometricBounds();
                Geom::OptRect dest_bbox = SP_ITEM(dest)->geometricBounds();
                if (dest_bbox && orig_bbox && root) {
                    Geom::Point orig_point = (*orig_bbox).corner(0);
                    Geom::Point dest_point = (*dest_bbox).corner(0);
                    if (use_center) {
                        orig_point = (*orig_bbox).midpoint();
                        dest_point = (*dest_bbox).midpoint();
                    }
                    if (scale != 100.0) {
                        double scale_affine = scale/100.0;
                        Geom::Scale scale = Geom::Scale(scale_affine);
                        c_pv *= Geom::Translate(orig_point).inverse();
                        c_pv *= scale;
                        c_pv *= Geom::Translate(orig_point);
                    }
                    if (preserve_position && hasLinkedTransform(attributes)) {
                        c_pv *= Geom::Translate(dest_point - orig_point);
                    }
                }
                c_pv *= i2anc_affine(dest, sp_lpe_item);
                c->set_pathvector(c_pv);
                if (!path_origin) {
                    shape_dest->setCurveInsync(c, TRUE);
                    dest->getRepr()->setAttribute(attribute, sp_svg_write_path(c_pv));
                } else {
                    shape_dest->setCurve(c, TRUE);
                }
                c->unref();
            } else {
                dest->getRepr()->setAttribute(attribute, NULL);
            }
        } else {
            dest->getRepr()->setAttribute(attribute, origin->getRepr()->attribute(attribute));
        }
        iter++;
    }
    g_strfreev (attarray);
    SPCSSAttr *css_origin = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css_origin, origin->getRepr()->attribute("style"));
    SPCSSAttr *css_dest = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css_dest, dest->getRepr()->attribute("style"));
    gchar ** styleattarray = g_strsplit(style_attributes, ",", 0);
    gchar ** styleiter = styleattarray;
    while (*styleiter != NULL) {
        const char* attribute = (*styleiter);
        const char* origin_attribute = sp_repr_css_property(css_origin, attribute, "");
        if (origin_attribute == "") {
            sp_repr_css_set_property (css_dest, attribute, NULL);
        } else {
            sp_repr_css_set_property (css_dest, attribute, origin_attribute);
        }
        styleiter++;
    }
    g_strfreev (styleattarray);
    Glib::ustring css_str;
    sp_repr_css_write_string(css_dest,css_str);
    dest->getRepr()->setAttribute("style", css_str.c_str());
}

void
LPECloneOriginal::doBeforeEffect (SPLPEItem const* lpeitem){
    original_bbox(lpeitem);
    if (linked_path.linksToPath()) { //Legacy staff
        Glib::ustring attributes_value("d");
        attributes.param_setValue(attributes_value);
        attributes.write_to_SVG();
        Glib::ustring style_attributes_value("");
        style_attributes.param_setValue(style_attributes_value);
        style_attributes.write_to_SVG();
        linked_item.param_readSVGValue(linked_path.param_getSVGValue());
        linked_path.param_readSVGValue("");
    }

    if (linked_item.linksToItem()) {
         if ( preserve_position_changed != preserve_position ) {
            if (!preserve_position) {
                sp_svg_transform_read(SP_ITEM(sp_lpe_item)->getAttribute("transform"), &preserve_affine);
            }
            preserve_position_changed = preserve_position;
        }
        cloneAttrbutes(linked_item.getObject(), SP_OBJECT(sp_lpe_item), true, attributes.param_getSVGValue(), style_attributes.param_getSVGValue(), true);
    }
}


Gtk::Widget *
LPECloneOriginal::newWidget()
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
            if (param->param_key == "linkedpath") {
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

void
LPECloneOriginal::doOnApply(SPLPEItem const* lpeitem){
    Glib::ustring attributes_value("d,transform");
    attributes.param_setValue(attributes_value);
    attributes.write_to_SVG();
    Glib::ustring style_attributes_value("opacity,stroke-width");
    style_attributes.param_setValue(style_attributes_value);
    style_attributes.write_to_SVG();
}

LPECloneOriginal::~LPECloneOriginal()
{

}

void
LPECloneOriginal::transform_multiply(Geom::Affine const& postmul, bool set)
{
    if (linked_item.linksToItem()) {
        bool changed = false;
        linked_item.getObject()->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
}

void 
LPECloneOriginal::doEffect (SPCurve * curve)
{
    if (linked_item.linksToItem()) {
        SPShape * shape = getCurrentShape();
        if(shape){
            curve->set_pathvector(shape->getCurve()->get_pathvector());
        }
    }
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
