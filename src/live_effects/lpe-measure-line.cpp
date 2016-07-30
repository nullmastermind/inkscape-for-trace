/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)

 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-measure-line.h"
#include <pangomm/fontdescription.h>
#include <libnrtype/font-lister.h>
#include "inkscape.h"
#include "xml/node.h"
#include "uri.h"
#include "uri-references.h"
#include "preferences.h"
#include "util/units.h"
#include "svg/svg-length.h"
#include "svg/svg-color.h"
#include "svg/svg.h"
#include "display/curve.h"
#include <2geom/affine.h>
#include "style.h"
#include "sp-root.h"
#include "sp-item.h"
#include "sp-shape.h"
#include "sp-path.h"
#include "desktop.h"
#include "document.h"
#include <iomanip>

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<OrientationMethod> OrientationMethodData[] = {
    { OM_HORIZONTAL, N_("Horizontal"), "horizontal" }, 
    { OM_VERTICAL, N_("Vertical"), "vertical" },
    { OM_PARALLEL, N_("Parallel"), "parallel" }
};
static const Util::EnumDataConverter<OrientationMethod> OMConverter(OrientationMethodData, OM_END);

LPEMeasureLine::LPEMeasureLine(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    fontbutton(_("Font*"), _("Font Selector"), "fontbutton", &wr, this),
    orientation(_("Orientation"), _("Orientation method"), "orientation", OMConverter, &wr, this, OM_PARALLEL, false),
    curve_linked(_("Curve on origin"), _("Curve on origin, set 0 to start/end"), "curve_linked", &wr, this, 1),
    scale(_("Scale*"), _("Scaling factor"), "scale", &wr, this, 1.0),
    precision(_("Precision*"), _("Precision"), "precision", &wr, this, 2),
    offset_right_left(_("Offset right left*"), _("Offset right left"), "offset_right_left", &wr, this, 0),
    offset_top_bottom(_("Offset top bottom*"), _("Offset top bottom"), "offset_top_bottom", &wr, this, 5),
    gap_start(_("Gap to line from origin"), _("Gap to line from origin, without affecting measure"), "gap_start", &wr, this, 0),
    gap_end(_("Gap to line from end"), _("Gap to line from end, without affecting measure"), "gap_end", &wr, this, 0),
    unit(_("Unit*"), _("Unit"), "unit", &wr, this),
    reverse(_("To other side*"), _("To other side"), "reverse", &wr, this, false),
    color_as_line(_("Measure color as line*"), _("Measure color as line"), "color_as_line", &wr, this, false),
    scale_insensitive(_("Scale insensitive*"), _("Scale insensitive to transforms in element, parents..."), "scale_insensitive", &wr, this, true),
    local_locale(_("Local Number Format*"), _("Local number format"), "local_locale", &wr, this, true)
{
    registerParameter(&fontbutton);
    registerParameter(&orientation);
    registerParameter(&curve_linked);
    registerParameter(&scale);
    registerParameter(&precision);
    registerParameter(&offset_right_left);
    registerParameter(&offset_top_bottom);
    registerParameter(&gap_start);
    registerParameter(&gap_end);
    registerParameter(&unit);
    registerParameter(&reverse);
    registerParameter(&color_as_line);
    registerParameter(&scale_insensitive);
    registerParameter(&local_locale);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    fontbutton.param_update_default(prefs->getString("/live_effects/measure-line/fontbutton"));
    scale.param_update_default(prefs->getDouble("/live_effects/measure-line/scale", 1.0));
    precision.param_update_default(prefs->getInt("/live_effects/measure-line/precision", 2));
    offset_right_left.param_update_default(prefs->getDouble("/live_effects/measure-line/offset_right_left", 0.0));
    offset_top_bottom.param_update_default(prefs->getDouble("/live_effects/measure-line/offset_top_bottom", 5.0));
    unit.param_update_default(prefs->getString("/live_effects/measure-line/unit"));
    reverse.param_update_default(prefs->getBool("/live_effects/measure-line/reverse"));
    color_as_line.param_update_default(prefs->getBool("/live_effects/measure-line/color_as_line"));
    scale_insensitive.param_update_default(prefs->getBool("/live_effects/measure-line/scale_insensitive"));
    local_locale.param_update_default(prefs->getBool("/live_effects/measure-line/local_locale"));
    precision.param_set_range(0, 100);
    precision.param_set_increments(1, 1);
    precision.param_set_digits(0);
    precision.param_make_integer(true);
    curve_linked.param_set_range(0, 999);
    curve_linked.param_set_increments(1, 1);
    curve_linked.param_set_digits(0);
    curve_linked.param_make_integer(true);
    precision.param_make_integer(true);
    offset_right_left.param_set_range(-999999.0, 999999.0);
    offset_right_left.param_set_increments(1, 1);
    offset_right_left.param_set_digits(2);
    offset_top_bottom.param_set_range(-999999.0, 999999.0);
    offset_top_bottom.param_set_increments(1, 1);
    offset_top_bottom.param_set_digits(2);
    gap_start.param_set_range(-999999.0, 999999.0);
    gap_start.param_set_increments(1, 1);
    gap_start.param_set_digits(2);
    gap_end.param_set_range(-999999.0, 999999.0);
    gap_end.param_set_increments(1, 1);
    gap_end.param_set_digits(2);
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
LPEMeasureLine::doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + (Glib::ustring)this->getRepr()->attribute("id") + (Glib::ustring)"_text").c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *rtext = NULL;
        if (elemref = SVGElemRef->getObject()) {
            rtext = elemref->getRepr();
            if (!this->isVisible()) {
                rtext->setAttribute("style", "display:none");
            } else {
                rtext->setAttribute("style", NULL);
            }
        }
    }
}

void
LPEMeasureLine::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
    SPPath *sp_path = dynamic_cast<SPPath *>(splpeitem);
    if (sp_path) {
        Geom::PathVector pathvector = sp_path->get_original_curve()->get_pathvector();
        if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
            size_t ncurves = pathvector.curveCount();
            curve_linked.param_set_range(0, ncurves);
            Geom::Point s = pathvector.initialPoint();
            Geom::Point e =  pathvector.finalPoint();
            if (curve_linked) { //0 start-end nodes
                s = pathvector.pointAt(curve_linked -1);
                e = pathvector.pointAt(curve_linked);
            }
            Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
            Glib::ustring orientation_str;
            Inkscape::XML::Node *rtext = NULL;
            if (orientation == OM_VERTICAL) {
                orientation_str = "vertical";
                Coord xpos = std::max(s[Geom::X],e[Geom::X]);
                if (reverse) {
                    xpos = std::min(s[Geom::X],e[Geom::X]);
                }
                s[Geom::X] = xpos;
                e[Geom::X] = xpos;
                if (s[Geom::Y] > e[Geom::Y]) {
                    swap(s,e);
                }
            }
            if (orientation == OM_HORIZONTAL) {
                orientation_str = "horizontal";
                Coord ypos = std::max(s[Geom::Y],e[Geom::Y]);
                if (reverse) {
                    ypos = std::min(s[Geom::Y],e[Geom::Y]);
                }
                s[Geom::Y] = ypos;
                e[Geom::Y] = ypos;
                if (s[Geom::X] < e[Geom::X]) {
                    swap(s,e);
                }
            }
            if (orientation == OM_PARALLEL) {
                orientation_str = "parallel";
            }
            double length = Geom::distance(s, e)  * scale;
            Geom::Point pos = Geom::middle_point(s,e);
            Geom::Ray ray(s,e);
            Geom::Coord angle = ray.angle();
            doc_unit = Inkscape::Util::unit_table.getUnit(desktop->doc()->getRoot()->height.unit)->abbr;
            if (reverse) {
                angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
                if (angle < 0) angle += 2*M_PI;
            }
            Geom::Point newpos = pos - Point::polar(angle, offset_right_left);
            Geom::Coord angle_cross = std::fmod(angle + rad_from_deg(90), 2*M_PI);
            if (angle_cross < 0) angle_cross += 2*M_PI;
            newpos = newpos - Point::polar(angle_cross, offset_top_bottom);
            Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + (Glib::ustring)this->getRepr()->attribute("id") + (Glib::ustring)"_text").c_str());
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
                rtext->setAttribute("sodipodi:insensitive", "true");
                rtext->setAttribute("id", ((Glib::ustring)this->getRepr()->attribute("id") + (Glib::ustring)"_text").c_str());
                /* Set style */
                sp_repr_set_svg_double(rtext, "x", newpos[Geom::X]);
                sp_repr_set_svg_double(rtext, "y", newpos[Geom::Y]);
                /* Create <tspan> */
                rtspan = xml_doc->createElement("svg:tspan");
                rtspan->setAttribute("sodipodi:role", "line");
            }
            SPCSSAttr *css = sp_repr_css_attr_new();
            Pango::FontDescription fontdesc((Glib::ustring)fontbutton.param_getSVGValue());
            double fontsize = fontdesc.get_size()/Pango::SCALE;
            Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
            fontlister->fill_css( css, (Glib::ustring)fontbutton.param_getSVGValue() );
            std::stringstream font_size;
            font_size.imbue(std::locale::classic());
            font_size <<  fontsize << "pt";
            sp_repr_css_set_property (css, "font-size", font_size.str().c_str());
            sp_repr_css_set_property (css, "line-height", "125%");
            sp_repr_css_set_property (css, "letter-spacing", "0");
            sp_repr_css_set_property (css, "word-spacing", "0");
            sp_repr_css_set_property (css, "text-align", "center");
            sp_repr_css_set_property (css, "text-anchor", "middle");
            if (color_as_line) {
                if (lpeitem->style) {
                    if (lpeitem->style->stroke.isPaintserver()) {
                        SPPaintServer * server = lpeitem->style->getStrokePaintServer();
                        if (server) {
                            Glib::ustring str;
                            str += "url(#";
                            str += server->getId();
                            str += ")";
                            sp_repr_css_set_property (css, "fill", str.c_str());
                        }
                    } else if (lpeitem->style->stroke.isColor()) {
                        gchar c[64];
                        sp_svg_write_color (c, sizeof(c), lpeitem->style->stroke.value.color.toRGBA32(SP_SCALE24_TO_FLOAT(lpeitem->style->stroke_opacity.value)));
                        sp_repr_css_set_property (css, "fill", c);
                    } else {
                        sp_repr_css_set_property (css, "fill", "#000000");
                    }
                } else {
                    sp_repr_css_unset_property (css, "#000000");
                }
            } else {
                sp_repr_css_set_property (css, "fill", "#000000");
            }
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
            if (!scale_insensitive) {
                Geom::Affine affinetransform = i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(desktop->doc()->getRoot()));
                length *= (affinetransform.expansionX() + affinetransform.expansionY()) / 2.0;
            }
            length = Inkscape::Util::Quantity::convert(length, doc_unit.c_str(), unit.get_abbreviation());
            std::stringstream length_str;
            length_str.precision(precision);
            length_str.setf(std::ios::fixed, std::ios::floatfield);
            if (local_locale) {
                length_str.imbue(std::locale(""));
            } else {
                length_str.imbue(std::locale::classic());
            }
            length_str << std::fixed << length;
            length_str << unit.get_abbreviation();
            Inkscape::XML::Node *rstring = NULL;
            if (!elemref) {
                rstring = xml_doc->createTextNode(length_str.str().c_str());
                rtspan->addChild(rstring, NULL);
                Inkscape::GC::release(rstring);
            } else {
                rstring = rtspan->firstChild();
                rstring->setContent(length_str.str().c_str());
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
            SP_ITEM(text_obj)->transform = affine * i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(desktop->doc()->getRoot()));
            text_obj->updateRepr();
        }
    }
}

void LPEMeasureLine::doOnRemove (SPLPEItem const* lpeitem)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
        Inkscape::URI SVGElem_uri(((Glib::ustring)this->getRepr()->attribute("id") + (Glib::ustring)"_text").c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *rtspan = NULL;
        if (elemref = SVGElemRef->getObject()) {
            elemref->deleteObject();
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
    vbox->set_spacing(2);

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
    Gtk::Button *save_default = Gtk::manage(new Gtk::Button(Glib::ustring(_("Save '*' as default"))));
    save_default->signal_clicked().connect(sigc::mem_fun(*this, &LPEMeasureLine::saveDefault));
    button1->pack_start(*save_default, true, true, 2);
    vbox->pack_start(*button1, true, true, 2);
    return dynamic_cast<Gtk::Widget *>(vbox);
}

Geom::PathVector
LPEMeasureLine::doEffect_path(Geom::PathVector const &path_in)
{
    return path_in;
}

void
LPEMeasureLine::saveDefault()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/live_effects/measure-line/fontbutton", (Glib::ustring)fontbutton.param_getSVGValue());
    prefs->setDouble("/live_effects/measure-line/scale", scale);
    prefs->setInt("/live_effects/measure-line/precision", precision);
    prefs->setDouble("/live_effects/measure-line/offset_right_left", offset_right_left);
    prefs->setDouble("/live_effects/measure-line/offset_top_bottom", offset_top_bottom);
    prefs->setString("/live_effects/measure-line/unit", (Glib::ustring)unit.get_abbreviation());
    prefs->setBool("/live_effects/measure-line/reverse", reverse);
    prefs->setBool("/live_effects/measure-line/color_as_line", color_as_line);
    prefs->setBool("/live_effects/measure-line/scale_insensitive", scale_insensitive);
    prefs->setBool("/live_effects/measure-line/local_locale", local_locale);
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
