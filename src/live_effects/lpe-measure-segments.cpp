/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Some code and ideas migrated from dimensioning.py by
 * Johannes B. Rutzmoser, johannes.rutzmoser (at) googlemail (dot) com
 * https://github.com/Rutzmoser/inkscape_dimensioning
 * Copyright (C) 2014 Author(s)

 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "live_effects/lpe-measure-segments.h"
#include "live_effects/lpeobject.h"
#include <pangomm/fontdescription.h>
#include "ui/dialog/livepatheffect-editor.h"
#include <libnrtype/font-lister.h>
#include "inkscape.h"
#include "xml/node.h"
#include "xml/sp-css-attr.h"
#include "preferences.h"
#include "util/units.h"
#include "svg/svg-length.h"
#include "svg/svg-color.h"
#include "svg/stringstream.h"
#include "svg/svg.h"
#include "display/curve.h"
#include "helper/geom.h"
#include "2geom/affine.h"
#include "path-chemistry.h"
#include "document.h"
#include "document-undo.h"
#include <iomanip>
#include <cmath>

#include "object/sp-root.h"
#include "object/sp-defs.h"
#include "object/sp-item.h"
#include "object/sp-shape.h"
#include "object/sp-path.h"
#include "object/sp-star.h"
#include "object/sp-spiral.h"

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

LPEMeasureSegments::LPEMeasureSegments(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    unit(_("Unit"), _("Unit"), "unit", &wr, this, "mm"),
    orientation(_("Orientation"), _("Orientation method"), "orientation", OMConverter, &wr, this, OM_PARALLEL, false),
    coloropacity(_("Color and opacity"), _("Set color and opacity of the measurements"), "coloropacity", &wr, this, 0x000000ff),
    fontbutton(_("Font"), _("Font Selector"), "fontbutton", &wr, this),
    precision(_("Precision"), _("Precision"), "precision", &wr, this, 2),
    fix_overlaps(_("Fix overlaps °"), _("Min angle where overlaps are fixed, 180° no fix"), "fix_overlaps", &wr, this, 0),
    position(_("Position"), _("Position"), "position", &wr, this, 5),
    text_top_bottom(_("Text top/bottom"), _("Text top/bottom"), "text_top_bottom", &wr, this, 0),
    helpline_distance(_("Helpline distance"), _("Helpline distance"), "helpline_distance", &wr, this, 0.0),
    helpline_overlap(_("Helpline overlap"), _("Helpline overlap"), "helpline_overlap", &wr, this, 2.0),
    line_width(_("Line width"), _("Line width. DIM line group standard are 0.25 or 0.35"), "line_width", &wr, this, 0.25),
    scale(_("Scale"), _("Scaling factor"), "scale", &wr, this, 1.0),
    
    format(_("Format"), _("Format the number ex:{measure} {unit}, return to save"), "format", &wr, this,"{measure}{unit}"),
    blacklist(_("Blacklist"), _("Optional segment index that exclude measure, comma limited, you can add more LPE like this to fill the holes"), "blacklist", &wr, this,""),
    whitelist(_("Inverse blacklist"), _("Blacklist as whitelist"), "whitelist", &wr, this, false),
    arrows_outside(_("Arrows outside"), _("Arrows outside"), "arrows_outside", &wr, this, false),
    flip_side(_("Flip side"), _("Flip side"), "flip_side", &wr, this, false),
    scale_sensitive(_("Scale sensitive"), _("Costrained scale sensitive to transformed containers"), "scale_sensitive", &wr, this, true),
    local_locale(_("Local Number Format"), _("Local number format"), "local_locale", &wr, this, true),
    rotate_anotation(_("Rotate Annotation"), _("Rotate Annotation"), "rotate_anotation", &wr, this, true),
    hide_back(_("Hide if label over"), _("Hide DIN line if label over"), "hide_back", &wr, this, true),
    message(_("Info Box"), _("Important messages"), "message", &wr, this, _("Use <b>\"Style Dialog\"</b> to more styling. Each measure element has extra selectors. Use !important to override defaults..."))
{
    //set to true the parameters you want to be changed his default values
    registerParameter(&unit);
    registerParameter(&orientation);
    registerParameter(&coloropacity);
    registerParameter(&fontbutton);
    registerParameter(&precision);
    registerParameter(&fix_overlaps);
    registerParameter(&position);
    registerParameter(&text_top_bottom);
    registerParameter(&helpline_distance);
    registerParameter(&helpline_overlap);
    registerParameter(&line_width);
    registerParameter(&scale);
    registerParameter(&format);
    registerParameter(&blacklist);
    registerParameter(&whitelist);
    registerParameter(&arrows_outside);
    registerParameter(&flip_side);
    registerParameter(&scale_sensitive);
    registerParameter(&local_locale);
    registerParameter(&rotate_anotation);
    registerParameter(&hide_back);
    registerParameter(&message);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Glib::ustring format_value = prefs->getString("/live_effects/measure-line/format");
    if(format_value.empty()){
        format_value = "{measure}{unit}";
    }
    format.param_update_default(format_value.c_str());

    format.param_hide_canvas_text();
    blacklist.param_hide_canvas_text();
    precision.param_set_range(0, 100);
    precision.param_set_increments(1, 1);
    precision.param_set_digits(0);
    precision.param_make_integer(true);
    fix_overlaps.param_set_range(0, 180);
    fix_overlaps.param_set_increments(1, 1);
    fix_overlaps.param_set_digits(0);
    fix_overlaps.param_make_integer(true);
    position.param_set_range(-999999.0, 999999.0);
    position.param_set_increments(1, 1);
    position.param_set_digits(2);
    scale.param_set_range(-999999.0, 999999.0);
    scale.param_set_increments(1, 1);
    scale.param_set_digits(4);
    text_top_bottom.param_set_range(-999999.0, 999999.0);
    text_top_bottom.param_set_increments(1, 1);
    text_top_bottom.param_set_digits(2);
    line_width.param_set_range(-999999.0, 999999.0);
    line_width.param_set_increments(1, 1);
    line_width.param_set_digits(2);
    helpline_distance.param_set_range(-999999.0, 999999.0);
    helpline_distance.param_set_increments(1, 1);
    helpline_distance.param_set_digits(2);
    helpline_overlap.param_set_range(-999999.0, 999999.0);
    helpline_overlap.param_set_increments(1, 1);
    helpline_overlap.param_set_digits(2);
    star_ellipse_fix = Geom::identity();
    message.param_set_min_height(95);
}

LPEMeasureSegments::~LPEMeasureSegments() {}

void
LPEMeasureSegments::createArrowMarker(const char * mode)
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    Glib::ustring style;
    gchar c[32];
    unsigned const rgb24 = coloropacity.get_value() >> 8;
    sprintf(c, "#%06x", rgb24);
    style = Glib::ustring("fill:") + Glib::ustring(c);
    Inkscape::SVGOStringStream os;
    os << SP_RGBA32_A_F(coloropacity.get_value());
    style = style + Glib::ustring(";fill-opacity:") + Glib::ustring(os.str());
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    SPObject *elemref = NULL;
    Inkscape::XML::Node *arrow = NULL;
    if ((elemref = document->getObjectById(mode))) {
        Inkscape::XML::Node *arrow= elemref->getRepr();
        if (arrow) {
            arrow->setAttribute("sodipodi:insensitive", "true");
            arrow->setAttribute("transform", NULL);
            Inkscape::XML::Node *arrow_data = arrow->firstChild();
            if (arrow_data) {
                arrow_data->setAttribute("transform", NULL);
                arrow_data->setAttribute("style", style.c_str());
            }
        }
    } else {
        arrow = xml_doc->createElement("svg:marker");
        arrow->setAttribute("id", mode);
        arrow->setAttribute("class", (Glib::ustring(sp_lpe_item->getId()) + Glib::ustring(" ") + Glib::ustring(this->lpeobj->getId()) + Glib::ustring(" measure-arrows-marker")).c_str());
        arrow->setAttribute("inkscape:stockid", mode);
        arrow->setAttribute("orient", "auto");
        arrow->setAttribute("refX", "0.0");
        arrow->setAttribute("refY", "0.0");

        arrow->setAttribute("sodipodi:insensitive", "true");
        /* Create <path> */
        Inkscape::XML::Node *arrow_path = xml_doc->createElement("svg:path");
        if (std::strcmp(mode, "ArrowDIN-start") == 0) {
            arrow_path->setAttribute("d", "M -8,0 8,-2.11 8,2.11 z");
        } else if (std::strcmp(mode, "ArrowDIN-end") == 0) {
            arrow_path->setAttribute("d", "M 8,0 -8,2.11 -8,-2.11 z");
        } else if (std::strcmp(mode, "ArrowDINout-start") == 0) {
            arrow_path->setAttribute("d", "M 0,0 -16,2.11 -16,0.5 -26,0.5 -26,-0.5 -16,-0.5 -16,-2.11 z");
        } else {
            arrow_path->setAttribute("d", "M 0,0 16,-2.11 16,-0.5 26,-0.5 26,0.5 16,0.5 16,2.11 z");
        }
        arrow_path->setAttribute("class", (Glib::ustring(sp_lpe_item->getId()) + Glib::ustring(" ") + Glib::ustring(this->lpeobj->getId()) + Glib::ustring(" measure-arrows")).c_str());
        arrow_path->setAttribute("id", Glib::ustring(mode).append("_path").c_str());
        arrow_path->setAttribute("style", style.c_str());
        arrow->addChild(arrow_path, NULL);
        Inkscape::GC::release(arrow_path);
        elemref = SP_OBJECT(document->getDefs()->appendChildRepr(arrow));
        Inkscape::GC::release(arrow);
    }
    items.push_back(mode);
}

void
LPEMeasureSegments::createTextLabel(Geom::Point pos, size_t counter, double length, Geom::Coord angle, bool remove, bool valid)
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *rtext = NULL;
    double doc_w = document->getRoot()->width.value;
    Geom::Scale scale = document->getDocumentScale();
    SPNamedView *nv = sp_document_namedview(document, NULL);
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
    const char * id = g_strdup(Glib::ustring("text-on-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str());
    SPObject *elemref = NULL;
    Inkscape::XML::Node *rtspan = NULL;
    if ((elemref = document->getObjectById(id))) {
        if (remove) {
            elemref->deleteObject();
            return;
        }
        rtext = elemref->getRepr();
        sp_repr_set_svg_double(rtext, "x", pos[Geom::X]);
        sp_repr_set_svg_double(rtext, "y", pos[Geom::Y]);
        rtext->setAttribute("sodipodi:insensitive", "true");
        rtext->setAttribute("transform", NULL);
    } else {
        if (remove) {
            return;
        }
        rtext = xml_doc->createElement("svg:text");
        rtext->setAttribute("xml:space", "preserve");
        rtext->setAttribute("id", id);
        rtext->setAttribute("class", (Glib::ustring(sp_lpe_item->getId()) + Glib::ustring(" ") + Glib::ustring(this->lpeobj->getId()) + Glib::ustring(" measure-labels")).c_str());
        rtext->setAttribute("sodipodi:insensitive", "true");
        sp_repr_set_svg_double(rtext, "x", pos[Geom::X]);
        sp_repr_set_svg_double(rtext, "y", pos[Geom::Y]);
        rtspan = xml_doc->createElement("svg:tspan");
        rtspan->setAttribute("sodipodi:role", "line");
    }
    gchar * transform;
    Geom::Affine affine = Geom::Affine(Geom::Translate(pos).inverse());
    angle = std::fmod(angle, 2*M_PI);
    if (angle < 0) angle += 2*M_PI;
    if (angle >= rad_from_deg(90) && angle < rad_from_deg(270)) {
        angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
        if (angle < 0) angle += 2*M_PI;
    }
    affine *= Geom::Rotate(angle);
    affine *= Geom::Translate(pos);
    if (rotate_anotation) {
        transform = sp_svg_transform_write(affine);
    } else {
        transform = NULL;
    }
    rtext->setAttribute("transform", transform);
    g_free(transform);
    SPCSSAttr *css = sp_repr_css_attr_new();
    Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
    fontlister->fill_css(css, Glib::ustring(fontbutton.param_getSVGValue()));
    std::stringstream font_size;
    font_size.imbue(std::locale::classic());
    font_size <<  fontsize << "pt";

    gchar c[32];
    unsigned const rgb24 = coloropacity.get_value() >> 8;
    sprintf(c, "#%06x", rgb24);
    sp_repr_css_set_property (css, "fill",c);
    Inkscape::SVGOStringStream os;
    os << SP_RGBA32_A_F(coloropacity.get_value());
    sp_repr_css_set_property (css, "fill-opacity",os.str().c_str());
    if (!rtspan) {
        rtspan = rtext->firstChild();
    }
    sp_repr_css_set_property (css, "font-size",font_size.str().c_str());
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    rtext->setAttribute("style", css_str.c_str());
    rtspan->setAttribute("style", NULL);
    rtspan->setAttribute("transform", NULL);
    sp_repr_css_attr_unref (css);
    if (!elemref) {
        rtext->addChild(rtspan, NULL);
        Inkscape::GC::release(rtspan);
    }
    length = Inkscape::Util::Quantity::convert(length / doc_scale, display_unit.c_str(), unit.get_abbreviation());
    char *oldlocale = g_strdup (setlocale(LC_NUMERIC, NULL));
    if (local_locale) {
        setlocale (LC_NUMERIC, "");
    } else {
        setlocale (LC_NUMERIC, "C");
    }
    gchar length_str[64];
    g_snprintf(length_str, 64, "%.*f", (int)precision, length);
    setlocale (LC_NUMERIC, oldlocale);
    g_free (oldlocale);
    Glib::ustring label_value(format.param_getSVGValue());
    size_t s = label_value.find(Glib::ustring("{measure}"),0);
    if(s < label_value.length()) {
        label_value.replace(s,s+9,length_str);
    }
    s = label_value.find(Glib::ustring("{unit}"),0);
    if(s < label_value.length()) {
        label_value.replace(s,s+6,unit.get_abbreviation());
    }
    if ( !valid ) {
        label_value = Glib::ustring(_("Non Uniform Scale"));
    }
    Inkscape::XML::Node *rstring = NULL;
    if (!elemref) {
        rstring = xml_doc->createTextNode(label_value.c_str());
        rtspan->addChild(rstring, NULL);
        Inkscape::GC::release(rstring);
    } else {
        rstring = rtspan->firstChild();
        rstring->setContent(label_value.c_str());
    }
    if (!elemref) {
        elemref = sp_lpe_item->parent->appendChildRepr(rtext);
        Inkscape::GC::release(rtext);
    } else if (elemref->parent != sp_lpe_item->parent) {
        Inkscape::XML::Node *old_repr = elemref->getRepr();
        Inkscape::XML::Node *copy = old_repr->duplicate(xml_doc);
        SPObject * elemref_copy = sp_lpe_item->parent->appendChildRepr(copy);
        Inkscape::GC::release(copy);
        elemref->deleteObject();
        copy->setAttribute("id", id);
        elemref = elemref_copy;
    }
    items.push_back(id);
    Geom::OptRect bounds = SP_ITEM(elemref)->bounds(SPItem::GEOMETRIC_BBOX);
    if (bounds) {
        anotation_width = bounds->width() * 1.15;
    }
}

void
LPEMeasureSegments::createLine(Geom::Point start,Geom::Point end, const char * id, bool main, bool remove, bool arrows)
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    SPObject *elemref = NULL;
    Inkscape::XML::Node *line = NULL;
    if (!main) {
        Geom::Ray ray(start, end);
        Geom::Coord angle = ray.angle();
        start = start + Point::polar(angle, helpline_distance );
        end = end + Point::polar(angle, helpline_overlap );
    }
    Geom::PathVector line_pathv;

    if (main && 
        std::abs(text_top_bottom) < fontsize/1.5 && 
        hide_back)
    {
        double k = 0;
        if (flip_side) {
            k = (Geom::distance(start,end)/2.0) + arrow_gap - (anotation_width/2.0);
        } else {
            k = (Geom::distance(start,end)/2.0) - arrow_gap - (anotation_width/2.0);
        }
        if (Geom::distance(start,end) < anotation_width){
            if ((elemref = document->getObjectById(id))) {
                if (remove) {
                    elemref->deleteObject();
                }
                return;
            }
        }
        //k = std::max(k , arrow_gap -1);
        Geom::Ray ray(end, start);
        Geom::Coord angle = ray.angle();
        Geom::Path line_path(start);
        line_path.appendNew<Geom::LineSegment>(start - Point::polar(angle, k));
        line_pathv.push_back(line_path);
        line_path.clear();
        line_path.start(end + Point::polar(angle, k));
        line_path.appendNew<Geom::LineSegment>(end);
        line_pathv.push_back(line_path);
    } else {
        Geom::Path line_path(start);
        line_path.appendNew<Geom::LineSegment>(end);
        line_pathv.push_back(line_path);
    }
    if ((elemref = document->getObjectById(id))) {
        if (remove) {
            elemref->deleteObject();
            return;
        }
        line = elemref->getRepr();
       
        gchar * line_str = sp_svg_write_path( line_pathv );
        line->setAttribute("d" , line_str);
        line->setAttribute("transform", NULL);
        g_free(line_str);
    } else {
        if (remove) {
            return;
        }
        line = xml_doc->createElement("svg:path");
        line->setAttribute("id", id);
        if (main) {
            line->setAttribute("class", (Glib::ustring(sp_lpe_item->getId()) + Glib::ustring(" ") + Glib::ustring(this->lpeobj->getId()) + Glib::ustring(" measure-DIM-lines measure-lines")).c_str());
        } else {
            line->setAttribute("class", (Glib::ustring(sp_lpe_item->getId()) + Glib::ustring(" ") + Glib::ustring(this->lpeobj->getId()) + Glib::ustring(" measure-helper-lines measure-lines")).c_str());
        }
        gchar * line_str = sp_svg_write_path( line_pathv );
        line->setAttribute("d" , line_str);
        g_free(line_str);
    }
    line->setAttribute("sodipodi:insensitive", "true");
    line_pathv.clear();
        
    Glib::ustring style;
    if (main) {
        line->setAttribute("inkscape:label", "dinline");
        if (arrows_outside) {
            style = style + Glib::ustring("marker-start:url(#ArrowDINout-start);marker-end:url(#ArrowDINout-end);");
        } else {
            style = style + Glib::ustring("marker-start:url(#ArrowDIN-start);marker-end:url(#ArrowDIN-end);");
        }
    } else {
        line->setAttribute("inkscape:label", "dinhelpline");
    }
    std::stringstream stroke_w;
    stroke_w.imbue(std::locale::classic());
    double stroke_width = Inkscape::Util::Quantity::convert(line_width / doc_scale, "mm", display_unit.c_str());
    stroke_w <<  stroke_width;
    style = style + Glib::ustring("stroke-width:" + stroke_w.str());
    gchar c[32];
    unsigned const rgb24 = coloropacity.get_value() >> 8;
    sprintf(c, "#%06x", rgb24);
    style = style + Glib::ustring(";stroke:") + Glib::ustring(c);
    Inkscape::SVGOStringStream os;
    os << SP_RGBA32_A_F(coloropacity.get_value());
    style = style + Glib::ustring(";stroke-opacity:") + Glib::ustring(os.str());
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css, style.c_str());
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    line->setAttribute("style", css_str.c_str());
    if (!elemref) {
        elemref = sp_lpe_item->parent->appendChildRepr(line);
        Inkscape::GC::release(line);
    } else if (elemref->parent != sp_lpe_item->parent) {
        Inkscape::XML::Node *old_repr = elemref->getRepr();
        Inkscape::XML::Node *copy = old_repr->duplicate(xml_doc);
        SPObject * elemref_copy = sp_lpe_item->parent->appendChildRepr(copy);
        Inkscape::GC::release(copy);
        elemref->deleteObject();
        copy->setAttribute("id", id);
    }
    items.push_back(id);
}

void
LPEMeasureSegments::doOnApply(SPLPEItem const* lpeitem)
{
    if (!SP_IS_SHAPE(lpeitem)) {
        g_warning("LPE measure line can only be applied to shapes (not groups).");
        SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
        item->removeCurrentPathEffect(false);
        return;
    }
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    bool saved = DocumentUndo::getUndoSensitive(document);
    DocumentUndo::setUndoSensitive(document, false);
    Inkscape::XML::Node *styleNode = NULL;
    Inkscape::XML::Node* textNode = NULL;
    Inkscape::XML::Node *root = SP_ACTIVE_DOCUMENT->getReprRoot();
    for (unsigned i = 0; i < root->childCount(); ++i) {
        if (Glib::ustring(root->nthChild(i)->name()) == "svg:style") {

            styleNode = root->nthChild(i);

            for (unsigned j = 0; j < styleNode->childCount(); ++j) {
                if (styleNode->nthChild(j)->type() == Inkscape::XML::TEXT_NODE) {
                    textNode = styleNode->nthChild(j);
                }
            }

            if (textNode == NULL) {
                // Style element found but does not contain text node!
                std::cerr << "StyleDialog::_getStyleTextNode(): No text node!" << std::endl;
                textNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");
                styleNode->appendChild(textNode);
                Inkscape::GC::release(textNode);
            }
        }
    }

    if (styleNode == NULL) {
        // Style element not found, create one
        styleNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createElement("svg:style");
        textNode  = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");

        styleNode->appendChild(textNode);
        Inkscape::GC::release(textNode);

        root->addChild(styleNode, NULL);
        Inkscape::GC::release(styleNode);
    }
    Glib::ustring styleContent = Glib::ustring(textNode->content());
    if (styleContent.find(".measure-arrows\n{\n") == -1) {
        styleContent = styleContent + Glib::ustring("\n.measure-arrows") + Glib::ustring("\n{\n}");
        styleContent = styleContent + Glib::ustring("\n.measure-labels") + Glib::ustring("\n{\nline-height:125%;\nletter-spacing:0;\nword-spacing:0;\ntext-align:center;\ntext-anchor:middle;\nstroke:none;\n}");
        styleContent = styleContent + Glib::ustring("\n.measure-lines") + Glib::ustring("\n{\n}");
        textNode->setContent(styleContent.c_str());
    }
    DocumentUndo::setUndoSensitive(document, saved);
}

bool
LPEMeasureSegments::hasMeassure (size_t i)
{
    std::string listsegments(std::string(blacklist.param_getSVGValue()) + std::string(","));
    listsegments.erase(std::remove(listsegments.begin(), listsegments.end(), ' '), listsegments.end());
    size_t s = listsegments.find(std::to_string(i) + std::string(","),0);
    if(s < listsegments.length()) {
        if (whitelist) {
            return true;
        } else {
            return false;
        }
    } else {
        if (whitelist) {
            return false;
        } else {
            return true;
        }
    }
    return false;
}

double getAngle(Geom::Point p1, Geom::Point p2, Geom::Point p3, bool flip_side, double fix_overlaps){
    Geom::Ray ray_1(p2,p1);
    Geom::Ray ray_2(p3,p1);
    bool ccw_toggle = cross(p1 - p2, p3 - p2) < 0;
    double angle = angle_between(ray_1, ray_2, ccw_toggle);
    if (Geom::deg_from_rad(angle) < fix_overlaps ||
        Geom::deg_from_rad(angle) > 180 || 
        ((ccw_toggle && flip_side) || (!ccw_toggle && !flip_side)))
    {
        angle = 0;
    }
    return angle;
}

void
LPEMeasureSegments::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);

    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    Inkscape::XML::Node *root = splpeitem->document->getReprRoot();
    Inkscape::XML::Node *root_origin = document->getReprRoot();
    if (root_origin != root) {
        return;
    }

    SPShape *shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        SPCurve * c = NULL;

        SPPath *path = dynamic_cast<SPPath *>(shape);
        if (path) {
            c = path->get_original_curve();
        } else {
            c = shape->getCurve();
        }
        Geom::Point start_stored;
        Geom::Point end_stored; 
        Geom::Affine affinetransform = i2anc_affine(SP_OBJECT(lpeitem->parent), SP_OBJECT(document->getRoot()));
        Geom::PathVector pathvector =  pathv_to_linear_and_cubic_beziers(c->get_pathvector());
        c->unref();
        Geom::Affine writed_transform = Geom::identity();
        sp_svg_transform_read(splpeitem->getAttribute("transform"), &writed_transform );
        if (star_ellipse_fix != Geom::identity()) {
            pathvector *= star_ellipse_fix;
            star_ellipse_fix = Geom::identity();
        } else {
            pathvector *= writed_transform;
        }
        if ((Glib::ustring(format.param_getSVGValue()).empty())) {
            format.param_setValue(Glib::ustring("{measure}{unit}"));
        }
        size_t ncurves = pathvector.curveCount();
        items.clear();
        double start_angle_cross = 0;
        double end_angle_cross = 0;
        size_t counter = -1;
        for (size_t i = 0; i < pathvector.size(); i++) {
            for (size_t j = 0; j < pathvector[i].size(); j++) {
                counter++;
                if(hasMeassure(counter + 1)) {
                    Geom::Point prev = Geom::Point(0,0);
                    if (j == 0 && pathvector[i].closed()) {
                        prev = pathvector.pointAt(pathvector[i].size() - 1);
                    } else if (j != 0) {
                        prev = pathvector[i].pointAt(j - 1);
                    }
                    Geom::Point start = pathvector[i].pointAt(j);
                    Geom::Point end = pathvector[i].pointAt(j + 1);
                    Geom::Point next = Geom::Point(0,0);
                    if(pathvector[i].closed() && pathvector[i].size() == j+1){
                        end = pathvector[i].pointAt(0);
                        next = pathvector[i].pointAt(1);
                    } else if (pathvector[i].size() > j + 1) {
                        next = pathvector[i].pointAt(j+2);
                    }
                    const char * idstart = Glib::ustring("infoline-on-start-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str();
                    SPObject *elemref = NULL;
                    if ((elemref = document->getObjectById(idstart))) {
                        start_stored = *SP_PATH(elemref)->get_curve()->first_point();
                    }
                    const char * idend = Glib::ustring("infoline-on-end-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str();
                    elemref = NULL;
                    if ((elemref = document->getObjectById(idend))) {
                        end_stored = *SP_PATH(elemref)->get_curve()->first_point();
                    }
                    if (Geom::are_near(start, start_stored, 0.01) && 
                        Geom::are_near(end, end_stored, 0.01))
                    {
                        continue;
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
                    } else if (orientation == OM_HORIZONTAL) {
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
                    } else if (fix_overlaps != 180) {
                        start_angle_cross = getAngle( start, prev, end, flip_side, fix_overlaps);
                        if (prev == Geom::Point(0,0)) {
                            start_angle_cross = 0;
                        }
                        end_angle_cross = getAngle(end, start, next, flip_side, fix_overlaps);
                        if (next == Geom::Point(0,0)) {
                            end_angle_cross = 0;
                        }
                    }
                    Geom::Ray ray(hstart,hend);
                    Geom::Coord angle = ray.angle();
                    if (flip_side) {
                        angle = std::fmod(angle + rad_from_deg(180), 2*M_PI);
                        if (angle < 0) angle += 2*M_PI;
                    }
                    Geom::Coord angle_cross = std::fmod(angle + rad_from_deg(90), 2*M_PI);
                    if (angle_cross < 0) angle_cross += 2*M_PI;
                    angle = std::fmod(angle, 2*M_PI);
                    if (angle < 0) angle += 2*M_PI;
                    double turn = Geom::rad_from_deg(-90);
                    if (flip_side) {
                        end_angle_cross *= -1;
                        start_angle_cross *= -1;
                        //turn *= -1;
                    }
                    if (fix_overlaps != 180 && start_angle_cross != 0) {
                        double position_turned = position / sin(start_angle_cross/2.0);
                        hstart = hstart - Point::polar(angle_cross - (start_angle_cross/2.0) - turn, position_turned);
                    } else {
                        hstart = hstart - Point::polar(angle_cross, position);
                    }
                    createLine(start, hstart, g_strdup(Glib::ustring("infoline-on-start-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), false, remove);

                    if (fix_overlaps != 180 && end_angle_cross != 0) {
                        double position_turned = position / sin(end_angle_cross/2.0);
                        hend = hend - Point::polar(angle_cross + (end_angle_cross/2.0) + turn, position_turned);
                    } else {
                        hend = hend - Point::polar(angle_cross, position);
                    }
                    double length = Geom::distance(hstart,hend)  * scale;
                    Geom::Point pos = Geom::middle_point(hstart,hend);
                    if (arrows_outside) {
                        createArrowMarker("ArrowDINout-start");
                        createArrowMarker("ArrowDINout-end");
                    } else {
                        createArrowMarker("ArrowDIN-start");
                        createArrowMarker("ArrowDIN-end");
                    }
                    //We get the font size to offset the text to the middle
                    Pango::FontDescription fontdesc(Glib::ustring(fontbutton.param_getSVGValue()));
                    fontsize = fontdesc.get_size()/(double)Pango::SCALE;
                    fontsize *= document->getRoot()->c2p.inverse().expansionX();
                    
                    if (angle >= rad_from_deg(90) && angle < rad_from_deg(270)) {
                        pos = pos - Point::polar(angle_cross, text_top_bottom + (fontsize/2.5));
                    } else {
                        pos = pos + Point::polar(angle_cross, text_top_bottom + (fontsize/2.5));
                    }
                    double parents_scale = (affinetransform.expansionX() + affinetransform.expansionY()) / 2.0;
                    if (scale_sensitive) {
                        length *= parents_scale;
                    }
                    if ((anotation_width/2) > Geom::distance(hstart,hend)/2.0) {
                        pos = pos - Point::polar(angle_cross, position);
                    }
                    if (scale_sensitive && !affinetransform.preservesAngles()) {
                        createTextLabel(pos, counter, length, angle, remove, false);
                    } else {
                        createTextLabel(pos, counter, length, angle, remove, true);
                    }
                    const char * downline = g_strdup(Glib::ustring("downline-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str());
                    arrow_gap = 8 * Inkscape::Util::Quantity::convert(line_width / doc_scale, "mm", display_unit.c_str());
                    SPCSSAttr *css = sp_repr_css_attr_new();

                    char *oldlocale = g_strdup (setlocale(LC_NUMERIC, NULL));
                    setlocale (LC_NUMERIC, "C");
                    double width_line =  atof(sp_repr_css_property(css,"stroke-width","-1"));
                    setlocale (LC_NUMERIC, oldlocale);
                    g_free (oldlocale);
                    if (width_line > -0.0001) {
                         arrow_gap = 8 * Inkscape::Util::Quantity::convert(width_line/ doc_scale, "mm", display_unit.c_str());
                    }
                    if(flip_side) {
                       arrow_gap *= -1;
                    }
                    createLine(end, hend, g_strdup(Glib::ustring("infoline-on-end-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), false, remove);
                    if (!arrows_outside) {
                        hstart = hstart + Point::polar(angle, arrow_gap);
                        hend = hend - Point::polar(angle, arrow_gap );
                    }
                    if ((anotation_width/2) < Geom::distance(hstart,hend)/2.0) {
                        createLine(hstart, hend, g_strdup(Glib::ustring("infoline-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, remove, true);
                    } else {
                        createLine(hstart, hend, g_strdup(Glib::ustring("infoline-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
                    }
                } else {
                    const char * downline = g_strdup(Glib::ustring("downline-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str());
                    createLine(Geom::Point(),Geom::Point(), downline, true, true, true);
                    createLine(Geom::Point(), Geom::Point(), g_strdup(Glib::ustring("infoline-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
                    createLine(Geom::Point(), Geom::Point(), g_strdup(Glib::ustring("infoline-on-start-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
                    createLine(Geom::Point(), Geom::Point(), g_strdup(Glib::ustring("infoline-on-end-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
                    const char * id = g_strdup(Glib::ustring("text-on-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str());
                    SPObject *elemref = NULL;
                    if ((elemref = document->getObjectById(id))) {
                        elemref->deleteObject();
                    }
                }
            }
        }
        for (size_t k = ncurves; k <= previous_size; k++) {
            const char * downline = g_strdup(Glib::ustring("downline-").append(Glib::ustring::format(counter) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str());
            createLine(Geom::Point(),Geom::Point(), downline, true, true, true);
            createLine(Geom::Point(), Geom::Point(), g_strdup(Glib::ustring("infoline-").append(Glib::ustring::format(k) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
            createLine(Geom::Point(), Geom::Point(), g_strdup(Glib::ustring("infoline-on-start-").append(Glib::ustring::format(k) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
            createLine(Geom::Point(), Geom::Point(), g_strdup(Glib::ustring("infoline-on-end-").append(Glib::ustring::format(k) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str()), true, true, true);
            const char * id = g_strdup(Glib::ustring("text-on-").append(Glib::ustring::format(k) + Glib::ustring("-")).append(this->getRepr()->attribute("id")).c_str());
            SPObject *elemref = NULL;
            if ((elemref = document->getObjectById(id))) {
                elemref->deleteObject();
            }
        }
        previous_size = ncurves;
    }
}

void
LPEMeasureSegments::doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/)
{
    processObjects(LPE_VISIBILITY);
}

void 
LPEMeasureSegments::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    //set "keep paths" hook on sp-lpe-item.cpp
    if (keep_paths) {
        processObjects(LPE_TO_OBJECTS);
        items.clear();
        return;
    }
    processObjects(LPE_ERASE);
}

void
LPEMeasureSegments::transform_multiply(Geom::Affine const& postmul, bool set)
{
    SPStar * star = dynamic_cast<SPStar *>(sp_lpe_item);
    SPSpiral * spiral = dynamic_cast<SPSpiral *>(sp_lpe_item);
    if((spiral || star) && !postmul.withoutTranslation().isUniformScale()) {
        star_ellipse_fix = postmul;
        sp_lpe_item_update_patheffect(sp_lpe_item, false, false);
    }
}

Geom::PathVector
LPEMeasureSegments::doEffect_path(Geom::PathVector const &path_in)
{
    return path_in;
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
