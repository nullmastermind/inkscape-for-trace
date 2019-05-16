// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/lpe-powerclip.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include <2geom/path-intersection.h>
#include <2geom/intersection-graph.h>
#include "display/curve.h"
#include "helper/geom.h"
#include "ui/tools-switch.h"
#include "path-chemistry.h"
#include "extract-uri.h"
#include <bad-uri-exception.h>

#include "object/sp-clippath.h"
#include "object/sp-path.h"
#include "object/sp-shape.h"
#include "object/sp-item.h"
#include "object/sp-item-group.h"
#include "object/uri.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEPowerClip::LPEPowerClip(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
    hide_clip(_("Hide clip"), _("Hide clip"), "hide_clip", &wr, this, false),
    is_inverse("Store the last inverse apply", "", "is_inverse", &wr, this, "false", false),
    uri("Store the uri of clip", "", "uri", &wr, this, "false", false),
    inverse(_("Inverse clip"), _("Inverse clip"), "inverse", &wr, this, false),
    flatten(_("Flatten clip"), _("Flatten clip, see fill rule once convert to paths"), "flatten", &wr, this, false),
    message(_("Info Box"), _("Important messages"), "message", &wr, this, _("Use fill-rule evenodd on <b>fill and stroke</b> dialog if no flatten result after convert clip to paths."))
{
    registerParameter(&uri);
    registerParameter(&inverse);
    registerParameter(&flatten);
    registerParameter(&hide_clip);
    registerParameter(&is_inverse);
    registerParameter(&message);
    message.param_set_min_height(55);
}

LPEPowerClip::~LPEPowerClip() = default;

void
LPEPowerClip::doOnApply (SPLPEItem const * lpeitem)
{
    SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
    SPObject * clip_path = item->clip_ref->getObject();
    if (!clip_path) {
        item->removeCurrentPathEffect(false);
    }
}

void
LPEPowerClip::doBeforeEffect (SPLPEItem const* lpeitem){
    SPObject * clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
    gchar * uri_str = uri.param_getSVGValue();
    if(hide_clip && clip_path) {
        SP_ITEM(sp_lpe_item)->clip_ref->detach();
    } else if (!hide_clip && !clip_path && uri_str) {
        try {
            SP_ITEM(sp_lpe_item)->clip_ref->attach(Inkscape::URI(uri_str));
        } catch (Inkscape::BadURIException &e) {
            g_warning("%s", e.what());
            SP_ITEM(sp_lpe_item)->clip_ref->detach();
        }
    }
    clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
    if (clip_path) {
        uri.param_setValue(Glib::ustring(extract_uri(sp_lpe_item->getRepr()->attribute("clip-path"))), true);
        SP_ITEM(sp_lpe_item)->clip_ref->detach();
        Geom::OptRect bbox = sp_lpe_item->visualBounds();
        if(!bbox) {
            return;
        }
        uri_str = uri.param_getSVGValue();
        if (uri_str) {
            try {
                SP_ITEM(sp_lpe_item)->clip_ref->attach(Inkscape::URI(uri_str));
            } catch (Inkscape::BadURIException &e) {
                g_warning("%s", e.what());
                SP_ITEM(sp_lpe_item)->clip_ref->detach();
            }
        } else {
            SP_ITEM(sp_lpe_item)->clip_ref->detach();
        }
        g_free(uri_str);
        Geom::Rect bboxrect = (*bbox);
        bboxrect.expandBy(1);
        Geom::Point topleft      = bboxrect.corner(0);
        Geom::Point topright     = bboxrect.corner(1);
        Geom::Point bottomright  = bboxrect.corner(2);
        Geom::Point bottomleft   = bboxrect.corner(3);
        clip_box.clear();
        clip_box.start(topleft);
        clip_box.appendNew<Geom::LineSegment>(topright);
        clip_box.appendNew<Geom::LineSegment>(bottomright);
        clip_box.appendNew<Geom::LineSegment>(bottomleft);
        clip_box.close();

        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
            SPObject * clip_data = *iter;
            gchar * is_inverse_str = is_inverse.param_getSVGValue();
            if(!strcmp(is_inverse_str,"false") && inverse && isVisible()) {
                SPCurve * clipcurve = new SPCurve();
                addInverse(SP_ITEM(clip_data), clipcurve, Geom::Affine::identity(), true);     
            } else if((!strcmp(is_inverse_str,"true") && !inverse && isVisible()) ||
                (inverse && !is_visible && is_inverse_str == (Glib::ustring)"true"))
            {
                removeInverse(SP_ITEM(clip_data));    
            } else if(inverse && isVisible()) {
                updateInverse(SP_ITEM(clip_data));
            }
            g_free(is_inverse_str);
        }
    } else if(!hide_clip) {
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
        item->removeCurrentPathEffect(false);
    }
}

void
LPEPowerClip::doAfterEffect (SPLPEItem const* lpeitem){
    is_load = false;
    if (!hide_clip && flatten && isVisible()) {
        SP_ITEM(sp_lpe_item)->clip_ref->detach();
    }
}

void
LPEPowerClip::addInverse (SPItem * clip_data, SPCurve * clipcurve, Geom::Affine affine, bool root){
    gchar * is_inverse_str = is_inverse.param_getSVGValue();
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    SPObject *elemref = NULL;
    if(root) {
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        SP_LPE_ITEM(clip_data)->removeAllPathEffects(true);
        Inkscape::XML::Node *clip_path_node = xml_doc->createElement("svg:path");
        Inkscape::XML::Node *parent = clip_data->getRepr()->parent();
        parent->appendChild(clip_path_node);
        elemref = document->getObjectByRepr(clip_path_node);
        elemref->setAttribute("style","fill-rule:evenodd");
        elemref->setAttribute("id", (Glib::ustring("lpe_") + Glib::ustring(this->getLPEObj()->getId())).c_str());
    }
    if(!strcmp(is_inverse_str,"false")) {
        if (SP_IS_GROUP(clip_data)) {
            std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
            for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
                SPItem *subitem = *iter;
                Geom::Affine affine_group = SP_ITEM(clip_data)->transform;
                addInverse(subitem, clipcurve, affine_group, false);
                if (root) {
                     clip_box *= affine_group;
                }
            }
        } else if (SP_IS_SHAPE(clip_data)) {
            SPCurve * c = nullptr;
            c = SP_SHAPE(clip_data)->getCurve();
            c->transform(affine);
            if (c) {
                Geom::PathVector c_pv = c->get_pathvector();
                //TODO: this can be not correct but no better way
                bool dir_a = Geom::path_direction(c_pv[0]);
                bool dir_b = Geom::path_direction(clip_box);
                if (dir_a == dir_b) {
                   clip_box = clip_box.reversed();
                }
                clipcurve->append(c, false);
                gchar * is_inverse_str = is_inverse.param_getSVGValue();
                if (strcmp(is_inverse_str, "true") != 0) {
                    is_inverse.param_setValue((Glib::ustring)"true", true);
                }
                c->reset();
                delete c;
            }  
            Geom::Affine hidetrans = (Geom::Affine)Geom::Translate(0,999999);    
            SP_ITEM(clip_data)->doWriteTransform(hidetrans); 
        }
    }
    if(root) {
        SPCurve * container = new SPCurve;
        container->set_pathvector(clip_box);
        clipcurve->append(container, false);
        SP_SHAPE(elemref)->setCurve(clipcurve);
        container->reset();
        delete container;
    }
    g_free(is_inverse_str);
}

void
LPEPowerClip::updateInverse (SPItem * clip_data) {
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    if(inverse && isVisible()) {
        if (SP_IS_GROUP(clip_data)) {
            std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
            for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
                SPItem *subitem = *iter;
                updateInverse(subitem);
            }
        } else if (SP_IS_SHAPE(clip_data)) {
            SPObject *elemref = document->getObjectById(Glib::ustring("lpe_") + Glib::ustring(this->getLPEObj()->getId()));
            if (elemref) {
                SPCurve * c = nullptr;
                c = SP_SHAPE(elemref)->getCurve();
                if (c) {
                    Geom::PathVector c_pv = c->get_pathvector();
                    //TODO: this can be not correct but no better way
                    bool dir_a = Geom::path_direction(c_pv[0]);
                    bool dir_b = Geom::path_direction(clip_box);
                    if (dir_a == dir_b) {
                        clip_box = clip_box.reversed();
                    }
                    if(c_pv.size() > 1) {
                        c_pv.pop_back();
                    }
                    c_pv.push_back(clip_box);
                    c->set_pathvector(c_pv);
                    SP_SHAPE(elemref)->setCurve(c);
                    c->unref();
                }
            }
        }
    }
}

void
LPEPowerClip::removeInverse (SPItem * clip_data){
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    gchar * is_inverse_str = is_inverse.param_getSVGValue();
    if(!strcmp(is_inverse_str,"true")) {
        if (SP_IS_GROUP(clip_data)) {
             std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
             for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
                 SPItem *subitem = *iter;
                 removeInverse(subitem);
             }
        } else if (SP_IS_SHAPE(clip_data)) {
            Geom::Affine unhidetrans = (Geom::Affine)Geom::Translate(0,-999999); 
            SP_ITEM(clip_data)->doWriteTransform(unhidetrans);  
            gchar * is_inverse_str = is_inverse.param_getSVGValue();
            if (strcmp(is_inverse_str, "false") != 0) {
                is_inverse.param_setValue((Glib::ustring)"false", true);
            }
        }
        SPObject *elemref = document->getObjectById(Glib::ustring("lpe_") + Glib::ustring(this->getLPEObj()->getId()));
        if (elemref) {
            elemref ->deleteObject(false);
        } 
    }
    g_free(is_inverse_str);
}

void 
LPEPowerClip::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    SPClipPath *clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
    if(clip_path) {
        if(!keep_paths) {
            gchar * is_inverse_str = is_inverse.param_getSVGValue();
            std::vector<SPObject*> clip_path_list = clip_path->childList(true);
            for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
                SPObject * clip_data = *iter;
                if(!strcmp(is_inverse_str,"true")) {
                    removeInverse(SP_ITEM(clip_data));
                }
            }
            g_free(is_inverse_str);
        
        } else if (flatten) {
            clip_path->deleteObject();
        }
    }
}

Geom::PathVector
LPEPowerClip::doEffect_path(Geom::PathVector const & path_in){
    Geom::PathVector path_out = path_in;
    SPClipPath *clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
    if (!hide_clip && flatten && isVisible()) {
        path_out *= sp_item_transform_repr (sp_lpe_item).inverse();
        SPDocument * document = SP_ACTIVE_DOCUMENT;
        if (!document) {
            return path_out;
        }
        SPObject *elemref = document->getObjectById(Glib::ustring("lpe_") + Glib::ustring(this->getLPEObj()->getId()));
        if (elemref) {
            SPCurve * c = nullptr;
            c = SP_SHAPE(elemref)->getCurve();
            if (c) {
                Geom::PathVector c_pv = c->get_pathvector();
                c_pv *= sp_item_transform_repr (sp_lpe_item).inverse();
                Geom::PathIntersectionGraph *pig = new Geom::PathIntersectionGraph(c_pv, path_out);
                if (pig && !c_pv.empty() && !path_in.empty()) {
                    path_out = pig->getIntersection();
                }
                c->unref();
            }
        } else {
            if(clip_path) {
                std::vector<SPObject*> clip_path_list = clip_path->childList(true);
                for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
                    SPObject * clip_data = *iter;
                    flattenClip(SP_ITEM(clip_data), path_out);
                }
            }
        }
        path_out *= sp_item_transform_repr (sp_lpe_item);
    }
    return path_out;
}

void 
LPEPowerClip::doOnVisibilityToggled(SPLPEItem const* lpeitem)
{
    doBeforeEffect(lpeitem);
}

void
LPEPowerClip::flattenClip(SPItem * clip_data, Geom::PathVector &path_in)
{
    SPClipPath *clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
    if (SP_IS_GROUP(clip_data)) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
        for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
            SPItem *subitem = *iter;
            flattenClip(subitem, path_in);
        }
    } else if (SP_IS_SHAPE(clip_data)) {
        SPCurve * c = nullptr;
        c = SP_SHAPE(clip_data)->getCurve();
        if (c) {
            Geom::PathVector c_pv = c->get_pathvector();
            c_pv *= sp_item_transform_repr (sp_lpe_item).inverse();
            Geom::PathIntersectionGraph *pig = new Geom::PathIntersectionGraph(c_pv, path_in);
            if (pig && !c_pv.empty() && !path_in.empty()) {
                path_in = pig->getIntersection();
            }
            c->unref();
        }
    }
}

void sp_inverse_powerclip(Inkscape::Selection *sel) {
    if (!sel->isEmpty()) {
        auto selList = sel->items();
        for(auto i = boost::rbegin(selList); i != boost::rend(selList); ++i) {
            SPLPEItem* lpeitem = dynamic_cast<SPLPEItem*>(*i);
            if (lpeitem) {
                Effect::createAndApply(POWERCLIP, SP_ACTIVE_DOCUMENT, lpeitem);
                Effect* lpe = lpeitem->getCurrentLPE();
                lpe->getRepr()->setAttribute("is_inverse", "false");
                lpe->getRepr()->setAttribute("is_visible", "true");
                lpe->getRepr()->setAttribute("inverse", "true");
                lpe->getRepr()->setAttribute("flatten", "false");
                lpe->getRepr()->setAttribute("hide_clip", "false");
            }
        }
    }
}

void sp_remove_powerclip(Inkscape::Selection *sel) {
    if (!sel->isEmpty()) {
        auto selList = sel->items();
        for(auto i = boost::rbegin(selList); i != boost::rend(selList); ++i) {
            SPLPEItem* lpeitem = dynamic_cast<SPLPEItem*>(*i);
            if (lpeitem) {
                if (lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled()) {
                    for (PathEffectList::iterator it = lpeitem->path_effect_list->begin(); it != lpeitem->path_effect_list->end(); ++it)
                    {
                        LivePathEffectObject *lpeobj = (*it)->lpeobject;
                        if (!lpeobj) {
                            /** \todo Investigate the cause of this.
                             * For example, this happens when copy pasting an object with LPE applied. Probably because the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                            */
                            g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                            return;
                        }
                        Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                        if (lpe->getName() == "powerclip") {
                            lpe->doOnRemove(lpeitem);
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
