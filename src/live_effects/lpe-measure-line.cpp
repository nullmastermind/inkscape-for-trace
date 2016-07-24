/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)

 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-measure-line.h"
#include "inkscape.h"
#include "uri.h"
#include "uri-references.h"
#include "preferences.h"
#include "util/units.h"
#include "svg/svg-length.h"
#include "display/curve.h"
#include "sp-root.h"
#include "sp-shape.h"
#include "sp-path.h"
#include "desktop.h"
#include "document.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

LPEMeasureLine::LPEMeasureLine(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    fontselector(_("Font Selector:"), _("Font Selector"), "fontselector", &wr, this, " "),
    scale(_("Scale:"), _("Scaling factor"), "scale", &wr, this, 1.0),
    precision(_("Number precision"), _("Number precision"), "precision", &wr, this, 2),
    unit(_("Unit:"), _("Unit"), "unit", &wr, this),
    reverse(_("To other side"), _("To other side"), "reverse", &wr, this, false),
    pos(0,0),
    angle(0)
{
    registerParameter(&fontselector);
    registerParameter(&scale);
    registerParameter(&precision);
    registerParameter(&unit);
    registerParameter(&reverse);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    fontselector.param_update_default(prefs->getString("/live_effects/measure-line/fontselector"));
    scale.param_update_default(prefs->getDouble("/live_effects/measure-line/scale", 1.0));
    precision.param_update_default(prefs->getInt("/live_effects/measure-line/precision", 2));
    unit.param_update_default(prefs->getString("/live_effects/measure-line/unit"));
    reverse.param_update_default(prefs->getBool("/live_effects/measure-line/reverse"));
    rtext = NULL;
    precision.param_set_range(0, 100);
    precision.param_set_increments(1, 1);
    precision.param_set_digits(0);
    precision.param_make_integer(true);
    fontlister = Inkscape::FontLister::get_instance();
}

LPEMeasureLine::~LPEMeasureLine() {}

void
LPEMeasureLine::doOnApply(SPLPEItem const* lpeitem)
{
    if (!SP_IS_SHAPE(lpeitem)) {
        g_warning("LPE measure line can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
        item->removeCurrentPathEffect(false);
    }
}

void
LPEMeasureLine::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
    SPPath *sp_path = dynamic_cast<SPPath *>(splpeitem);
    if (sp_path) {
        Geom::PathVector pathvector = sp_path->get_original_curve()->get_pathvector();
        if (SP_ACTIVE_DESKTOP) {
            Geom::Point s = pathvector.initialPoint();
            Geom::Point e =  pathvector.finalPoint();
            pos = Geom::middle_point(s,e);
            Geom::Ray ray(s,e);
            angle = ray.angle();
            SPDesktop *desktop = SP_ACTIVE_DESKTOP;
            doc_unit = Inkscape::Util::unit_table.getUnit(desktop->doc()->getRoot()->height.unit)->abbr;
            if (reverse) {
                angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
                if (angle < 0) angle += 2*M_PI;
            }
            Geom::Coord angle_cross = std::fmod(angle + rad_from_deg(90), 2*M_PI);
            if (angle_cross < 0) angle_cross += 2*M_PI;
            Geom::Point newpos = pos - Point::polar(angle_cross, 10);
            Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
            Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + (Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_mlsize").c_str());
            Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
            SVGElemRef->attach(SVGElem_uri);
            SPObject *elemref = NULL;
            Inkscape::XML::Node *rtspan = NULL;
            if (elemref = SVGElemRef->getObject()) {
                rtext = elemref->getRepr();
                sp_repr_set_svg_double(rtext, "x", newpos[Geom::X]);
                sp_repr_set_svg_double(rtext, "y", newpos[Geom::Y]);
            } else {
                rtext = xml_doc->createElement("svg:text");
                rtext->setAttribute("xml:space", "preserve");
                rtext->setAttribute("id", ((Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_mlsize").c_str());
                /* Set style */
                sp_repr_set_svg_double(rtext, "x", newpos[Geom::X]);
                sp_repr_set_svg_double(rtext, "y", newpos[Geom::Y]);
                /* Create <tspan> */
                rtspan = xml_doc->createElement("svg:tspan");
                rtspan->setAttribute("sodipodi:role", "line");
            }
            SPCSSAttr *css = sp_repr_css_attr_new();
            Glib::ustring fontspec = fontselector.param_readFontSpec(fontselector.param_getSVGValue());
            double fontsize = fontselector.param_readFontSize(fontselector.param_getSVGValue());
            fontlister->fill_css( css, fontspec );
            std::stringstream font_size;
            font_size.imbue(std::locale::classic());
            font_size <<  fontsize << "pt";
            sp_repr_css_set_property (css, "font-size", font_size.str().c_str());
            sp_repr_css_set_property (css, "font-style", "normal");
            sp_repr_css_set_property (css, "font-weight", "normal");
            sp_repr_css_set_property (css, "line-height", "125%");
            sp_repr_css_set_property (css, "letter-spacing", "0");
            sp_repr_css_set_property (css, "word-spacing", "0");
            sp_repr_css_set_property (css, "text-align", "center");
            sp_repr_css_set_property (css, "text-anchor", "middle");
            sp_repr_css_set_property (css, "fill", "#000000");
            sp_repr_css_set_property (css, "fill-opacity", "1");
            sp_repr_css_set_property (css, "stroke", "none");
            Glib::ustring css_str;
            sp_repr_css_write_string(css,css_str);
            if (!rtspan) {
                rtspan = rtext->firstChild();
            }
            rtspan->setAttribute("style", css_str.c_str());
            sp_repr_css_attr_unref (css);
            if (!elemref) {
                rtext->addChild(rtspan, NULL);
                Inkscape::GC::release(rtspan);
            }
            /* Create TEXT */
            length = Inkscape::Util::Quantity::convert(length, doc_unit.c_str(), unit.get_abbreviation());
            std::stringstream length_str;
            length_str.imbue(std::locale::classic());
            length_str << "%." << precision << "f %s";
            gchar *lengthchar = g_strdup_printf(length_str.str().c_str(), length, unit.get_abbreviation());
            Inkscape::XML::Node *rstring = NULL;
            if (!elemref) {
                rstring = xml_doc->createTextNode(lengthchar);
                rtspan->addChild(rstring, NULL);
                Inkscape::GC::release(rstring);
            } else {
                rstring = rtspan->firstChild();
                rstring->setContent(lengthchar);
            }
            SPObject * text_obj = NULL;
            if (!elemref) {
                text_obj = SP_OBJECT(desktop->currentLayer()->appendChildRepr(rtext));
                Inkscape::GC::release(rtext);
            } else {
                text_obj = desktop->currentLayer()->get_child_by_repr(rtext);
            }
            Geom::Affine affine = Geom::Affine(Geom::Translate(newpos).inverse());
            affine *= Geom::Rotate(angle);
            affine *= Geom::Translate(newpos);
            SP_ITEM(text_obj)->transform = affine;
            text_obj->updateRepr();
        }
    }
}

Gtk::Widget *LPEMeasureLine::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(Effect::newWidget()));

    vbox->set_border_width(5);
    vbox->set_homogeneous(false);
    vbox->set_spacing(6);

    std::vector<Parameter *>::iterator it = param_vector.begin();
    Gtk::HBox * button1 = Gtk::manage(new Gtk::HBox(true,0));
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter *param = *it;
            Gtk::Widget *widg = dynamic_cast<Gtk::Widget *>(param->param_newWidget());
            Glib::ustring *tip = param->param_getTooltip();
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
    Gtk::Button *save_default = Gtk::manage(new Gtk::Button(Glib::ustring(_("Save as default"))));
    save_default->signal_clicked().connect(sigc::mem_fun(*this, &LPEMeasureLine::saveDefault));
    button1->pack_start(*save_default, true, true, 2);
    vbox->pack_start(*button1, true, true, 2);
    return dynamic_cast<Gtk::Widget *>(vbox);
}

Geom::PathVector
LPEMeasureLine::doEffect_path(Geom::PathVector const &path_in)
{

    length = Geom::distance(path_in.initialPoint(), path_in.finalPoint())  * scale;
    Geom::Path path;
    Geom::Point s = path_in.initialPoint();
    Geom::Point e =  path_in.finalPoint();
    path.start( s );
    path.appendNew<Geom::LineSegment>( e );
    Geom::PathVector output;
    output.push_back(path);
    return output;
}

void
LPEMeasureLine::saveDefault()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/live_effects/measure-line/fontselector", (Glib::ustring)fontselector.param_getSVGValue());
    prefs->setDouble("/live_effects/measure-line/scale", scale);
    prefs->setInt("/live_effects/measure-line/precision", precision);
    prefs->setString("/live_effects/measure-line/unit", (Glib::ustring)unit.get_abbreviation());
    prefs->setInt("/live_effects/measure-line/reverse", reverse);
}

}; //namespace LivePathEffect
}; /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offset:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
