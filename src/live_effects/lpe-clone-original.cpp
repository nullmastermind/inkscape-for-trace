/*
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-clone-original.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"
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
    attributes("Attributes linked", "Attributes linked", "attributes", &wr, this,""),
    style_attributes("Style attributes linked", "Style attributes linked", "style_attributes", &wr, this,"")
{
    registerParameter(&linked_path);
    registerParameter(&linked_item);
    registerParameter(&scale);
    registerParameter(&attributes);
    registerParameter(&style_attributes);
    registerParameter(&preserve_position);
    scale.param_set_range(0.01, 999999.0);
    scale.param_set_increments(1, 1);
    scale.param_set_digits(2);
    attributes.param_hide_canvas_text();
    style_attributes.param_hide_canvas_text();
    apply_to_clippath_and_mask = true;
}

void
LPECloneOriginal::cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * attributes, const char * style_attributes) 
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if ( SP_IS_GROUP(origin) && SP_IS_GROUP(dest) && SP_GROUP(origin)->getItemCount() == SP_GROUP(dest)->getItemCount() ) {
        std::vector< SPObject * > childs = origin->childList(true);
        size_t index = 0;
        for (std::vector<SPObject * >::iterator obj_it = childs.begin(); 
             obj_it != childs.end(); ++obj_it) {
            SPObject *dest_child = dest->nthChild(index); 
            cloneAttrbutes(*obj_it, dest_child, live, attributes, style_attributes); 
            index++;
        }
    }
    //Attributes
    SPShape * shape_origin =  SP_SHAPE(origin);
    SPShape * shape_dest =  SP_SHAPE(dest);
    gchar ** attarray = g_strsplit(attributes, ",", 0);
    gchar ** iter = attarray;
    while (*iter != NULL) {
        const char* attribute = (*iter);
        if ( std::strcmp(attribute, "transform") == 0 ) {
            Geom::Affine affine_dest = SP_ITEM(dest)->transform;
            Geom::Affine affine_origin = SP_ITEM(origin)->transform;
            //dest->getRepr()->setAttribute(attribute, origin->getRepr()->attribute(attribute));
            if (preserve_position) {
                SP_ITEM(dest)->transform = Geom::Translate(affine_dest.translation()) * Geom::Translate(affine_origin.translation()).inverse() * affine_origin ;
            } else {
                SP_ITEM(dest)->transform = affine_origin ;
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
                if (orig_bbox) {
                    if (scale != 100.0) {
                        double scale_affine = scale/100.0;
                        Geom::Scale scale = Geom::Scale(scale_affine);
                        c_pv *= Geom::Translate((*orig_bbox).midpoint()).inverse();
                        c_pv *= scale;
                        c_pv *= Geom::Translate((*orig_bbox).midpoint());
                    }
                    if (preserve_position) {
                        c_pv *= Geom::Translate(Geom::Point(boundingbox_X.middle(), boundingbox_Y.middle()) - (*orig_bbox).midpoint());
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
        iter++;
    }
    g_strfreev (attarray);

    //Style Attributes
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
        cloneAttrbutes(linked_item.getObject(), SP_OBJECT(sp_lpe_item), true, attributes.param_getSVGValue(), style_attributes.param_getSVGValue());
        SPShape * shape = dynamic_cast<SPShape *>(sp_lpe_item);
        if(shape){
            this->setSPCurve(shape->getCurve());
        }
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
    Glib::ustring style_attributes_value("opacity,border-width");
    style_attributes.param_setValue(style_attributes_value);
    style_attributes.write_to_SVG();
}

LPECloneOriginal::~LPECloneOriginal()
{

}

void LPECloneOriginal::doEffect (SPCurve * curve)
{
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
