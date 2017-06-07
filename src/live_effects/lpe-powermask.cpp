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
#include "sp-defs.h"
#include "style.h"
#include "sp-item-group.h"
#include "svg/svg.h"
#include "ui/tools-switch.h"
#include "path-chemistry.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEPowerMask::LPEPowerMask(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
    invert(_("Invert mask"), _("Invert mask"), "invert", &wr, this, false),
    wrap(_("Wrap filtered clip data"), _("Wrap filtered clip data"), "wrap", &wr, this, false),
    background(_("Add background to mask"), _("Add background to mask"), "background", &wr, this, false),
    background_style(_("Background Style"), _("CSS to background"), "background_style", &wr, this,"fill:#ffffff;opacity:0.7;")
   
{
    registerParameter(&invert);
    registerParameter(&wrap);
    registerParameter(&background);
    registerParameter(&background_style);
    background_style.param_hide_canvas_text();
    hide_mask = false;
    previous_invert = !invert;
    previous_wrap = !wrap;
    previous_background_style = "";
}

LPEPowerMask::~LPEPowerMask() {}

void
LPEPowerMask::doBeforeEffect (SPLPEItem const* lpeitem){
    original_bbox(lpeitem);
    const Glib::ustring uri = (Glib::ustring)sp_lpe_item->getRepr()->attribute("mask");
    SPMask *mask = SP_ITEM(lpeitem)->mask_ref->getObject();
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
    //mask *= sp_lpe_item->i2dt_affine();
    if(mask) {
        setMask();
    }
}

void
LPEPowerMask::setMask(){
    SPMask *mask = SP_ITEM(sp_lpe_item)->mask_ref->getObject();
    SPObject *elemref = NULL;
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if(!document || !mask) {
        return;
    }
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *box = NULL;
    Inkscape::XML::Node *filter = NULL;
    SPDefs * defs = document->getDefs();
    Glib::ustring mask_id = (Glib::ustring)mask->getId();
    Glib::ustring box_id = mask_id + (Glib::ustring)"_transparentbox";
    Glib::ustring filter_id = mask_id + (Glib::ustring)"_inverse";
    Glib::ustring filter_uri = (Glib::ustring)"url(#" + mask_id + (Glib::ustring)"_inverse)";
    if (previous_invert != invert || previous_wrap != wrap) {
        if (invert) {
            if (!(elemref = document->getObjectById(filter_id))) {
                filter = xml_doc->createElement("svg:filter");
                filter->setAttribute("id", filter_id.c_str());
                filter->setAttribute("color-interpolation-filters", "sRGB");
                filter->setAttribute("height", "100");
                filter->setAttribute("width", "100");
                filter->setAttribute("x", "-50");
                filter->setAttribute("y", "-50");
                Inkscape::XML::Node *primitive1 =  xml_doc->createElement("svg:feColorMatrix");
                Glib::ustring primitive1_id = (mask_id + (Glib::ustring)"_primitive1").c_str();
                primitive1->setAttribute("id", primitive1_id.c_str());
                primitive1->setAttribute("values", "1");
                primitive1->setAttribute("type", "saturate");
                primitive1->setAttribute("result", "fbSourceGraphic");
                Inkscape::XML::Node *primitive2 =  xml_doc->createElement("svg:feColorMatrix");
                Glib::ustring primitive2_id = (mask_id + (Glib::ustring)"_primitive2").c_str();
                primitive2->setAttribute("id", primitive2_id.c_str());
                primitive2->setAttribute("values", "-1 0 0 0 1 0 -1 0 0 1 0 0 -1 0 1 0 0 0 1 0 ");
                primitive2->setAttribute("in", "fbSourceGraphic");
                elemref = defs->appendChildRepr(filter);
                filter->appendChild(primitive1);
                filter->appendChild(primitive2);
                Inkscape::GC::release(filter);
                Inkscape::GC::release(primitive1);
                Inkscape::GC::release(primitive2);
            }
        } else {
            if ((elemref = document->getObjectById(filter_id))) {
                elemref->deleteObject(true);
            }
            filter_uri = "";
        }
        std::vector<SPObject*> mask_list = mask->childList(true);
        for ( std::vector<SPObject*>::const_iterator iter=mask_list.begin();iter!=mask_list.end();++iter) {
            SPItem * mask_data = SP_ITEM(*iter);
            if (! strcmp(mask_data->getId(), box_id.c_str())){
                continue;
            }
            Glib::ustring mask_data_id = (Glib::ustring)mask_data->getId();
            SPCSSAttr *css = sp_repr_css_attr_new();
            if(mask_data->getRepr()->attribute("style")) {
                sp_repr_css_attr_add_from_string(css, mask_data->getRepr()->attribute("style"));
            }
            char const* filter = sp_repr_css_property (css, "filter", "");
            if(!filter ||! strcmp(filter, filter_uri.c_str())) {
                if (filter_uri.empty()) {
                    sp_repr_css_set_property (css, "filter", NULL);
                } else {
                    sp_repr_css_set_property (css, "filter", filter_uri.c_str());
                }
                Glib::ustring css_str;
                sp_repr_css_write_string(css, css_str);
                mask_data->getRepr()->setAttribute("style", css_str.c_str());
            } else if(wrap){
                Glib::ustring g_data_id = mask_data_id + (Glib::ustring)"_container";
                Inkscape::XML::Node * container = xml_doc->createElement("svg:g");
                container->setAttribute("id", g_data_id.c_str());
                mask->appendChildRepr(container);
                container->setPosition(mask_data->getPosition());
                container->appendChild(mask_data->getRepr());
                Inkscape::GC::release(container);
                SPCSSAttr *css = sp_repr_css_attr_new();
                if (filter_uri.empty()) {
                    sp_repr_css_set_property (css, "filter", NULL);
                } else {
                    sp_repr_css_set_property (css, "filter", filter_uri.c_str());
                }
                Glib::ustring css_str;
                sp_repr_css_write_string(css, css_str);
                container->setAttribute("style", css_str.c_str());
            }
        }
    }
    if (background) {
        if ((elemref = document->getObjectById(box_id))) {
            if (strcmp(previous_background_style, background_style.param_getSVGValue())) {
                elemref->getRepr()->setAttribute("style", background_style.param_getSVGValue());
            }
        } else {
            std::vector<SPObject*> mask_list = mask->childList(true);
            box = xml_doc->createElement("svg:path");
            box->setAttribute("id", box_id.c_str());
            box->setAttribute("style", background_style.param_getSVGValue());
            gchar * box_str = sp_svg_write_path( mask_box );
            box->setAttribute("d" , box_str);
            g_free(box_str);
            elemref = mask->appendChildRepr(box);
            box->setPosition(mask_list.size());
            Inkscape::GC::release(box);
            mask_list.clear();
        }
    } else {
        if ((elemref = document->getObjectById(box_id))) {
            elemref->deleteObject(true);
        }
    }
    previous_invert = invert;
    previous_wrap = wrap;
    previous_background_style = background_style.param_getSVGValue();
}

void
LPEPowerMask::toggleMask() {
    SPItem * item = SP_ITEM(sp_lpe_item);
    if (item) {
        SPMask *mask = item->mask_ref->getObject();
        if (mask) {
            hide_mask = !hide_mask;
            if(hide_mask) {
                SPItemView *v;
                for (v = item->display; v != NULL; v = v->next) {
                    mask->sp_mask_hide(v->arenaitem->key());
                }
            } else {
                Geom::OptRect bbox = item->geometricBounds();
                for (SPItemView *v = item->display; v != NULL; v = v->next) {
                    if (!v->arenaitem->key()) {
                        v->arenaitem->setKey(SPItem::display_key_new(3));
                    }
                    Inkscape::DrawingItem *ai = mask->sp_mask_show(
                                                           v->arenaitem->drawing(),
                                                           v->arenaitem->key());
                    v->arenaitem->setMask(ai);
                    mask->sp_mask_set_bbox(v->arenaitem->key(), bbox);
                }
            }
            mask->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
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
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void 
LPEPowerMask::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    SPMask *mask = SP_ITEM(sp_lpe_item)->mask_ref->getObject();
    if(!keep_paths) {
        if(mask) {
            invert.param_setValue(false);
            wrap.param_setValue(false);
            background.param_setValue(false);
            setMask();
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
