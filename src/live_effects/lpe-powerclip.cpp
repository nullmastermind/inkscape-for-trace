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

namespace Inkscape {
namespace LivePathEffect {

LPEPowerClip::LPEPowerClip(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
    inverse(_("Inverse clip"), _("Inverse clip"), "inverse", &wr, this, false),
    flatten(_("Flatten clip"), _("Flatten clip, see fill rule once convert to paths"), "flatten", &wr, this, false),
    //tooltip empty to no show in default param set
    is_inverse("Store the last inverse apply", "", "is_inverse", &wr, this, "false", false)
{
    registerParameter(&inverse);
    registerParameter(&flatten);
    registerParameter(&is_inverse);
    is_clip = false;
    hide_clip = false;
    convert_shapes = false;
}

LPEPowerClip::~LPEPowerClip() {}

void
LPEPowerClip::doBeforeEffect (SPLPEItem const* lpeitem){
    original_bbox(lpeitem);
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
        const Glib::ustring uri = (Glib::ustring)sp_lpe_item->getRepr()->attribute("clip-path");
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for ( std::vector<SPObject*>::const_iterator iter=clip_path_list.begin();iter!=clip_path_list.end();++iter) {
            SPObject * clip_data = *iter;
            SPObject * clip_to_path = NULL;
            if (SP_IS_SHAPE(clip_data) && !SP_IS_PATH(clip_data) && convert_shapes) {
                SPDocument * document = SP_ACTIVE_DOCUMENT;
                if (!document) {
                    return;
                }
                Inkscape::XML::Document *xml_doc = document->getReprDoc();
                Inkscape::XML::Node *clip_path_node = sp_selected_item_to_curved_repr(SP_ITEM(clip_data), 0);
                // remember the position of the item
                gint pos = clip_data->getRepr()->position();
                // remember parent
                Inkscape::XML::Node *parent = clip_data->getRepr()->parent();
                // remember id
                char const *id = clip_data->getRepr()->attribute("id");
                // remember title
                gchar *title = clip_data->title();
                // remember description
                gchar *desc = clip_data->desc();

                // It's going to resurrect, so we delete without notifying listeners.
                clip_data->deleteObject(false);

                // restore id
                clip_path_node->setAttribute("id", id);
                // add the new repr to the parent
                parent->appendChild(clip_path_node);
                clip_to_path = document->getObjectByRepr(clip_path_node);
                if (title && clip_to_path) {
                    clip_to_path->setTitle(title);
                    g_free(title);
                }
                if (desc && clip_to_path) {
                    clip_to_path->setDesc(desc);
                    g_free(desc);
                }
                // move to the saved position
                clip_path_node->setPosition(pos > 0 ? pos : 0);
                Inkscape::GC::release(clip_path_node);
                clip_to_path->emitModified(SP_OBJECT_MODIFIED_CASCADE);
            }
            if( inverse && isVisible()) {
                if (clip_to_path) {
                    addInverse(SP_ITEM(clip_to_path));
                } else {
                    addInverse(SP_ITEM(clip_data));
                }
            } else if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
                if (clip_to_path) {
                    removeInverse(SP_ITEM(clip_to_path));
                } else {
                    removeInverse(SP_ITEM(clip_data));
                }
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
        SPCurve * c = NULL;
        c = SP_SHAPE(clip_data)->getCurve();
        if (c) {
            Geom::PathVector c_pv = c->get_pathvector();
            if(c_pv.size() > 1 && is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
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
            SPCurve * c = NULL;
            c = SP_SHAPE(clip_data)->getCurve();
            if (c) {
                Geom::PathVector c_pv = c->get_pathvector();
                if(c_pv.size() > 1) {
                    c_pv.pop_back();
                }
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

void
LPEPowerClip::convertShapes() {
    convert_shapes = true;
    sp_lpe_item_update_patheffect(SP_LPE_ITEM(sp_lpe_item), false, false);
}

Gtk::Widget *
LPEPowerClip::newWidget()
{
    // use manage here, because after deletion of Effect object, others might still be pointing to this widget.
    Gtk::VBox * vbox = Gtk::manage( new Gtk::VBox(Effect::newWidget()) );

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
    Gtk::HBox * hbox = Gtk::manage(new Gtk::HBox(false,0));
    Gtk::Button * toggle_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Toggle clip visibiliy"))));
    toggle_button->signal_clicked().connect(sigc::mem_fun (*this,&LPEPowerClip::toggleClip));
    toggle_button->set_size_request(140,30);
    vbox->pack_start(*hbox, true,true,2);
    hbox->pack_start(*toggle_button, false, false,2);
    Gtk::HBox * hbox2 = Gtk::manage(new Gtk::HBox(false,0));
    Gtk::Button * topaths_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Convert clips to paths, undoable"))));
    topaths_button->signal_clicked().connect(sigc::mem_fun (*this,&LPEPowerClip::convertShapes));
    topaths_button->set_size_request(200,30);
    vbox->pack_start(*hbox2, true,true,2);
    hbox2->pack_start(*topaths_button, false, false,2);
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
