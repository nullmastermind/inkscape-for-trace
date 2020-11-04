// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/lpe-powermask.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"   
#include <2geom/path-intersection.h>
#include <2geom/intersection-graph.h>
#include "display/curve.h"
#include "helper/geom.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "svg/stringstream.h"
#include "ui/tools-switch.h"
#include "path-chemistry.h"
#include "extract-uri.h"
#include <bad-uri-exception.h>

#include "object/sp-mask.h"
#include "object/sp-path.h"
#include "object/sp-shape.h"
#include "object/sp-defs.h"
#include "object/sp-item-group.h"
#include "object/uri.h"


// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEPowerMask::LPEPowerMask(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
    uri("Store the uri of mask", "", "uri", &wr, this, "false", false),
    invert(_("Invert mask"), _("Invert mask"), "invert", &wr, this, false),
    //wrap(_("Wrap mask data"), _("Wrap mask data allowing previous filters"), "wrap", &wr, this, false),
    hide_mask(_("Hide mask"), _("Hide mask"), "hide_mask", &wr, this, false),
    background(_("Add background to mask"), _("Add background to mask"), "background", &wr, this, false),
    background_color(_("Background color and opacity"), _("Set color and opacity of the background"), "background_color", &wr, this, 0xffffffff)
{
    registerParameter(&uri);
    registerParameter(&invert);
    registerParameter(&hide_mask);
    registerParameter(&background);
    registerParameter(&background_color);
    previous_color = background_color.get_value();
}

LPEPowerMask::~LPEPowerMask() = default;

Glib::ustring LPEPowerMask::getId() { return Glib::ustring("mask-powermask-") + Glib::ustring(getLPEObj()->getId()); }

void
LPEPowerMask::doOnApply (SPLPEItem const * lpeitem)
{
    SPLPEItem *item = const_cast<SPLPEItem*>(lpeitem);
    SPObject * mask = item->getMaskObject();
    bool hasit = false;
    if (lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled()) {
        PathEffectList path_effect_list(*lpeitem->path_effect_list);
        for (auto &lperef : path_effect_list) {
            LivePathEffectObject *lpeobj = lperef->lpeobject;
            if (!lpeobj) {
                /** \todo Investigate the cause of this.
                 * For example, this happens when copy pasting an object with LPE applied. Probably because the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                */
                g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                return;
            }
            if (LPETypeConverter.get_key(lpeobj->effecttype) == "powermask") {
                hasit = true;
                break;
            }
        }
    }
    if (!mask || hasit) {
        item->removeCurrentPathEffect(false);
    } else {
        Glib::ustring newmask = getId();
        Glib::ustring uri = Glib::ustring("url(#") + newmask + Glib::ustring(")");
        mask->setAttribute("id", newmask);
        item->setAttribute("mask", uri);
    }
}

void LPEPowerMask::tryForkMask()
{
    SPDocument *document = getSPDoc();
    if (!document || !sp_lpe_item) {
        return;
    }
    SPObject *mask = sp_lpe_item->getMaskObject();
    SPObject *elemref = document->getObjectById(getId().c_str());
    if (!elemref && sp_lpe_item && mask) {
        Glib::ustring newmask = getId();
        Glib::ustring uri = Glib::ustring("url(#") + newmask + Glib::ustring(")");
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *fork = mask->getRepr()->duplicate(xml_doc);
        mask = SP_OBJECT(document->getDefs()->appendChildRepr(fork));
        fork->setAttribute("id", newmask);
        Inkscape::GC::release(fork);
        sp_lpe_item->setAttribute("mask", uri);
    }
}

void
LPEPowerMask::doBeforeEffect (SPLPEItem const* lpeitem){
    //To avoid close of color dialog and better performance on change color
    tryForkMask();
    SPObject * mask = SP_ITEM(sp_lpe_item)->getMaskObject();
    auto uri_str = uri.param_getSVGValue();
    if (hide_mask && mask) {
        SP_ITEM(sp_lpe_item)->getMaskRef().detach();
    } else if (!hide_mask && !mask && !uri_str.empty()) {
        SP_ITEM(sp_lpe_item)->getMaskRef().try_attach(uri_str.c_str());
    }
    mask = SP_ITEM(sp_lpe_item)->getMaskObject();
    if (mask) {
        if (previous_color != background_color.get_value()) {
            previous_color = background_color.get_value();
            setMask();
        } else {
            uri.param_setValue(Glib::ustring(extract_uri(sp_lpe_item->getRepr()->attribute("mask"))), true);
            SP_ITEM(sp_lpe_item)->getMaskRef().detach();
            Geom::OptRect bbox = lpeitem->visualBounds();
            if(!bbox) {
                return;
            }
            uri_str = uri.param_getSVGValue();
            SP_ITEM(sp_lpe_item)->getMaskRef().try_attach(uri_str.c_str());

            Geom::Rect bboxrect = (*bbox);
            bboxrect.expandBy(1);
            mask_box.clear();
            mask_box = Geom::Path(bboxrect);
            setMask();
        }
    } else if(!hide_mask) {
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
        item->removeCurrentPathEffect(false);
    }
}

void
LPEPowerMask::setMask(){
    SPMask *mask = SP_ITEM(sp_lpe_item)->getMaskObject();
    SPObject *elemref = nullptr;
    SPDocument *document = getSPDoc();
    if (!document || !mask) {
        return;
    }
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *box = nullptr;
    Inkscape::XML::Node *filter = nullptr;
    SPDefs * defs = document->getDefs();
    Glib::ustring mask_id = getId();
    Glib::ustring box_id = mask_id + (Glib::ustring)"_box";
    Glib::ustring filter_id = mask_id + (Glib::ustring)"_inverse";
    Glib::ustring filter_label = (Glib::ustring)"filter" + mask_id;
    Glib::ustring filter_uri = (Glib::ustring)"url(#" + filter_id + (Glib::ustring)")";
    if (!(elemref = document->getObjectById(filter_id))) {
        filter = xml_doc->createElement("svg:filter");
        filter->setAttribute("id", filter_id);
        filter->setAttribute("inkscape:label", filter_label);
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, "color-interpolation-filters", "sRGB");
        sp_repr_css_change(filter, css, "style");
        sp_repr_css_attr_unref(css);
        filter->setAttribute("height", "100");
        filter->setAttribute("width", "100");
        filter->setAttribute("x", "-50");
        filter->setAttribute("y", "-50");
        Inkscape::XML::Node *primitive1 =  xml_doc->createElement("svg:feColorMatrix");
        Glib::ustring primitive1_id = (mask_id + (Glib::ustring)"_primitive1").c_str();
        primitive1->setAttribute("id", primitive1_id);
        primitive1->setAttribute("values", "1");
        primitive1->setAttribute("type", "saturate");
        primitive1->setAttribute("result", "fbSourceGraphic");
        Inkscape::XML::Node *primitive2 =  xml_doc->createElement("svg:feColorMatrix");
        Glib::ustring primitive2_id = (mask_id + (Glib::ustring)"_primitive2").c_str();
        primitive2->setAttribute("id", primitive2_id);
        primitive2->setAttribute("values", "-1 0 0 0 1 0 -1 0 0 1 0 0 -1 0 1 0 0 0 1 0 ");
        primitive2->setAttribute("in", "fbSourceGraphic");
        elemref = defs->appendChildRepr(filter);
        Inkscape::GC::release(filter);
        filter->appendChild(primitive1);
        Inkscape::GC::release(primitive1);
        filter->appendChild(primitive2);
        Inkscape::GC::release(primitive2);
    }
    Glib::ustring g_data_id = mask_id + (Glib::ustring)"_container";
    if((elemref = document->getObjectById(g_data_id))){
        std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(elemref));
        for (auto iter : item_list) {
            Inkscape::XML::Node *mask_node = iter->getRepr();
            elemref->getRepr()->removeChild(mask_node);
            mask->getRepr()->appendChild(mask_node);
            Inkscape::GC::release(mask_node);
        }
        elemref->deleteObject(true);
    }
    std::vector<SPObject*> mask_list = mask->childList(true);
    for (auto iter : mask_list) {
        SPItem * mask_data = SP_ITEM(iter);
        Inkscape::XML::Node *mask_node = mask_data->getRepr();
        if (! strcmp(mask_data->getId(), box_id.c_str())){
            continue;
        }
        Glib::ustring mask_data_id = (Glib::ustring)mask_data->getId();
        SPCSSAttr *css = sp_repr_css_attr_new();
        if(mask_node->attribute("style")) {
            sp_repr_css_attr_add_from_string(css, mask_node->attribute("style"));
        }
        char const* filter = sp_repr_css_property (css, "filter", nullptr);
        if(!filter || !strcmp(filter, filter_uri.c_str())) {
            if (invert && is_visible) {
                sp_repr_css_set_property (css, "filter", filter_uri.c_str());
            } else {
                sp_repr_css_set_property (css, "filter", nullptr);
            }
            Glib::ustring css_str;
            sp_repr_css_write_string(css, css_str);
            mask_node->setAttribute("style", css_str);
        }
    }
    if ((elemref = document->getObjectById(box_id))) {
        elemref->deleteObject(true);
    }
    if (background && is_visible) {
        bool exist = true;
        if (!(elemref = document->getObjectById(box_id))) {
            box = xml_doc->createElement("svg:path");
            box->setAttribute("id", box_id);
            exist = false;
        }
        Glib::ustring style;
        gchar c[32];
        unsigned const rgb24 = background_color.get_value() >> 8;
        sprintf(c, "#%06x", rgb24);
        style = Glib::ustring("fill:") + Glib::ustring(c);
        Inkscape::SVGOStringStream os;
        os << SP_RGBA32_A_F(background_color.get_value());
        style = style + Glib::ustring(";fill-opacity:") + Glib::ustring(os.str());
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_attr_add_from_string(css, style.c_str());
        char const* filter = sp_repr_css_property (css, "filter", nullptr);
        if(!filter || !strcmp(filter, filter_uri.c_str())) {
            if (invert && is_visible) {
                sp_repr_css_set_property (css, "filter", filter_uri.c_str());
            } else {
                sp_repr_css_set_property (css, "filter", nullptr);
            }
            
        }
        Glib::ustring css_str;
        sp_repr_css_write_string(css, css_str);
        box->setAttributeOrRemoveIfEmpty("style", css_str);
        box->setAttribute("d", sp_svg_write_path(mask_box));
        if (!exist) {
            elemref = mask->appendChildRepr(box);
            Inkscape::GC::release(box);
        }
        box->setPosition(0);
    } else if ((elemref = document->getObjectById(box_id))) {
        elemref->deleteObject(true);
    }
    mask->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void 
LPEPowerMask::doOnVisibilityToggled(SPLPEItem const* lpeitem)
{
    doBeforeEffect(lpeitem);
}

void
LPEPowerMask::doEffect (SPCurve * curve)
{
}

void 
LPEPowerMask::doOnRemove (SPLPEItem const* lpeitem)
{
    SPMask *mask = lpeitem->getMaskObject();
    if (mask) {
        if (keep_paths) {
            return;
        }
        invert.param_setValue(false);
        //wrap.param_setValue(false);
        background.param_setValue(false);
        setMask();
        SPObject *elemref = nullptr;
        SPDocument *document = getSPDoc();
        Glib::ustring mask_id = getId();
        Glib::ustring filter_id = mask_id + (Glib::ustring)"_inverse";
        if ((elemref = document->getObjectById(filter_id))) {
            elemref->deleteObject(true);
        }
    }
}

void sp_inverse_powermask(Inkscape::Selection *sel) {
    if (!sel->isEmpty()) {
        SPDocument *document = SP_ACTIVE_DOCUMENT;
        if (!document) {
            return;
        }
        auto selList = sel->items();
        for(auto i = boost::rbegin(selList); i != boost::rend(selList); ++i) {
            SPLPEItem* lpeitem = dynamic_cast<SPLPEItem*>(*i);
            if (lpeitem) {
                SPMask *mask = lpeitem->getMaskObject();
                if (mask) {
                    Effect::createAndApply(POWERMASK, SP_ACTIVE_DOCUMENT, lpeitem);
                    Effect* lpe = lpeitem->getCurrentLPE();
                    if (lpe) {
                        lpe->getRepr()->setAttribute("invert", "false");
                        lpe->getRepr()->setAttribute("is_visible", "true");
                        lpe->getRepr()->setAttribute("hide_mask", "false");
                        lpe->getRepr()->setAttribute("background", "true");
                        lpe->getRepr()->setAttribute("background_color", "#ffffffff");
                    }
                }
            }
        }
    }
}

void sp_remove_powermask(Inkscape::Selection *sel) {
    if (!sel->isEmpty()) {
        auto selList = sel->items();
        for (auto i = boost::rbegin(selList); i != boost::rend(selList); ++i) {
            SPLPEItem *lpeitem = dynamic_cast<SPLPEItem *>(*i);
            if (lpeitem) {
                if (lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled()) {
                    PathEffectList path_effect_list(*lpeitem->path_effect_list);
                    for (auto &lperef : path_effect_list) {
                        LivePathEffectObject *lpeobj = lperef->lpeobject;
                        if (!lpeobj) {
                            /** \todo Investigate the cause of this.
                             * For example, this happens when copy pasting an object with LPE applied. Probably because
                             * the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                             */
                            g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                            return;
                        }
                        if (LPETypeConverter.get_key(lpeobj->effecttype) == "powermask") {
                            lpeitem->setCurrentPathEffect(lperef);
                            lpeitem->removeCurrentPathEffect(false);
                            break;
                        }
                    }
                }
            }
        }
    }
}

}; //namespace LivePathEffect
}; /* namespace Inkscape */

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
