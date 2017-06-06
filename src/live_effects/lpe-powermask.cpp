/*
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-powermask.h"
#include <2geom/path-intersection.h>
#include <2geom/intersection-graph.h>
#include "display/drawing-item.h"
#include "display/curve.h"
#include "helper/geom.h"
#include "sp-mask.h"
#include "sp-path.h"
#include "sp-shape.h"
#include "sp-item-group.h"
#include "ui/tools-switch.h"
#include "path-chemistry.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEPowerMask::LPEPowerMask(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
    inverse(_("Inverse mask"), _("Inverse mask"), "inverse", &wr, this, false),
    flatten(_("Flatten mask"), _("Flatten mask, see fill rule once convert to paths"), "flatten", &wr, this, false),
    //tooltip empty to no show in default param set
    is_inverse("Store the last inverse apply", "", "is_inverse", &wr, this, "false", false)
{
    registerParameter(&inverse);
    registerParameter(&flatten);
    registerParameter(&is_inverse);
    is_mask = false;
    hide_mask = false;
    convert_shapes = false;
}

LPEPowerMask::~LPEPowerMask() {}

void
LPEPowerMask::doBeforeEffect (SPLPEItem const* lpeitem){
    original_bbox(lpeitem);
    const Glib::ustring uri = (Glib::ustring)sp_lpe_item->getRepr()->attribute("mask-path");
    SPMaskPath *mask_path = SP_ITEM(lpeitem)->mask_ref->getObject();
    Geom::Point topleft      = Geom::Point(boundingbox_X.min() - 5,boundingbox_Y.max() + 5);
    Geom::Point topright     = Geom::Point(boundingbox_X.max() + 5,boundingbox_Y.max() + 5);
    Geom::Point bottomright  = Geom::Point(boundingbox_X.max() + 5,boundingbox_Y.min() - 5);
    Geom::Point bottomleft   = Geom::Point(boundingbox_X.min() - 5,boundingbox_Y.min() - 5);
    mask_box.clear();
    mask_box.start(topleft);
    mask_box.appendNew<Geom::LineSegment>(topright);
    mask_box.appendNew<Geom::LineSegment>(bottomright);
    mask_box.appendNew<Geom::LineSegment>(bottomleft);
    mask_box.close();
    //mask_path *= sp_lpe_item->i2dt_affine();
    if(mask_path) {
        is_mask = true;
        std::vector<SPObject*> mask_path_list = mask_path->childList(true);
        for ( std::vector<SPObject*>::const_iterator iter=mask_path_list.begin();iter!=mask_path_list.end();++iter) {
            SPObject * mask_data = *iter;
            SPObject * mask_to_path = NULL;
            if (SP_IS_SHAPE(mask_data) && !SP_IS_PATH(mask_data) && convert_shapes) {
                SPDocument * document = SP_ACTIVE_DOCUMENT;
                if (!document) {
                    return;
                }
                Inkscape::XML::Document *xml_doc = document->getReprDoc();
                Inkscape::XML::Node *mask_path_node = sp_selected_item_to_curved_repr(SP_ITEM(mask_data), 0);
                // remember the position of the item
                gint pos = mask_data->getRepr()->position();
                // remember parent
                Inkscape::XML::Node *parent = mask_data->getRepr()->parent();
                // remember id
                char const *id = mask_data->getRepr()->attribute("id");
                // remember title
                gchar *title = mask_data->title();
                // remember description
                gchar *desc = mask_data->desc();

                // It's going to resurrect, so we delete without notifying listeners.
                mask_data->deleteObject(false);

                // restore id
                mask_path_node->setAttribute("id", id);
                // add the new repr to the parent
                parent->appendChild(mask_path_node);
                mask_to_path = document->getObjectByRepr(mask_path_node);
                if (title && mask_to_path) {
                    mask_to_path->setTitle(title);
                    g_free(title);
                }
                if (desc && mask_to_path) {
                    mask_to_path->setDesc(desc);
                    g_free(desc);
                }
                // move to the saved position
                mask_path_node->setPosition(pos > 0 ? pos : 0);
                Inkscape::GC::release(mask_path_node);
                mask_to_path->emitModified(SP_OBJECT_MODIFIED_CASCADE);
            }
            if( inverse && isVisible()) {
                if (mask_to_path) {
                    addInverse(SP_ITEM(mask_to_path));
                } else {
                    addInverse(SP_ITEM(mask_data));
                }
            } else if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
                if (mask_to_path) {
                    removeInverse(SP_ITEM(mask_to_path));
                } else {
                    removeInverse(SP_ITEM(mask_data));
                }
            }
        }
    } else {
        is_mask = false;
    }
}

void
LPEPowerMask::addInverse (SPItem * mask_data){
    if (SP_IS_GROUP(mask_data)) {
        std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(mask_data));
        for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
            SPItem *subitem = *iter;
            addInverse(subitem);
        }
    } else if (SP_IS_PATH(mask_data)) {
        SPCurve * c = NULL;
        c = SP_SHAPE(mask_data)->getCurve();
        if (c) {
            Geom::PathVector c_pv = c->get_pathvector();
            if(c_pv.size() > 1 && is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
               c_pv.pop_back();
            }
            //TODO: this can be not correct but no better way
            bool dir_a = Geom::path_direction(c_pv[0]);
            bool dir_b = Geom::path_direction(mask_box);
            if (dir_a == dir_b) {
               mask_box = mask_box.reversed();
            }
            c_pv.push_back(mask_box);
            c->set_pathvector(c_pv);
            SP_SHAPE(mask_data)->setCurve(c, TRUE);
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
LPEPowerMask::removeInverse (SPItem * mask_data){
    if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
        if (SP_IS_GROUP(mask_data)) {
             std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(mask_data));
             for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
                 SPItem *subitem = *iter;
                 removeInverse(subitem);
             }
        } else if (SP_IS_PATH(mask_data)) {
            SPCurve * c = NULL;
            c = SP_SHAPE(mask_data)->getCurve();
            if (c) {
                Geom::PathVector c_pv = c->get_pathvector();
                if(c_pv.size() > 1) {
                    c_pv.pop_back();
                }
                c->set_pathvector(c_pv);
                SP_SHAPE(mask_data)->setCurve(c, TRUE);
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
LPEPowerMask::toggleMask() {
    SPItem * item = SP_ITEM(sp_lpe_item);
    if (item) {
        SPMaskPath *mask_path = item->mask_ref->getObject();
        if (mask_path) {
            hide_mask = !hide_mask;
            if(hide_mask) {
                SPItemView *v;
                for (v = item->display; v != NULL; v = v->next) {
                    mask_path->hide(v->arenaitem->key());
                }
            } else {
                Geom::OptRect bbox = item->geometricBounds();
                for (SPItemView *v = item->display; v != NULL; v = v->next) {
                    if (!v->arenaitem->key()) {
                        v->arenaitem->setKey(SPItem::display_key_new(3));
                    }
                    Inkscape::DrawingItem *ai = mask_path->show(
                                                           v->arenaitem->drawing(),
                                                           v->arenaitem->key());
                    v->arenaitem->setMask(ai);
                    mask_path->setBBox(v->arenaitem->key(), bbox);
                }
            }
            mask_path->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
}

void
LPEPowerMask::convertShapes() {
    convert_shapes = true;
    sp_lpe_item_update_patheffect(SP_LPE_ITEM(sp_lpe_item), false, false);
}

Gtk::Widget *
LPEPowerMask::newWidget()
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
    Gtk::HBox * hbox = Gtk::manage(new Gtk::HBox(false,0));
    Gtk::Button * toggle_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Toggle mask visibiliy"))));
    toggle_button->signal_clicked().connect(sigc::mem_fun (*this,&LPEPowerMask::toggleMask));
    toggle_button->set_size_request(140,30);
    vbox->pack_start(*hbox, true,true,2);
    hbox->pack_start(*toggle_button, false, false,2);
    Gtk::HBox * hbox2 = Gtk::manage(new Gtk::HBox(false,0));
    Gtk::Button * topaths_button = Gtk::manage(new Gtk::Button(Glib::ustring(_("Convert masks to paths, undoable"))));
    topaths_button->signal_clicked().connect(sigc::mem_fun (*this,&LPEPowerMask::convertShapes));
    topaths_button->set_size_request(200,30);
    vbox->pack_start(*hbox2, true,true,2);
    hbox2->pack_start(*topaths_button, false, false,2);
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void 
LPEPowerMask::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    SPMaskPath *mask_path = SP_ITEM(sp_lpe_item)->mask_ref->getObject();
    if(!keep_paths) {
        if(mask_path) {
            is_mask = true;
            std::vector<SPObject*> mask_path_list = mask_path->childList(true);
            for ( std::vector<SPObject*>::const_iterator iter=mask_path_list.begin();iter!=mask_path_list.end();++iter) {
                SPObject * mask_data = *iter;
                if(is_inverse.param_getSVGValue() == (Glib::ustring)"true") {
                    removeInverse(SP_ITEM(mask_data));
                    is_inverse.param_setValue((Glib::ustring)"false");
                }
            }
        }
    } else {
        if (flatten && mask_path) {
            mask_path->deleteObject();
            sp_lpe_item->getRepr()->setAttribute("mask-path", NULL);
        }
    }
}

Geom::PathVector
LPEPowerMask::doEffect_path(Geom::PathVector const & path_in){
    Geom::PathVector path_out = pathv_to_linear_and_cubic_beziers(path_in);
    if (flatten && is_mask && isVisible()) {
        SPMaskPath *mask_path = SP_ITEM(sp_lpe_item)->mask_ref->getObject();
        if(mask_path) {
            std::vector<SPObject*> mask_path_list = mask_path->childList(true);
            for ( std::vector<SPObject*>::const_iterator iter=mask_path_list.begin();iter!=mask_path_list.end();++iter) {
                SPObject * mask_data = *iter;
                flattenMask(SP_ITEM(mask_data), path_out);
            }
        }
    }
    return path_out;
}

void
LPEPowerMask::flattenMask(SPItem * mask_data, Geom::PathVector &path_in)
{
    if (SP_IS_GROUP(mask_data)) {
         std::vector<SPItem*> item_list = sp_item_group_item_list(SP_GROUP(mask_data));
         for ( std::vector<SPItem*>::const_iterator iter=item_list.begin();iter!=item_list.end();++iter) {
             SPItem *subitem = *iter;
             flattenMask(subitem, path_in);
         }
    } else if (SP_IS_PATH(mask_data)) {
        SPCurve * c = NULL;
        c = SP_SHAPE(mask_data)->getCurve();
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
