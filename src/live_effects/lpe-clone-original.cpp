/*
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-clone-original.h"
#include "live_effects/lpe-spiro.h"
#include "live_effects/lpe-bspline.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"

#include "object/sp-clippath.h"
#include "object/sp-mask.h"
#include "object/sp-path.h"
#include "object/sp-shape.h"

#include "xml/sp-css-attr.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<Clonelpemethod> ClonelpemethodData[] = {
    { CLM_NONE, N_("No shape"), "none" },
    { CLM_ORIGINALD, N_("Without LPE's"), "originald" }, 
    { CLM_BSPLINESPIRO, N_("With Spiro or BSpline"), "bsplinespiro" },
    { CLM_D, N_("With LPE's"), "d" }
};
static const Util::EnumDataConverter<Clonelpemethod> CLMConverter(ClonelpemethodData, CLM_END);

LPECloneOriginal::LPECloneOriginal(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    linkeditem(_("Linked Item:"), _("Item from which to take the original data"), "linkeditem", &wr, this),
    method(_("Shape linked"), _("Shape linked"), "method", CLMConverter, &wr, this, CLM_D),
    attributes("Attributes linked", "Attributes linked, comma separated attributes", "attributes", &wr, this,""),
    style_attributes("Style attributes linked", "Style attributes linked, comma separated attributes like fill, filter, opacity", "style_attributes", &wr, this,""),
    allow_transforms(_("Allow transforms"), _("Allow transforms"), "allow_transforms", &wr, this, true)
{
    //0.92 compatibility
    const gchar * linkedpath = this->getRepr()->attribute("linkedpath");
    if (linkedpath && strcmp(linkedpath, "") != 0){
        this->getRepr()->setAttribute("linkeditem", linkedpath);
        this->getRepr()->setAttribute("linkedpath", nullptr);
        this->getRepr()->setAttribute("method", "bsplinespiro");
        this->getRepr()->setAttribute("allow_transforms", "false");
    };
    is_updating = false;
    listening = false;
    linked = this->getRepr()->attribute("linkeditem");
    registerParameter(&linkeditem);
    registerParameter(&method);
    registerParameter(&attributes);
    registerParameter(&style_attributes);
    registerParameter(&allow_transforms);
    previus_method = method;
    attributes.param_hide_canvas_text();
    style_attributes.param_hide_canvas_text();
}

void
LPECloneOriginal::cloneAttrbutes(SPObject *origin, SPObject *dest, const gchar * attributes, const gchar * style_attributes) 
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document || !origin || !dest) {
        return;
    }
    if ( SP_IS_GROUP(origin) && SP_IS_GROUP(dest) && SP_GROUP(origin)->getItemCount() == SP_GROUP(dest)->getItemCount() ) {
        std::vector< SPObject * > childs = origin->childList(true);
        size_t index = 0;
        for (std::vector<SPObject * >::iterator obj_it = childs.begin(); 
             obj_it != childs.end(); ++obj_it) {
            SPObject *dest_child = dest->nthChild(index); 
            cloneAttrbutes((*obj_it), dest_child, attributes, style_attributes); 
            index++;
        }
    }
    //Attributes
    SPShape * shape_origin = SP_SHAPE(origin);
    SPPath  * path_origin  = SP_PATH(origin);
    SPShape * shape_dest   = SP_SHAPE(dest);
    SPPath  * path_dest    = SP_PATH(dest);
    SPMask  * mask_origin  = SP_ITEM(origin)->mask_ref->getObject();
    SPMask  * mask_dest    = SP_ITEM(dest)->mask_ref->getObject();
    if(mask_origin && mask_dest) {
        std::vector<SPObject*> mask_list = mask_origin->childList(true);
        std::vector<SPObject*> mask_list_dest = mask_dest->childList(true);
        if (mask_list.size() == mask_list_dest.size()) {
            size_t i = 0;
            for ( std::vector<SPObject*>::const_iterator iter=mask_list.begin();iter!=mask_list.end();++iter) {
                SPObject * mask_data = *iter;
                SPObject * mask_dest_data = mask_list_dest[i];
                cloneAttrbutes(mask_data, mask_dest_data, attributes, style_attributes);
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
                cloneAttrbutes(clippath_data, clippath_dest_data, attributes, style_attributes);
                i++;
            }
        }
    }
    gchar ** attarray = g_strsplit(attributes, ",", 0);
    gchar ** iter = attarray;
    while (*iter != nullptr) {
        const char* attribute = (*iter);
        if (strlen(attribute)) {
            if ( shape_dest && shape_origin && (std::strcmp(attribute, "d") == 0)) {
                SPCurve *c = nullptr;
                if (method == CLM_BSPLINESPIRO) {
                    c = shape_origin->getCurveForEdit();
                    SPLPEItem * lpe_item = SP_LPE_ITEM(origin);
                    if (lpe_item) {
                        PathEffectList lpelist = lpe_item->getEffectList();
                        PathEffectList::iterator i;
                        for (i = lpelist.begin(); i != lpelist.end(); ++i) {
                            LivePathEffectObject *lpeobj = (*i)->lpeobject;
                            if (lpeobj) {
                                Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                                if (dynamic_cast<Inkscape::LivePathEffect::LPEBSpline *>(lpe)) {
                                    LivePathEffect::sp_bspline_do_effect(c, 0);
                                } else if (dynamic_cast<Inkscape::LivePathEffect::LPESpiro *>(lpe)) {
                                    LivePathEffect::sp_spiro_do_effect(c);
                                }
                            }
                        }
                    }
                } else if (method == CLM_ORIGINALD) {
                    c = shape_origin->getCurveForEdit();
                } else if (method == CLM_NONE) {
                    c = shape_dest->getCurve();
                } else {
                    c = shape_origin->getCurve();
                }
                if (c) {
                    Geom::PathVector c_pv = c->get_pathvector();
                    c->set_pathvector(c_pv);
                    shape_dest->setCurveInsync(c);
                    gchar *str = sp_svg_write_path(c_pv);
                    dest->getRepr()->setAttribute("d", str);
                    g_free(str);
                    c->unref();
                } else {
                    dest->getRepr()->setAttribute(attribute, nullptr);
                }
            } else {
                dest->getRepr()->setAttribute(attribute, origin->getRepr()->attribute(attribute));
            }
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
    while (*styleiter != nullptr) {
        const char* attribute = (*styleiter);
        if (strlen(attribute)) {
            const char* origin_attribute = sp_repr_css_property(css_origin, attribute, "");
            if (!strlen(origin_attribute)) { //==0
                sp_repr_css_set_property (css_dest, attribute, nullptr);
            } else {
                sp_repr_css_set_property (css_dest, attribute, origin_attribute);
            }
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
    start_listening();
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    if (linkeditem.linksToItem()) {
        Glib::ustring attr = "d,";
        if (!allow_transforms) {
           attr += Glib::ustring("transform") + Glib::ustring(",");
        }
        gchar * attributes_str = attributes.param_getSVGValue();
        attr += Glib::ustring(attributes_str) + Glib::ustring(",");
        if (attr.size()  && !Glib::ustring(attributes_str).size()) {
            attr.erase (attr.size()-1, 1);
        }
        gchar * style_attributes_str = style_attributes.param_getSVGValue();
        Glib::ustring style_attr = "";
        if (style_attr.size() && !Glib::ustring( style_attributes_str).size()) {
            style_attr.erase (style_attr.size()-1, 1);
        }
        style_attr += Glib::ustring( style_attributes_str) + Glib::ustring(",");

        SPItem * orig =  SP_ITEM(linkeditem.getObject());
        if(!orig) {
            return;
        }
        SPItem * dest =  SP_ITEM(sp_lpe_item); 
        Geom::OptRect o_bbox = orig->geometricBounds(orig->transform);
        Geom::OptRect d_bbox = dest->geometricBounds(dest->transform);
        const gchar * id = orig->getId();
        cloneAttrbutes(orig, dest, attr.c_str(), style_attr.c_str());
        linked = id;
    } else {
        linked = "";
    }
    previus_method = method;
}

void
LPECloneOriginal::start_listening()
{
    if ( !sp_lpe_item || listening ) {
        return;
    }
    quit_listening();
    modified_connection = SP_OBJECT(sp_lpe_item)->connectModified(sigc::mem_fun(*this, &LPECloneOriginal::modified));
    listening = true;
}

void
LPECloneOriginal::quit_listening()
{
    modified_connection.disconnect();
    listening = false;
}

void
LPECloneOriginal::modified(SPObject */*obj*/, guint /*flags*/)
{
    if ( !sp_lpe_item || is_updating) {
        is_updating = false;
        return;
    }
    SP_OBJECT(this->getLPEObj())->requestModified(SP_OBJECT_MODIFIED_FLAG);
    is_updating = true;
}

LPECloneOriginal::~LPECloneOriginal()
{
    quit_listening();
}

void 
LPECloneOriginal::doEffect (SPCurve * curve)
{
    curve->set_pathvector(current_shape->getCurve()->get_pathvector());
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
