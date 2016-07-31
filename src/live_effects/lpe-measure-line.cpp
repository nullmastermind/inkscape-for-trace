/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Some code and ideas migrated from dimensioning.py by
 * Johannes B. Rutzmoser, johannes.rutzmoser (at) googlemail (dot) com
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
#include "sp-defs.h"
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
    position(_("Positon*"), _("Positon"), "position", &wr, this, 5),
    text_distance(_("Text distance*"), _("Text distance"), "text_distance", &wr, this, 12),
    helpline_distance(_("Helpline distance*"), _("Helpline distance"), "helpline_distance", &wr, this, 0.0),
    helpline_overlap(_("Helpline overlap*"), _("Helpline overlap"), "helpline_overlap", &wr, this, 2.0),
    unit(_("Unit*"), _("Unit"), "unit", &wr, this, "px"),
    format(_("Format*"), _("Format the number ex:measure+ +unit"), "format", &wr, this, "measure+unit"),
    arrows_outside(_("Arrows outside"), _("Arrows outside"), "arrows_outside", &wr, this, false),
    flip_side(_("Flip side*"), _("Flip side"), "flip_side", &wr, this, false),
    scale_insensitive(_("Scale insensitive*"), _("Scale insensitive to transforms in element, parents..."), "scale_insensitive", &wr, this, true),
    local_locale(_("Local Number Format*"), _("Local number format"), "local_locale", &wr, this, true),
    line_group_05(_("Line Group 0.5*"), _("Line Group 0.5, from 0.7"), "line_group_05", &wr, this, true),
    rotate_anotation(_("Rotate Anotation*"), _("Rotate Anotation"), "rotate_anotation", &wr, this, true)
{
    registerParameter(&fontbutton);
    registerParameter(&orientation);
    registerParameter(&curve_linked);
    registerParameter(&scale);
    registerParameter(&precision);
    registerParameter(&position);
    registerParameter(&text_distance);
    registerParameter(&helpline_distance);
    registerParameter(&helpline_overlap);
    registerParameter(&unit);
    registerParameter(&format);
    registerParameter(&arrows_outside);
    registerParameter(&flip_side);
    registerParameter(&scale_insensitive);
    registerParameter(&local_locale);
    registerParameter(&line_group_05);
    registerParameter(&rotate_anotation);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    fontbutton.param_update_default(prefs->getString("/live_effects/measure-line/fontbutton"));
    scale.param_update_default(prefs->getDouble("/live_effects/measure-line/scale", 1.0));
    precision.param_update_default(prefs->getInt("/live_effects/measure-line/precision", 2));
    position.param_update_default(prefs->getDouble("/live_effects/measure-line/position", 10.0));
    text_distance.param_update_default(prefs->getDouble("/live_effects/measure-line/text_distance", 5.0));
    helpline_distance.param_update_default(prefs->getDouble("/live_effects/measure-line/helpline_distance", 0.0));
    helpline_overlap.param_update_default(prefs->getDouble("/live_effects/measure-line/helpline_overlap", 0.0));
    unit.param_update_default(prefs->getString("/live_effects/measure-line/unit"));
    format.param_update_default(prefs->getString("/live_effects/measure-line/format"));
    flip_side.param_update_default(prefs->getBool("/live_effects/measure-line/flip_side"));
    scale_insensitive.param_update_default(prefs->getBool("/live_effects/measure-line/scale_insensitive"));
    local_locale.param_update_default(prefs->getBool("/live_effects/measure-line/local_locale"));
    line_group_05.param_update_default(prefs->getBool("/live_effects/measure-line/line_group_05"));
    rotate_anotation.param_update_default(prefs->getBool("/live_effects/measure-line/rotate_anotation"));
    format.param_hide_canvas_text();
    precision.param_set_range(0, 100);
    precision.param_set_increments(1, 1);
    precision.param_set_digits(0);
    precision.param_make_integer(true);
    curve_linked.param_set_range(0, 999);
    curve_linked.param_set_increments(1, 1);
    curve_linked.param_set_digits(0);
    curve_linked.param_make_integer(true);
    precision.param_make_integer(true);
    position.param_set_range(-999999.0, 999999.0);
    position.param_set_increments(1, 1);
    position.param_set_digits(2);
    text_distance.param_set_range(-999999.0, 999999.0);
    text_distance.param_set_increments(1, 1);
    text_distance.param_set_digits(2);
    helpline_distance.param_set_range(-999999.0, 999999.0);
    helpline_distance.param_set_increments(1, 1);
    helpline_distance.param_set_digits(2);
    helpline_overlap.param_set_range(-999999.0, 999999.0);
    helpline_overlap.param_set_increments(1, 1);
    helpline_overlap.param_set_digits(2);
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
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" +  (Glib::ustring)"text-on-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
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
LPEMeasureLine::createArrowMarker(Glib::ustring mode)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + mode).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *arrow = NULL;
        if (!(elemref = SVGElemRef->getObject())) {
            arrow = xml_doc->createElement("svg:marker");
            arrow->setAttribute("id", mode.c_str());
            arrow->setAttribute("inkscape:stockid", mode.c_str());
            arrow->setAttribute("orient", "auto");
            arrow->setAttribute("refX", "0.0");
            arrow->setAttribute("refY", "0.0");
            arrow->setAttribute("style", "overflow:visible");
            /* Create <path> */
            Inkscape::XML::Node *arrow_path = xml_doc->createElement("svg:path");
            if (mode == (Glib::ustring)"ArrowDIN-start") {
                arrow_path->setAttribute("d", "M -8,0 8,-2.11 8,2.11 z");
            } else if (mode == (Glib::ustring)"ArrowDIN-end") {
                arrow_path->setAttribute("d", "M 8,0 -8,2.11 -8,-2.11 z");
            } else if (mode == (Glib::ustring)"ArrowDINout-start") {
                arrow_path->setAttribute("d", "M 0,0 -16,2.11 -16,0.5 -26,0.5 -26,-0.5 -16,-0.5 -16,-2.11 z");
            } else {
                arrow_path->setAttribute("d", "M 0,0 16,2.11 16,0.5 26,0.5 26,-0.5 16,-0.5 16,-2.11 z");
            }
            arrow_path->setAttribute("id", (mode + (Glib::ustring)"_path").c_str());
            arrow_path->setAttribute("style", "fill:#000000;stroke:none");
            arrow->addChild(arrow_path, NULL);
            Inkscape::GC::release(arrow_path);
            SPObject * arrow_obj = SP_OBJECT(desktop->getDocument()->getDefs()->appendChildRepr(arrow));
            Inkscape::GC::release(arrow);
        }
    }
}

void
LPEMeasureLine::createTextLabel(Geom::Point pos, double length, Geom::Coord angle, double fontsize, bool remove)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::XML::Node *rtext = NULL;
        double doc_w = desktop->getDocument()->getRoot()->width.value;
        Geom::Scale scale = desktop->getDocument()->getDocumentScale();
        SPNamedView *nv = desktop->getNamedView();
        Glib::ustring display_unit = nv->display_units->abbr;
        if (display_unit.empty()) {
            display_unit = "px";
        }
        //only check constrain viewbox on X
        doc_scale = Inkscape::Util::Quantity::convert( scale[Geom::X], "px", nv->display_units );
        if( doc_scale > 0 ) {
            doc_scale= 1.0/doc_scale;
        } else {
            doc_scale = 1.0;
        }
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" +  (Glib::ustring)"text-on-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *rtspan = NULL;
        if (elemref = SVGElemRef->getObject()) {
            if (remove) {
                elemref->deleteObject();
                return;
            }
            rtext = elemref->getRepr();
            sp_repr_set_svg_double(rtext, "x", pos[Geom::X]);
            sp_repr_set_svg_double(rtext, "y", pos[Geom::Y]);
        } else {
            if (remove) {
                return;
            }
            rtext = xml_doc->createElement("svg:text");
            rtext->setAttribute("xml:space", "preserve");
            rtext->setAttribute("id", ( (Glib::ustring)"text-on-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
            sp_repr_set_svg_double(rtext, "x", pos[Geom::X]);
            sp_repr_set_svg_double(rtext, "y", pos[Geom::Y]);
            rtspan = xml_doc->createElement("svg:tspan");
            rtspan->setAttribute("sodipodi:role", "line");
        }
        gchar * transform;
        if (rotate_anotation) {
            Geom::Affine affine = Geom::Affine(Geom::Translate(pos).inverse());
            angle = std::fmod(angle, 2*M_PI);
            if (angle < 0) angle += 2*M_PI;
            if (angle >= rad_from_deg(90) && angle < rad_from_deg(270)) {
                angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
                if (angle < 0) angle += 2*M_PI;
            }
            affine *= Geom::Rotate(angle);
            affine *= Geom::Translate(pos);
            transform = sp_svg_transform_write(affine);
        } else {
            transform = NULL;
        }
        rtext->setAttribute("transform", transform);
        SPCSSAttr *css = sp_repr_css_attr_new();
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
        length = Inkscape::Util::Quantity::convert(length / doc_scale, display_unit.c_str(), unit.get_abbreviation());
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
        }
    }
}

void
LPEMeasureLine::createLine(Geom::Point start,Geom::Point end, Glib::ustring id, bool main, bool remove)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + id).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *line = NULL;
        if (!main) {
            Geom::Ray ray(start, end);
            Geom::Coord angle = ray.angle();
            start = start + Point::polar(angle, helpline_distance );
            end = end + Point::polar(angle, helpline_overlap );
        }
        Geom::Path line_path;
        line_path.start(start);
        line_path.appendNew<Geom::LineSegment>(end);
        Geom::PathVector line_pathv;
        line_pathv.push_back(line_path);
        gchar * line_str = sp_svg_write_path( line_pathv );
        line_pathv.clear();
        if (elemref = SVGElemRef->getObject()) {
            if (remove) {
                elemref->deleteObject();
                return;
            }
            line = elemref->getRepr();
            line->setAttribute("d" , line_str);
        } else {
            if (remove) {
                return;
            }
            line = xml_doc->createElement("svg:path");
            line->setAttribute("id", id.c_str());
            line->setAttribute("d" , line_str);
        }
        Glib::ustring style = (Glib::ustring)"stroke:#000000;fill:none;";
        if (main) {
            line->setAttribute("inkscape:label", "dinline");
            if (arrows_outside) {
                style = style + (Glib::ustring)"marker-start:url(#ArrowDINout-start);marker-end:url(#ArrowDINout-end);";
            } else {
                style = style + (Glib::ustring)"marker-start:url(#ArrowDIN-start);marker-end:url(#ArrowDIN-end);";
            }
        } else {
            line->setAttribute("inkscape:label", "dinhelpline");
        }
        std::stringstream stroke_w;
        stroke_w.imbue(std::locale::classic());
        if (line_group_05) {
            double stroke_width = Inkscape::Util::Quantity::convert(0.25 / doc_scale, "mm", display_unit.c_str());
            stroke_w <<  stroke_width;
            style = style + (Glib::ustring)"stroke-width:" + (Glib::ustring)stroke_w.str();
        } else {
            double stroke_width = Inkscape::Util::Quantity::convert(0.35 / doc_scale, "mm", display_unit.c_str());
            stroke_w <<  stroke_width;
            style = style + (Glib::ustring)"stroke-width:" + (Glib::ustring)stroke_w.str();
        }
        line->setAttribute("style", style.c_str());
        SPObject * line_obj = NULL;
        if (!elemref) {
            line_obj = SP_OBJECT(desktop->currentLayer()->appendChildRepr(line));
            Inkscape::GC::release(line);
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
        if (arrows_outside) {
            createArrowMarker((Glib::ustring)"ArrowDINout-start");
            createArrowMarker((Glib::ustring)"ArrowDINout-end");
        } else {
            createArrowMarker((Glib::ustring)"ArrowDIN-start");
            createArrowMarker((Glib::ustring)"ArrowDIN-end");
        }
        if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
            size_t ncurves = pathvector.curveCount();
            curve_linked.param_set_range(0, ncurves);
            Geom::Point start = pathvector.initialPoint();
            Geom::Point end =  pathvector.finalPoint();
            if (curve_linked) { //!0 
                start = pathvector.pointAt(curve_linked -1);
                end = pathvector.pointAt(curve_linked);
            }
            Geom::Point hstart = start;
            Geom::Point hend = end;
            bool remove = false;
            if (Geom::are_near(hstart, hend)) {
                remove = true;
            }
            if (orientation == OM_VERTICAL) {
                Coord xpos = std::max(hstart[Geom::X],hend[Geom::X]);
                if (flip_side) {
                    xpos = std::min(hstart[Geom::X],hend[Geom::X]);
                }
                hstart[Geom::X] = xpos;
                hend[Geom::X] = xpos;
                if (hstart[Geom::Y] > hend[Geom::Y]) {
                    swap(hstart,hend);
                    swap(start,end);
                }
                if (Geom::are_near(hstart[Geom::Y], hend[Geom::Y])) {
                    remove = true;
                }
            }
            if (orientation == OM_HORIZONTAL) {
                Coord ypos = std::max(hstart[Geom::Y],hend[Geom::Y]);
                if (flip_side) {
                    ypos = std::min(hstart[Geom::Y],hend[Geom::Y]);
                }
                hstart[Geom::Y] = ypos;
                hend[Geom::Y] = ypos;
                if (hstart[Geom::X] < hend[Geom::X]) {
                    swap(hstart,hend);
                    swap(start,end);
                }
                if (Geom::are_near(hstart[Geom::X], hend[Geom::X])) {
                    remove = true;
                }
            }
            double length = Geom::distance(hstart,hend)  * scale;
            Geom::Point pos = Geom::middle_point(hstart,hend);
            Geom::Ray ray(hstart,hend);
            Geom::Coord angle = ray.angle();
            if (flip_side) {
                angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
                if (angle < 0) angle += 2*M_PI;
            }
            //We get the font size to offset the text to the middle
            Pango::FontDescription fontdesc((Glib::ustring)fontbutton.param_getSVGValue());
            double fontsize = fontdesc.get_size()/Pango::SCALE;
            fontsize *= desktop->doc()->getRoot()->c2p.inverse().expansionX();
            Geom::Coord angle_cross = std::fmod(angle + rad_from_deg(90), 2*M_PI);
            if (angle_cross < 0) angle_cross += 2*M_PI;
            angle = std::fmod(angle, 2*M_PI);
            if (angle < 0) angle += 2*M_PI;
            if (angle >= rad_from_deg(90) && angle < rad_from_deg(270)) {
                pos = pos - Point::polar(angle_cross, (position - text_distance) + fontsize/2.0);
            } else {
                pos = pos - Point::polar(angle_cross, (position + text_distance) - fontsize/2.0);
            }
            if (!scale_insensitive) {
                Geom::Affine affinetransform = i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(desktop->doc()->getRoot()));
                length *= (affinetransform.expansionX() + affinetransform.expansionY()) / 2.0;
            }
            createTextLabel(pos, length, angle, fontsize, remove);
            //LINE
            double arrow_gap = 8 * Inkscape::Util::Quantity::convert(0.35 / doc_scale, "mm", display_unit.c_str());
            if (line_group_05) {
                arrow_gap = 8 * Inkscape::Util::Quantity::convert(0.25 / doc_scale, "mm", display_unit.c_str());
            }
            if (flip_side) {
                arrow_gap *= -1;
            }
            angle_cross = std::fmod(angle + rad_from_deg(90), 2*M_PI);
            if (angle_cross < 0) angle_cross += 2*M_PI;
            hstart = hstart - Point::polar(angle_cross, position);
            Glib::ustring id = (Glib::ustring)"infoline-on-start-" + (Glib::ustring)this->getRepr()->attribute("id");
            createLine(start, hstart, id, false, remove);
            hend = hend - Point::polar(angle_cross, position);
            id = (Glib::ustring)"infoline-on-end-" + (Glib::ustring)this->getRepr()->attribute("id");
            createLine(end, hend, id, false, remove);
            if (!arrows_outside) {
                hstart = hstart + Point::polar(angle, arrow_gap);
                hend = hend - Point::polar(angle, arrow_gap );
            }
            id = (Glib::ustring)"infoline-" + (Glib::ustring)this->getRepr()->attribute("id");
            createLine(hstart, hend, id, true, remove);
        }
    }
}

void LPEMeasureLine::doOnRemove (SPLPEItem const* lpeitem)
{
    if (SPDesktop *desktop = SP_ACTIVE_DESKTOP) {
        SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
        Inkscape::URI SVGElem_uri(( (Glib::ustring)"text-on-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        if (elemref = SVGElemRef->getObject()) {
            elemref->deleteObject();
        }
        Inkscape::URI SVGElem_uri2(( (Glib::ustring)"infoline-on-end-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
        SVGElemRef->attach(SVGElem_uri2);
        elemref = NULL;
        if (elemref = SVGElemRef->getObject()) {
            elemref->deleteObject();
        }
        Inkscape::URI SVGElem_uri3(( (Glib::ustring)"infoline-on-start-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
        SVGElemRef->attach(SVGElem_uri3);
        elemref = NULL;
        if (elemref = SVGElemRef->getObject()) {
            elemref->deleteObject();
        }
        Inkscape::URI SVGElem_uri4(( (Glib::ustring)"infoline-" + (Glib::ustring)this->getRepr()->attribute("id")).c_str());
        SVGElemRef->attach(SVGElem_uri4);
        elemref = NULL;
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
    prefs->setDouble("/live_effects/measure-line/position", position);
    prefs->setDouble("/live_effects/measure-line/text_distance", text_distance);
    prefs->setDouble("/live_effects/measure-line/helpline_distance", helpline_distance);
    prefs->setDouble("/live_effects/measure-line/helpline_overlap", helpline_overlap);
    prefs->setString("/live_effects/measure-line/unit", (Glib::ustring)unit.get_abbreviation());
    prefs->setString("/live_effects/measure-line/format", (Glib::ustring)format.param_getSVGValue());
    prefs->setBool("/live_effects/measure-line/flip_side", flip_side);
    prefs->setBool("/live_effects/measure-line/scale_insensitive", scale_insensitive);
    prefs->setBool("/live_effects/measure-line/local_locale", local_locale);
    prefs->setBool("/live_effects/measure-line/line_group_05", line_group_05);
    prefs->setBool("/live_effects/measure-line/rotate_anotation", rotate_anotation);
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
