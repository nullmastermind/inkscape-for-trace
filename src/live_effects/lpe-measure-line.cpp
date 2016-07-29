/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)

 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-measure-line.h"
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
    { OM_PARALLEL, N_("Parallel"), "parallel" },
    { OM_PARALLEL_VERTICAL, N_("Parallel and vertical,"), "parallel_vertical" },
    { OM_PARALLEL_HORIZONTAL, N_("Parallel and horizontal"), "parallel_horizontal" },
    { OM_VERTICAL_HORIZONTAL, N_("Vertical and horizontal"), "vertical_horizontal" },
    { OM_PARALLEL_VERTICAL_HORIZONTAL, N_("Parallel, vertical and horizontal"), "parallel_vertical_horizontal" }
};
static const Util::EnumDataConverter<OrientationMethod> OMConverter(OrientationMethodData, OM_END);

LPEMeasureLine::LPEMeasureLine(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    fontselector(_("Font Selector*"), _("Font Selector"), "fontselector", &wr, this, " "),
    orientation(_("Orientation"), _("Orientation method"), "orientation", OMConverter, &wr, this, OM_PARALLEL, false),
    origin(_("Optional Origin"), _("Optional origin"), "origin", &wr, this),
    curve_linked(_("Curve on optional origin"), _("Curve on optional origin, set 0 to start/end"), "curve_linked", &wr, this, 1),
    origin_offset(_("Optional origin offset*"), _("Optional origin offset"), "origin_offset", &wr, this, 5),
    scale(_("Scale*"), _("Scaling factor"), "scale", &wr, this, 1.0),
    precision(_("Number precision*"), _("Number precision"), "precision", &wr, this, 2),
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
    registerParameter(&fontselector);
    registerParameter(&orientation);
    registerParameter(&origin);
    registerParameter(&curve_linked);
    registerParameter(&origin_offset);
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
    fontselector.param_update_default(prefs->getString("/live_effects/measure-line/fontselector"));
    scale.param_update_default(prefs->getDouble("/live_effects/measure-line/scale", 1.0));
    precision.param_update_default(prefs->getInt("/live_effects/measure-line/precision", 2));
    offset_right_left.param_update_default(prefs->getDouble("/live_effects/measure-line/offset_right_left", 0.0));
    offset_top_bottom.param_update_default(prefs->getDouble("/live_effects/measure-line/offset_top_bottom", 5.0));
    origin_offset.param_update_default(prefs->getDouble("/live_effects/measure-line/origin_offset", 5.0));
    unit.param_update_default(prefs->getString("/live_effects/measure-line/unit"));
    reverse.param_update_default(prefs->getBool("/live_effects/measure-line/reverse"));
    color_as_line.param_update_default(prefs->getBool("/live_effects/measure-line/color_as_line"));
    scale_insensitive.param_update_default(prefs->getBool("/live_effects/measure-line/scale_insensitive"));
    local_locale.param_update_default(prefs->getBool("/live_effects/measure-line/local_locale"));
    precision.param_set_range(0, 100);
    precision.param_set_increments(1, 1);
    precision.param_set_digits(0);
    precision.param_make_integer(true);
    curve_linked.param_set_range(1, 999);
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
    origin_offset.param_set_range(-999999.0, 999999.0);
    origin_offset.param_set_increments(1, 1);
    origin_offset.param_set_digits(2);
    gap_start.param_set_range(-999999.0, 999999.0);
    gap_start.param_set_increments(1, 1);
    gap_start.param_set_digits(2);
    gap_end.param_set_range(-999999.0, 999999.0);
    gap_end.param_set_increments(1, 1);
    gap_end.param_set_digits(2);
    fontlister = Inkscape::FontLister::get_instance();
}

bool LPEMeasureLine::alerts_off = false;

LPEMeasureLine::~LPEMeasureLine() {}

void
LPEMeasureLine::doOnApply(SPLPEItem const* lpeitem)
{
    if (!SP_IS_SHAPE(lpeitem)) {
        g_warning("LPE measure line can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
        item->removeCurrentPathEffect(false);
    } else {
        if(!alerts_off) {
            char *msg = _("The \"measure line\" path effect could update original path on the object you are applying it to if link it to other path. If this is not what you want, click Cancel.");
            Gtk::MessageDialog dialog(msg, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL, true);
            gint response = dialog.run();
            alerts_off = true;
            if(response == GTK_RESPONSE_CANCEL) {
                SPLPEItem* item = const_cast<SPLPEItem*>(lpeitem);
                item->removeCurrentPathEffect(false);
                return;
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
            Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
            std::vector<OrientationMethod> orientations;
            std::vector<OrientationMethod> orientations_remove;
            if ( orientation == OM_HORIZONTAL ||
                 orientation == OM_PARALLEL_VERTICAL_HORIZONTAL ||
                 orientation == OM_PARALLEL_HORIZONTAL ||
                 orientation == OM_VERTICAL_HORIZONTAL)
            {                
                orientations.push_back(OM_HORIZONTAL);
            } else {
                orientations_remove.push_back(OM_HORIZONTAL);
            }
            if ( orientation == OM_VERTICAL ||
                 orientation == OM_PARALLEL_VERTICAL_HORIZONTAL ||
                 orientation == OM_PARALLEL_VERTICAL ||
                 orientation == OM_VERTICAL_HORIZONTAL)
            { 
                orientations.push_back(OM_VERTICAL);
            } else {
                orientations_remove.push_back(OM_VERTICAL);
            }
            if ( orientation == OM_PARALLEL ||
                 orientation == OM_PARALLEL_VERTICAL_HORIZONTAL ||
                 orientation == OM_PARALLEL_VERTICAL ||
                 orientation == OM_PARALLEL_HORIZONTAL)
            { 
                orientations.push_back(OM_PARALLEL);
            } else {
                orientations_remove.push_back(OM_PARALLEL);
            }
            for (std::vector<OrientationMethod>::const_iterator om_it = orientations.begin(); om_it != orientations.end(); ++om_it) {
                Geom::Point s = pathvector.initialPoint();
                Geom::Point e =  pathvector.finalPoint();
                Glib::ustring orientation_str;
                Inkscape::XML::Node *rtext = NULL;
                if (*om_it == OM_VERTICAL) {
                    orientation_str = "vertical";
                }
                if (*om_it == OM_HORIZONTAL) {
                    orientation_str = "horizontal";
                }
                if (*om_it == OM_PARALLEL) {
                    orientation_str = "parallel";
                }
                if (origin.linksToPath() && origin.getObject() && !origin.get_pathvector().empty()) {
                    pathvector = origin.get_pathvector();
                    size_t ncurves = pathvector.curveCount();
                    curve_linked.param_set_range(1, ncurves);
                    if(previous_ncurves != ncurves) {
                        this->upd_params = true;
                        previous_ncurves = ncurves;
                    }
                    s = pathvector.pointAt(curve_linked -1);
                    e = pathvector.pointAt(curve_linked);
                    if(*om_it == OM_VERTICAL) {
                        Coord xpos = std::max(s[Geom::X],e[Geom::X]);
                        if (reverse) {
                            xpos = std::min(s[Geom::X],e[Geom::X]);
                        }
                        s[Geom::X] = xpos;
                        e[Geom::X] = xpos;
                    }
                    if(*om_it == OM_HORIZONTAL) {
                        Coord ypos = std::max(s[Geom::Y],e[Geom::Y]);
                        if (reverse) {
                            ypos = std::min(s[Geom::Y],e[Geom::Y]);
                        }
                        s[Geom::Y] = ypos;
                        e[Geom::Y] = ypos;
                    }
                    Geom::Ray ray(s,e);
                    Geom::Coord angle = ray.angle();
                    if (reverse) {
                        angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
                        if (angle < 0) angle += 2*M_PI;
                    }
                    Geom::Coord angle_cross = std::fmod(angle + rad_from_deg(90), 2*M_PI);
                    if (angle_cross < 0) angle_cross += 2*M_PI;
                    s = s - Point::polar(angle_cross, origin_offset);
                    e = e - Point::polar(angle_cross, origin_offset);
                    Geom::Path path;
                    path.start( s );
                    path.appendNew<Geom::LineSegment>( e );
                    Geom::PathVector line_upd;
                    line_upd.push_back(path);
                    Inkscape::XML::Node *line = SP_OBJECT(sp_path)->getRepr();
                    gchar *str = sp_svg_write_path(line_upd);
                    line->setAttribute("inkscape:original-d", str);
                } else {
                    if(*om_it == OM_VERTICAL) {
                        Coord xpos = std::max(s[Geom::X],e[Geom::X]);
                        if (reverse) {
                            xpos = std::min(s[Geom::X],e[Geom::X]);
                        }
                        s[Geom::X] = xpos;
                        e[Geom::X] = xpos;
                    }
                    if(*om_it == OM_HORIZONTAL) {
                        Coord ypos = std::max(s[Geom::Y],e[Geom::Y]);
                        if (reverse) {
                            ypos = std::min(s[Geom::Y],e[Geom::Y]);
                        }
                        s[Geom::Y] = ypos;
                        e[Geom::Y] = ypos;
                    }
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
                Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + (Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_DINnumber_" + orientation_str).c_str());
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
                    rtext->setAttribute("id", ((Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_DINnumber_" + orientation_str).c_str());
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
                length_str.setf(std::ios::fixed, std::ios::floatfield);
                length_str.precision(precision);
                if (local_locale) {
                    length_str.imbue(std::locale(""));
                } else {
                    length_str.imbue(std::locale::classic());
                }
                length_str << length << unit.get_abbreviation();
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
            for (std::vector<OrientationMethod>::const_iterator omdel_it = orientations_remove.begin(); omdel_it != orientations_remove.end(); ++omdel_it) {
                Glib::ustring orientation_str;
                if (*omdel_it == OM_VERTICAL) {
                    orientation_str = "vertical";
                }
                if (*omdel_it == OM_HORIZONTAL) {
                    orientation_str = "horizontal";
                }
                if (*omdel_it == OM_PARALLEL) {
                    orientation_str = "parallel";
                }
                Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + (Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_DINnumber_" + orientation_str).c_str());
                Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
                SVGElemRef->attach(SVGElem_uri);
                SPObject *elemref = NULL;
                if (elemref = SVGElemRef->getObject()) {
                    elemref->deleteObject();
                }
            }
        }
    }
}

void LPEMeasureLine::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
//    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
//        if (horizontal_text) {
//            SPObject * text_obj = desktop->currentLayer()->get_child_by_repr(horizontal_text);
//            if (text_obj) {
//                text_obj->deleteObject();
//            }
//        }
//        if (vertical_text) {
//            SPObject * text_obj = desktop->currentLayer()->get_child_by_repr(vertical_text);
//            if (text_obj) {
//                text_obj->deleteObject();
//            }
//        }
//        if (parallel_text) {
//            SPObject * text_obj = desktop->currentLayer()->get_child_by_repr(parallel_text);
//            if (text_obj) {
//                text_obj->deleteObject();
//            }
//        }
//    }
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
    Geom::Path path;
    Geom::Point s = path_in.initialPoint();
    Geom::Point e =  path_in.finalPoint();
    Geom::Ray ray(s,e);
    Geom::Coord angle = ray.angle();
    if (reverse) {
        angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
        if (angle < 0) angle += 2*M_PI;
    }
    s = s - Point::polar(angle, gap_start);
    angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
    if (angle < 0) angle += 2*M_PI;
    e = e - Point::polar(angle, gap_end);
    if(orientation == OM_VERTICAL) {
        Coord xpos = std::max(s[Geom::X],e[Geom::X]);
        if (reverse) {
            xpos = std::min(s[Geom::X],e[Geom::X]);
        }
        s[Geom::X] = xpos;
        e[Geom::X] = xpos;
    }
    if(orientation == OM_HORIZONTAL) {
        Coord ypos = std::max(s[Geom::Y],e[Geom::Y]);
        if (reverse) {
            ypos = std::min(s[Geom::Y],e[Geom::Y]);
        }
        s[Geom::Y] = ypos;
        e[Geom::Y] = ypos;
    }
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
    prefs->setDouble("/live_effects/measure-line/offset_right_left", offset_right_left);
    prefs->setDouble("/live_effects/measure-line/offset_top_bottom", offset_top_bottom);
    prefs->setDouble("/live_effects/measure-line/origin_offset", origin_offset);
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
