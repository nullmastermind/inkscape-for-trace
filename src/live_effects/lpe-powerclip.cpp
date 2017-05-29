/*
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-powerclip.h"
#include <2geom/path-intersection.h>
#include <2geom/intersection-graph.h>
#include "display/drawing-item.h"
#include "display/curve.h"
#include "helper/geom.h"
#include "sp-clippath.h"
#include "sp-path.h"
#include "sp-shape.h"
#include "sp-item-group.h"
#include "ui/tools-switch.h"
#include "path-chemistry.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

// FIXME: expose these from sp-clippath/mask.cpp
struct SPClipPathView {
    SPClipPathView *next;
    unsigned int key;
    Inkscape::DrawingItem *arenaitem;
    Geom::OptRect bbox;
};

namespace Inkscape {
namespace LivePathEffect {

LPEPowerClip::LPEPowerClip(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
    inverse(_("Inverse clip"), _("Inverse clip"), "inverse", &wr, this, false),
    flatten(_("Flatten clip"), _("Flatten clip"), "flatten", &wr, this, false),
    fillrule(_("Set/unset evenodd fill rule"), _("Set/unset evenodd fill rule (this is overwriting your current value)."), "fillrule", &wr, this, false),
    convert_shapes(_("Convert clip shapes to paths"), _("Convert clip shapes to paths (this is overwriting your current value)."), "convert_shapes", &wr, this, false),
    //tooltip empty to no show in default param set
    is_inverse("Store the last inverse apply", "", "is_inverse", &wr, this, "false", false)
{
    registerParameter(&inverse);
    registerParameter(&flatten);
    registerParameter(&fillrule);
    registerParameter(&convert_shapes);
    registerParameter(&is_inverse);
    is_clip = false;
    previous_fillrule = fillrule;
    hide_clip = false;
}

LPEPowerClip::~LPEPowerClip() {}

void
LPEPowerClip::doBeforeEffect (SPLPEItem const* lpeitem){
    original_bbox(lpeitem);
    const Glib::ustring uri = (Glib::ustring)sp_lpe_item->getRepr()->attribute("clip-path");
    SPClipPath *clip_path = SP_ITEM(lpeitem)->clip_ref->getObject();
    Geom::Point topleft      = Geom::Point(boundingbox_X.min() - 5,boundingbox_Y.max() + 5);
    Geom::Point topright     = Geom::Point(boundingbox_X.max() + 5,boundingbox_Y.max() + 5);
    Geom::Point bottomright  = Geom::Point(boundingbox_X.max() + 5,boundingbox_Y.min() - 5);
    Geom::Point bottomleft   = Geom::Point(boundingbox_X.min() - 5,boundingbox_Y.min() - 5);
    clip_box.clear();
    clip_box.start(topleft);
    clip_box.appendNew<Geom::LineSegment>(topright);
    clip_box.appendNew<Geom::LineSegment>(bottomright);
    clip_box.appendNew<Geom::LineSegment>(bottomleft);
    clip_box.close();
    //clip_path *= sp_lpe_item->i2dt_affine();
    if(clip_path) {
        is_clip = true;
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
            SPObject * clip_data = *iter;
            if (SP_IS_SHAPE(clip_data) && !SP_IS_PATH(clip_data) && convert_shapes) {
                SPDocument * document = SP_ACTIVE_DOCUMENT;
                if (!document) {
                    return;
                }
                Inkscape::XML::Document *xml_doc = document->getReprDoc();
                const char * id = clip_data->getId();
                Inkscape::XML::Node *clip_path_node = sp_selected_item_to_curved_repr(SP_ITEM(clip_data), 0);
                clip_data->updateRepr(xml_doc, clip_path_node, SP_OBJECT_WRITE_ALL);
                clip_data->getRepr()->setAttribute("id", id);
                clip_path->emitModified(SP_OBJECT_MODIFIED_CASCADE);
                std::cout << "toshapes\n";
            }
            if( inverse && isVisible()) {
                addInverse(SP_ITEM(clip_data));
            } else if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
                removeInverse(SP_ITEM(clip_data));
            }
        }
    } else {
        is_clip = false;
    }
}

void
LPEPowerClip::addInverse (SPItem * clip_data){
    if (SP_IS_GROUP(clip_data)) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
        for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
            SPItem *subitem = *iter;
            addInverse(subitem);
        }
    } else if (SP_IS_PATH(clip_data)) {
        setFillRule(clip_data);
        SPCurve * c = NULL;
        c = SP_SHAPE(clip_data)->getCurve();
        if (c) {
            Geom::PathVector c_pv = c->get_pathvector();
            if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
               c_pv.pop_back();
            }
            //TODO: this can be not correct but no better way
            bool dir_a = Geom::path_direction(c_pv[0]);
            bool dir_b = Geom::path_direction(clip_box);
            if (dir_a == dir_b) {
               clip_box = clip_box.reversed();
            }
            c_pv.push_back(clip_box);
            c->set_pathvector(c_pv);
            SP_SHAPE(clip_data)->setCurve(c, TRUE);
            c->unref();
            is_inverse.param_setValue((Glib::ustring)"true", true);
            SPDesktop *desktop = SP_ACTIVE_DESKTOP;
            if (desktop) {
                if (tools_isactive(desktop, TOOLS_NODES)) {
                    Inkscape::Selection * sel = SP_ACTIVE_DESKTOP->getSelection();
                    SPItem * item = sel->singleItem();
                    if (item != NULL) {
                        sel->remove(item);
                        sel->add(item);
                    }
                }
            }
        }
    }
}

void
LPEPowerClip::removeInverse (SPItem * clip_data){
    if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
        if (SP_IS_GROUP(clip_data)) {
             std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
             for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
                 SPItem *subitem = *iter;
                 removeInverse(subitem);
             }
        } else if (SP_IS_PATH(clip_data)) {
            setFillRule(clip_data);
            SPCurve * c = NULL;
            c = SP_SHAPE(clip_data)->getCurve();
            if (c) {
                Geom::PathVector c_pv = c->get_pathvector();
                c_pv.pop_back();
                c->set_pathvector(c_pv);
                SP_SHAPE(clip_data)->setCurve(c, TRUE);
                c->unref();
                is_inverse.param_setValue((Glib::ustring)"false", true);
                SPDesktop *desktop = SP_ACTIVE_DESKTOP;
                if (desktop) {
                    if (tools_isactive(desktop, TOOLS_NODES)) {
                        Inkscape::Selection * sel = SP_ACTIVE_DESKTOP->getSelection();
                        SPItem * item = sel->singleItem();
                        if (item != NULL) {
                            sel->remove(item);
                            sel->add(item);
                        }
                    }
                }
            }
        }
    }
}

void
LPEPowerClip::toggleClip() {
    SPItem * item = SP_ITEM(sp_lpe_item);
    if (item) {
        SPClipPath *clip_path = item->clip_ref->getObject();
        if (clip_path) {
            hide_clip = !hide_clip;
            if(hide_clip) {
                SPItemView *v;
                for (v = item->display; v != NULL; v = v->next) {
                    clip_path->hide(v->arenaitem->key());
                }
            } else {
                Geom::OptRect bbox = item->geometricBounds();
                for (SPItemView *v = item->display; v != NULL; v = v->next) {
                    if (!v->arenaitem->key()) {
                        v->arenaitem->setKey(SPItem::display_key_new(3));
                    }
                    Inkscape::DrawingItem *ai = clip_path->show(
                                                           v->arenaitem->drawing(),
                                                           v->arenaitem->key());
                    v->arenaitem->setClip(ai);
                    clip_path->setBBox(v->arenaitem->key(), bbox);
                }
            }
            clip_path->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
}

Gtk::Widget *
LPEPowerClip::newWidget()
{
    // use manage here, because after deletion of Effect object, others might still be pointing to this widget.
    Gtk::VBox * vbox = Gtk::manage( new Gtk::VBox(Effect::newWidget()) );

    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(6);
    Gtk::HBox * hbox = Gtk::manage(new Gtk::HBox(false,0));
    Gtk::Button * toggle_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Toggle clip visibiliy"))));
    toggle_button->signal_clicked().connect(sigc::mem_fun (*this,&LPEPowerClip::toggleClip));
    toggle_button->set_size_request(140,30);
    vbox->pack_start(*hbox, true,true,2);
    hbox->pack_start(*toggle_button, false, false,2);
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter * param = *it;
            Gtk::Widget * widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
            if(param->param_key == "grid") {
                widg = NULL;
            }
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
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void 
LPEPowerClip::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    SPClipPath *clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
    if(!keep_paths) {
        if(clip_path) {
            is_clip = true;
            std::vector<SPObject*> clip_path_list = clip_path->childList(true);
            for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
                SPObject * clip_data = *iter;
                if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
                    removeInverse(SP_ITEM(clip_data));
                    is_inverse.param_setValue((Glib::ustring)"false");
                }
            }
        }
    } else {
        if (flatten && clip_path) {
            clip_path->deleteObject();
            sp_lpe_item->getRepr()->setAttribute("clip-path", NULL);
        }
    }
}

Geom::PathVector
LPEPowerClip::doEffect_path(Geom::PathVector const & path_in){
    Geom::PathVector path_out = pathv_to_linear_and_cubic_beziers(path_in);
    if (flatten && is_clip && isVisible()) {
        SPClipPath *clip_path = SP_ITEM(sp_lpe_item)->clip_ref->getObject();
        if(clip_path) {
            std::vector<SPObject*> clip_path_list = clip_path->childList(true);
            for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
                SPObject * clip_data = *iter;
                flattenClip(SP_ITEM(clip_data), path_out);
            }
        }
    }
    return path_out;
}

void
LPEPowerClip::flattenClip(SPItem * clip_data, Geom::PathVector &path_in)
{
    if (SP_IS_GROUP(clip_data)) {
         std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(clip_data));
         for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
             SPItem *subitem = *iter;
             flattenClip(subitem, path_in);
         }
    } else if (SP_IS_PATH(clip_data)) {
        if (!SP_IS_PATH(clip_data) && convert_shapes) {
            SPDocument * document = SP_ACTIVE_DOCUMENT;
            if (!document) {
                return;
            }
            Inkscape::XML::Document *xml_doc = document->getReprDoc();
            const char * id = clip_data->getId();
            Inkscape::XML::Node *clip_path_node = sp_selected_item_to_curved_repr(clip_data, 0);
            clip_data->updateRepr(xml_doc, clip_path_node, SP_OBJECT_WRITE_ALL);
            clip_data->getRepr()->setAttribute("id", id);
        }
        SPCurve * c = NULL;
        c = SP_SHAPE(clip_data)->getCurve();
        if (c) {
            Geom::PathVector c_pv = c->get_pathvector();
            Geom::PathIntersectionGraph *pig = new Geom::PathIntersectionGraph(c_pv, path_in);
            if (pig && !c_pv.empty() && !path_in.empty()) {
                path_in = pig->getIntersection();
            }
            c->unref();
        }
    }
}

void
LPEPowerClip::setFillRule(SPItem * clip_data)
{
    if (previous_fillrule != fillrule) {
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_attr_add_from_string(css, clip_data->getRepr()->attribute("style"));
        if (fillrule) {
           sp_repr_css_set_property (css, "fill-rule", "evenodd");
        } else {
           sp_repr_css_set_property (css, "fill-rule", "nonzero");
        }
        Glib::ustring css_str;
        sp_repr_css_write_string(css,css_str);
        clip_data->getRepr()->setAttribute("style", css_str.c_str());
        previous_fillrule = fillrule;
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
