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
    linkeditem(_("Linked Item:"), _("Item from which to take the original data"), "linkeditem", &wr, this),
    scale(_("Scale %"), _("Scale item %"), "scale", &wr, this, 100.0),
    preserve_position(_("Preserve position"), _("Preserve position"), "preserve_position", &wr, this, false),
    inverse(_("Inverse clone"), _("Use LPE item as origin"), "inverse", &wr, this, false),
    d(_("Clone shape -d-"), _("Clone shape -d-"), "d", &wr, this, true),
    transform(_("Clone transforms"), _("Clone transforms"), "transform", &wr, this, true),
    fill(_("Clone fill"), _("Clone fill"), "fill", &wr, this, false),
    stroke(_("Clone stroke"), _("Clone stroke"), "stroke", &wr, this, false),
    paintorder(_("Clone paint order"), _("Clone paint order"), "paintorder", &wr, this, false),
    opacity(_("Clone opacity"), _("Clone opacity"), "opacity", &wr, this, false),
    filter(_("Clone filter"), _("Clone filter"), "filter", &wr, this, false),
    attributes("Attributes linked", "Attributes linked, comma separated atributes", "attributes", &wr, this,""),
    style_attributes("Style attributes linked", "Style attributes linked, comma separated atributes", "style_attributes", &wr, this,""),
    expanded(false),
    origin(Geom::Point(0,0))
{
    //0.92 compatibility
    const gchar * linkedpath = this->getRepr()->attribute("linkedpath");
    if (linkedpath && strcmp(linkedpath, "") != 0){
        this->getRepr()->setAttribute("linkeditem", linkedpath);
        this->getRepr()->setAttribute("linkedpath", NULL);
        this->getRepr()->setAttribute("transform", "false");
    };

    registerParameter(&linkeditem);
    registerParameter(&scale);
    registerParameter(&attributes);
    registerParameter(&style_attributes);
    registerParameter(&preserve_position);
    registerParameter(&inverse);
    registerParameter(&d);
    registerParameter(&transform);
    registerParameter(&fill);
    registerParameter(&stroke);
    registerParameter(&paintorder);
    registerParameter(&opacity);
    registerParameter(&filter);
    scale.param_set_range(0.01, 999999.0);
    scale.param_set_increments(1, 1);
    scale.param_set_digits(2);
    attributes.param_hide_canvas_text();
    style_attributes.param_hide_canvas_text();
    preserve_position_changed = preserve_position;
    preserve_affine = Geom::identity();
}

void
LPECloneOriginal::cloneAttrbutes(SPObject *origin, SPObject *dest, bool live, const char * attributes, const char * style_attributes, bool root) 
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
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
                    gchar * str = sp_svg_transform_write(dest_affine);
                    SP_ITEM(dest)->getRepr()->setAttribute("transform", str);
                    g_free(str);
                }
            } else {
                gchar * str = sp_svg_transform_write(affine_origin);
                SP_ITEM(dest)->getRepr()->setAttribute("transform", str);
                g_free(str);
            }
        } else if ( shape_dest && shape_origin && live && (std::strcmp(attribute, "d") == 0)) {
            SPCurve *c = NULL;
            if (inverse) {
                c = shape_origin->getCurveBeforeLPE();
            } else {
                c = shape_origin->getCurve();
            }
            if (c) {
                Geom::PathVector c_pv = c->get_pathvector();
                Geom::OptRect orig_bbox = SP_ITEM(origin)->geometricBounds();
                Geom::OptRect dest_bbox = SP_ITEM(dest)->geometricBounds();
                if (dest_bbox && orig_bbox && root) {
                    Geom::Point orig_point = (*orig_bbox).corner(0);
                    Geom::Point dest_point = (*dest_bbox).corner(0);
                    if (scale != 100.0) {
                        double scale_affine = scale/100.0;
                        Geom::Scale scale = Geom::Scale(scale_affine);
                        c_pv *= Geom::Translate(orig_point).inverse();
                        c_pv *= scale;
                        c_pv *= Geom::Translate(orig_point);
                    }
                    if (preserve_position) {
                        c_pv *= Geom::Translate(dest_point - orig_point);
                    }
                }
                if (inverse) {
                    c_pv *= i2anc_affine(origin, sp_lpe_item);
                } else {
                    c_pv *= i2anc_affine(dest, sp_lpe_item);
                }
                c->set_pathvector(c_pv);
                if (!path_origin) {
                    shape_dest->setCurveInsync(c, TRUE);
                    gchar *str = sp_svg_write_path(c_pv);
                    dest->getRepr()->setAttribute(attribute, str);
                    g_free(str);
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
        if (!strlen(origin_attribute)) { //==0
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
    if (linkeditem.linksToItem()) {
        linkeditem.setInverse(inverse);
        if ( preserve_position_changed != preserve_position ) {
            if (!preserve_position) {
                sp_svg_transform_read(SP_ITEM(sp_lpe_item)->getAttribute("transform"), &preserve_affine);
            }
            preserve_position_changed = preserve_position;
        }
        Glib::ustring attr = "";
        if (d) {
            attr.append("d,");
        }
        if (transform) {
            attr.append("transform,");
        }
        attr.append(Glib::ustring(attributes.param_getSVGValue()).append(","));
        if (attr.size()  && !Glib::ustring(attributes.param_getSVGValue()).size()) {
            attr.erase (attr.size()-1, 1);
        }
        Glib::ustring style_attr = "";
        if (fill) {
            style_attr.append("fill,").append("fill-rule,");
        }
        if (stroke) {
            style_attr.append("stroke,").append("stroke-width,").append("stroke-linecap,").append("stroke-linejoin,");
            style_attr.append("stroke-opacity,").append("stroke-miterlimit,").append("stroke-dasharray,");
            style_attr.append("stroke-opacity,").append("stroke-dashoffset,").append("marker-start,");
            style_attr.append("marker-mid,").append("marker-end,");
        }
        if (paintorder) {
            style_attr.append("paint-order,");
        }
        if (filter) {
            style_attr.append("filter,");
        }
        if (opacity) {
            style_attr.append("opacity,");
        }
        if (style_attr.size() && !Glib::ustring(style_attributes.param_getSVGValue()).size()) {
            style_attr.erase (style_attr.size()-1, 1);
        }
        style_attr.append(Glib::ustring(style_attributes.param_getSVGValue()).append(","));

        SPItem * from =  inverse ? SP_ITEM(sp_lpe_item) : SP_ITEM(linkeditem.getObject()); 
        SPItem * to   = !inverse ? SP_ITEM(sp_lpe_item) : SP_ITEM(linkeditem.getObject()); 
        cloneAttrbutes(from, to, true, g_strdup(attr.c_str()), g_strdup(style_attr.c_str()), true);
        Geom::OptRect bbox = from->geometricBounds();
        if (bbox && preserve_position && origin != Geom::Point(0,0)) {
            origin = (*bbox).corner(0) - origin;
            to->transform *= Geom::Translate(origin);
        }
        bbox = from->geometricBounds();
        if (bbox && preserve_position) {
            origin = (*bbox).corner(0);
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
    Gtk::VBox * vbox_expander = Gtk::manage( new Gtk::VBox(Effect::newWidget()) );
    vbox_expander->set_border_width(0);
    vbox_expander->set_spacing(2);
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
                if (param->param_key != "attributes" &&
                    param->param_key != "style_attributes") {
                    vbox->pack_start(*widg, true, true, 2);
                } else {
                    vbox_expander->pack_start(*widg, true, true, 2);
                }
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
    expander = Gtk::manage(new Gtk::Expander(Glib::ustring(_("Show attributes override"))));
    expander->add(*vbox_expander);
    expander->set_expanded(expanded);
    expander->property_expanded().signal_changed().connect(sigc::mem_fun(*this, &LPECloneOriginal::onExpanderChanged) );
    vbox->pack_start(*expander, true, true, 2);
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void
LPECloneOriginal::onExpanderChanged()
{
    expanded = expander->get_expanded();
    if(expanded) {
        expander->set_label (Glib::ustring(_("Hide attributes override")));
    } else {
        expander->set_label (Glib::ustring(_("Show attributes override")));
    }
}

LPECloneOriginal::~LPECloneOriginal()
{

}

void
LPECloneOriginal::transform_multiply(Geom::Affine const& postmul, bool set)
{
    if (linkeditem.linksToItem()) {
        linkeditem.getObject()->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
}

void 
LPECloneOriginal::doEffect (SPCurve * curve)
{
    if (linkeditem.linksToItem() && !inverse) {
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
