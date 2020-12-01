// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpe-clone-original.h"
#include "live_effects/lpe-spiro.h"
#include "live_effects/lpe-bspline.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "display/curve.h"
#include "svg/path-string.h"
#include "svg/svg.h"

#include "ui/tools-switch.h"
#include "ui/tools/node-tool.h"
#include "object/sp-clippath.h"
#include "object/sp-mask.h"
#include "object/sp-path.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "display/curve.h"

#include "xml/sp-css-attr.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<Clonelpemethod> ClonelpemethodData[] = {
    { CLM_NONE, N_("No Shape"), "none" },
    { CLM_D, N_("With LPE's"), "d" },
    { CLM_ORIGINALD, N_("Without LPE's"), "originald" },
    { CLM_BSPLINESPIRO, N_("Spiro or BSpline Only"), "bsplinespiro" },
};
static const Util::EnumDataConverter<Clonelpemethod> CLMConverter(ClonelpemethodData, CLM_END);

LPECloneOriginal::LPECloneOriginal(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , linkeditem(_("Linked Item:"), _("Item from which to take the original data"), "linkeditem", &wr, this)
    , method(_("Shape"), _("Shape linked"), "method", CLMConverter, &wr, this, CLM_D)
    , attributes("Attributes", "Attributes linked, comma separated attributes like trasform, X, Y...",
                 "attributes", &wr, this, "")
    , css_properties("CSS Properties",
                       "CSS properties linked, comma separated attributes like fill, filter, opacity...",
                       "css_properties", &wr, this, "")
    , allow_transforms(_("Allow Transforms"), _("Allow transforms"), "allow_transforms", &wr, this, true)
{
    //0.92 compatibility
    const gchar * linkedpath = this->getRepr()->attribute("linkedpath");
    if (linkedpath && strcmp(linkedpath, "") != 0){
        this->getRepr()->setAttribute("linkeditem", linkedpath);
        this->getRepr()->removeAttribute("linkedpath");
        this->getRepr()->setAttribute("method", "bsplinespiro");
        this->getRepr()->setAttribute("allow_transforms", "false");
    };
    listening = false;
    sync = false;
    linked = "";
    if (this->getRepr()->attribute("linkeditem")) {
        linked = this->getRepr()->attribute("linkeditem");
    }
    registerParameter(&linkeditem);
    registerParameter(&method);
    registerParameter(&attributes);
    registerParameter(&css_properties);
    registerParameter(&allow_transforms);
    attributes.param_hide_canvas_text();
    css_properties.param_hide_canvas_text();
}

void 
LPECloneOriginal::syncOriginal()
{
    if (method != CLM_NONE) {
        sync = true;
        // TODO remove the tools_switch atrocity.
        sp_lpe_item_update_patheffect (sp_lpe_item, false, true);
        method.param_set_value(CLM_NONE);
        refresh_widgets = true;
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        sp_lpe_item_update_patheffect (sp_lpe_item, false, true);
        if (desktop && dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop->event_context)) {
            tools_switch(desktop, TOOLS_SELECT);
            tools_switch(desktop, TOOLS_NODES);
        }
    }
}

Gtk::Widget *
LPECloneOriginal::newWidget()
{
    // use manage here, because after deletion of Effect object, others might still be pointing to this widget.
    Gtk::Box *vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(6);
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter * param = *it;
            Gtk::Widget * widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
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
    Gtk::Button * sync_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("No Shape Sync to Current"))));
    sync_button->signal_clicked().connect(sigc::mem_fun (*this,&LPECloneOriginal::syncOriginal));
    vbox->pack_start(*sync_button, true, true, 2);
    if(Gtk::Widget* widg = defaultParamSet()) {
        vbox->pack_start(*widg, true, true, 2);
    }
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void
LPECloneOriginal::cloneAttrbutes(SPObject *origin, SPObject *dest, const gchar * attributes, const gchar * css_properties, bool init) 
{
    SPDocument *document = getSPDoc();
    if (!document || !origin || !dest) {
        return;
    }
    SPGroup * group_origin = dynamic_cast<SPGroup *>(origin);
    SPGroup * group_dest = dynamic_cast<SPGroup *>(dest);
    if (group_origin && group_dest && group_origin->getItemCount() != group_dest->getItemCount()) {
        sp_lpe_item_enable_path_effects(sp_lpe_item, false);
        std::vector< SPObject * > childs = group_origin->childList(true);
        std::vector< SPObject * > childsdest = group_dest->childList(true);
        for (auto & child : childsdest) {
            child->deleteObject(true);
        }
        for (auto & child : childs) {
            Inkscape::XML::Document *xml_doc = document->getReprDoc();
            Inkscape::XML::Node* dup = child->getRepr()->duplicate(xml_doc);
            dest->getRepr()->appendChild(dup);
            Inkscape::GC::release(dup);
        }   
        sp_lpe_item_enable_path_effects(sp_lpe_item, true);                 
    }
    if (group_origin && group_dest) {
        std::vector< SPObject * > childs = group_origin->childList(true);
        size_t index = 0;
        for (auto & child : childs) {
            SPObject *dest_child = group_dest->nthChild(index); 
            cloneAttrbutes(child, dest_child, attributes, css_properties, init); 
            index++;
        }
    }
    //Attributes
    SPShape * shape_origin = dynamic_cast<SPShape *>(origin);
    SPShape * shape_dest   = dynamic_cast<SPShape *>(dest);
    SPItem  * item_origin  = dynamic_cast<SPItem  *>(origin);
    SPItem  * item_dest    = dynamic_cast<SPItem  *>(dest);
    SPMask  * mask_origin  = dynamic_cast<SPMask  *>(item_origin->getMaskObject());
    SPMask  * mask_dest    = dynamic_cast<SPMask  *>(item_dest->getMaskObject());
    if(mask_origin && mask_dest) {
        std::vector<SPObject*> mask_list = mask_origin->childList(true);
        std::vector<SPObject*> mask_list_dest = mask_dest->childList(true);
        if (mask_list.size() == mask_list_dest.size()) {
            size_t i = 0;
            for (auto mask_data : mask_list) {
                SPObject * mask_dest_data = mask_list_dest[i];
                cloneAttrbutes(mask_data, mask_dest_data, attributes, css_properties, init);
                i++;
            }
        }
    }
    
    SPClipPath *clippath_origin = SP_ITEM(origin)->getClipObject();
    SPClipPath *clippath_dest = SP_ITEM(dest)->getClipObject();
    if(clippath_origin && clippath_dest) {
        std::vector<SPObject*> clippath_list = clippath_origin->childList(true);
        std::vector<SPObject*> clippath_list_dest = clippath_dest->childList(true);
        if (clippath_list.size() == clippath_list_dest.size()) {
            size_t i = 0;
            for (auto clippath_data : clippath_list) {
                SPObject * clippath_dest_data = clippath_list_dest[i];
                cloneAttrbutes(clippath_data, clippath_dest_data, attributes, css_properties, init);
                i++;
            }
        }
    }
    gchar ** attarray = g_strsplit(old_attributes.c_str(), ",", 0);
    gchar ** iter = attarray;
    while (*iter != nullptr) {
        const char* attribute = (*iter);
        if (strlen(attribute)) {
            dest->getRepr()->removeAttribute(attribute);
        }
        iter++;
    }
    attarray = g_strsplit(attributes, ",", 0);
    iter = attarray;
    while (*iter != nullptr) {
        const char* attribute = (*iter);
        if (strlen(attribute) && shape_dest && shape_origin) {
            if (std::strcmp(attribute, "d") == 0) {
                std::unique_ptr<SPCurve> c;
                if (method == CLM_BSPLINESPIRO) {
                    c = SPCurve::copy(shape_origin->curveForEdit());
                    SPLPEItem * lpe_item = SP_LPE_ITEM(origin);
                    if (lpe_item) {
                        PathEffectList lpelist = lpe_item->getEffectList();
                        PathEffectList::iterator i;
                        for (i = lpelist.begin(); i != lpelist.end(); ++i) {
                            LivePathEffectObject *lpeobj = (*i)->lpeobject;
                            if (lpeobj) {
                                Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                                if (dynamic_cast<Inkscape::LivePathEffect::LPEBSpline *>(lpe)) {
                                    Geom::PathVector hp;
                                    LivePathEffect::sp_bspline_do_effect(c.get(), 0, hp);
                                } else if (dynamic_cast<Inkscape::LivePathEffect::LPESpiro *>(lpe)) {
                                    LivePathEffect::sp_spiro_do_effect(c.get());
                                }
                            }
                        }
                    }
                } else if (method == CLM_ORIGINALD) {
                    c = SPCurve::copy(shape_origin->curveForEdit());
                } else if(method == CLM_D){
                    c = SPCurve::copy(shape_origin->curve());
                }
                if (c && method != CLM_NONE) {
                    Geom::PathVector c_pv = c->get_pathvector();
                    c->set_pathvector(c_pv);
                    auto str = sp_svg_write_path(c_pv);
                    if (sync){
                        dest->getRepr()->setAttribute("inkscape:original-d", str);
                    }
                    shape_dest->setCurveInsync(std::move(c));
                    dest->getRepr()->setAttribute("d", str);
                } else if (method != CLM_NONE) {
                    dest->getRepr()->removeAttribute(attribute);
                }
            } else {
                dest->getRepr()->setAttribute(attribute, origin->getRepr()->attribute(attribute));
            }
        }
        iter++;
    }
    if (!allow_transforms) {
        dest->getRepr()->setAttribute("transform", origin->getRepr()->attribute("transform"));
    }
    g_strfreev (attarray);
    SPCSSAttr *css_origin = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css_origin, origin->getRepr()->attribute("style"));
    SPCSSAttr *css_dest = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css_dest, dest->getRepr()->attribute("style"));
    if (init) {
        css_dest = css_origin;
    }
    gchar ** styleattarray = g_strsplit(old_css_properties.c_str(), ",", 0);
    gchar ** styleiter = styleattarray;
    while (*styleiter != nullptr) {
        const char* attribute = (*styleiter);
        if (strlen(attribute)) {
            sp_repr_css_set_property (css_dest, attribute, nullptr);
        }
        styleiter++;
    }
    styleattarray = g_strsplit(css_properties, ",", 0);
    styleiter = styleattarray;
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
    dest->getRepr()->setAttributeOrRemoveIfEmpty("style", css_str);
}

void
LPECloneOriginal::doBeforeEffect (SPLPEItem const* lpeitem){
    start_listening();
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    if (!deleted_connection) {
        deleted_connection = sp_lpe_item->connectDelete(sigc::mem_fun(*this, &LPECloneOriginal::lpeitem_deleted));
    }
    
    if (linkeditem.linksToItem()) {
        SPItem *orig = dynamic_cast<SPItem *>(linkeditem.getObject());
        if(!orig) {
            return;
        }
        SPText  *text_origin = dynamic_cast<SPText *>(orig);
        SPGroup  *group_origin = dynamic_cast<SPGroup *>(orig);
        SPItem *dest = dynamic_cast<SPItem *>(sp_lpe_item);
        const gchar * id = orig->getId();
        bool init = !is_load && g_strcmp0(id, linked.c_str()) != 0;
        /* if (sp_lpe_item->getRepr()->attribute("style")) {
            init = false;
        } */
        Glib::ustring attr = "d,";
        if (text_origin) {
            std::unique_ptr<SPCurve> curve = text_origin->getNormalizedBpath();
            dest->getRepr()->setAttribute("inkscape:original-d", sp_svg_write_path(curve->get_pathvector()));
            attr = "";
        }
        if (!allow_transforms) {
           attr += Glib::ustring("transform") + Glib::ustring(",");
        }
        original_bbox(lpeitem, false, true);
        auto attributes_str = attributes.param_getSVGValue();
        attr += attributes_str + ",";
        if (attr.size()  && attributes_str.empty()) {
            attr.erase (attr.size()-1, 1);
        }
        auto css_properties_str = css_properties.param_getSVGValue();
        Glib::ustring style_attr = "";
        if (style_attr.size() && css_properties_str.empty()) {
            style_attr.erase (style_attr.size()-1, 1);
        }
        style_attr += css_properties_str + ",";
        cloneAttrbutes(orig, dest, attr.c_str(), style_attr.c_str(), init);
        if (!group_origin && linkeditem.last_transform.isTranslation()) {
            Geom::Affine orig = sp_lpe_item->transform;
            sp_lpe_item->transform *= orig.inverse() * linkeditem.last_transform.inverse() * orig;  
            linkeditem.last_transform = Geom::identity();      
        }
        old_css_properties = css_properties.param_getSVGValue();
        old_attributes = attributes.param_getSVGValue();
        sync = false;
        linked = id;
    } else {
        linked = "";
    }
}

void
LPECloneOriginal::start_listening()
{
    if ( !sp_lpe_item || listening ) {
        return;
    }
    quit_listening();
    listening = true;
}

void
LPECloneOriginal::quit_listening()
{
    listening = false;
}

void
LPECloneOriginal::lpeitem_deleted(SPObject */*deleted*/)
{
    quit_listening();
    deleted_connection.disconnect();
}

LPECloneOriginal::~LPECloneOriginal()
{
    quit_listening();
}

void 
LPECloneOriginal::doEffect (SPCurve * curve)
{
    if (method != CLM_NONE) {
        SPCurve const *current_curve = current_shape->curve();
        if (current_curve != nullptr) {
            curve->set_pathvector(current_curve->get_pathvector());
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
