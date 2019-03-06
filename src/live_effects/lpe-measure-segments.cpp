// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Some code and ideas migrated from dimensioning.py by
 * Johannes B. Rutzmoser, johannes.rutzmoser (at) googlemail (dot) com
 * https://github.com/Rutzmoser/inkscape_dimensioning
 * Copyright (C) 2014 Author(s)

 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpe-measure-segments.h"
#include "2geom/affine.h"
#include "2geom/angle.h"
#include "2geom/point.h"
#include "2geom/ray.h"
#include "display/curve.h"
#include "helper/geom.h"
#include "text-editing.h"
#include "object/sp-defs.h"
#include "object/sp-text.h"
#include "object/sp-flowtext.h"
#include "object/sp-item-group.h"
#include "object/sp-item.h"
#include "object/sp-path.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "svg/stringstream.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "svg/svg-length.h"
#include "util/units.h"
#include "xml/node.h"
#include "xml/sp-css-attr.h"
#include "libnrtype/Layout-TNG.h"
#include "document.h"
#include "document-undo.h"
#include "inkscape.h"
#include "preferences.h"
#include "path-chemistry.h"

#include <cmath>
#include <iomanip>
#include <libnrtype/font-lister.h>
#include <pangomm/fontdescription.h>

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {


static const Util::EnumData<OrientationMethod> OrientationMethodData[] = {
    { OM_HORIZONTAL , N_("Horizontal"), "horizontal" }, 
    { OM_VERTICAL   , N_("Vertical")  , "vertical"   },
    { OM_PARALLEL   , N_("Parallel")  , "parallel"   }
};
static const Util::EnumDataConverter<OrientationMethod> OMConverter(OrientationMethodData, OM_END);

LPEMeasureSegments::LPEMeasureSegments(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    unit(_("Unit"), _("Unit of measurement"), "unit", &wr, this, "mm"),
    orientation(_("Orientation"), _("Orientation of the line and labels"), "orientation", OMConverter, &wr, this, OM_PARALLEL, false),
    coloropacity(_("Color and opacity"), _("Set color and opacity of the dimensions"), "coloropacity", &wr, this, 0x000000ff),
    fontbutton(_("Font"), _("Select font for labels"), "fontbutton", &wr, this),
    precision(_("Precision"), _("Number of digits after the decimal point"), "precision", &wr, this, 2),
    fix_overlaps(_("Merge overlaps °"), _("Minimum angle at which overlapping dimension lines are merged into one, use 180° to disable merging"), "fix_overlaps", &wr, this, 0),
    position(_("Position"), _("Distance of dimension line from the path"), "position", &wr, this, 5),
    text_top_bottom(_("Label position"), _("Distance of the labels from the dimension line"), "text_top_bottom", &wr, this, 0),
    helpline_distance(_("Help line distance"), _("Distance of the perpendicular lines from the path"), "helpline_distance", &wr, this, 0.0),
    helpline_overlap(_("Help line elongation"), _("Distance of the perpendicular lines' ends from the dimension line"), "helpline_overlap", &wr, this, 2.0),
    line_width(_("Line width"), _("Dimension line width. DIN standard: 0.25 or 0.35 mm"), "line_width", &wr, this, 0.25),
    scale(_("Scale"), _("Scaling factor"), "scale", &wr, this, 1.0),
    
    format(_("Label format"), _("Label text format, available variables: {measure}, {unit}"), "format", &wr, this,"{measure}{unit}"),
    blacklist(_("Blacklist segments"), _("Comma-separated list of indices of segments that should not be measured. You can use another LPE with different parameters to measure these."), "blacklist", &wr, this,""),
    whitelist(_("Invert blacklist"), _("Use the blacklist as whitelist"), "whitelist", &wr, this, false),
    showindex(_("Show segment index"), _("Display the index of the segments in the text label for easier blacklisting"), "showindex", &wr, this, false),
    arrows_outside(_("Arrows outside"), _("Draw arrows pointing in the opposite direction outside the dimension line"), "arrows_outside", &wr, this, false),
    flip_side(_("Flip side"), _("Draw dimension lines and labels on the other side of the path"), "flip_side", &wr, this, false),
    scale_sensitive(_("Scale sensitive"), _("When the path is grouped and the group is then scaled, adjust the dimensions."), "scale_sensitive", &wr, this, true),
    local_locale(_("Localize number format"), _("Use localized number formatting, e.g. '1,0' instead of '1.0' with German locale"), "local_locale", &wr, this, true),
    rotate_anotation(_("Rotate labels"), _("Labels are parallel to the dimension line"), "rotate_anotation", &wr, this, true),
    hide_back(_("Hide line under label"), _("Hide the dimension line where the label overlaps it"), "hide_back", &wr, this, true),
    hide_arrows(_("Hide arrows"), _("Don't show any arrows"), "hide_arrows", &wr, this, false),
    smallx100(_("Multiply values < 1"), _("Multiply values smaller than 1 by 100 and leave out the unit"), "smallx100", &wr, this, false),
    linked_items(_("Linked objects:"), _("Objects whose nodes are projected onto the path and generate new measurements"), "linked_items", &wr, this),
    distance_projection(_("Distance"), _("Distance of the dimension lines from the outermost node"), "distance_projection", &wr, this, 20.0),
    angle_projection(_("Angle of projection"), _("Angle of projection in 90° steps"), "angle_projection", &wr, this, 0.0),
    active_projection(_("Activate projection"), _("Activate projection mode"), "active_projection", &wr, this, false),
    avoid_overlapping(_("Avoid label overlap"), _("Rotate labels if the segment is shorter than the label"), "avoid_overlapping", &wr, this, true),
    onbbox(_("Measure bounding box"), _("Add measurements for the geometrical bounding box"), "onbbox", &wr, this, false),
    bboxonly(_("Only bounding box"), _("Measure only the geometrical bounding box"), "bboxonly", &wr, this, false),
    centers(_("Add object center"), _("Add the projected object center"), "centers", &wr, this, false),
    maxmin(_("Only max and min"), _("Compute only max/min projection values"), "maxmin", &wr, this, false),
    helpdata(_("Help"), _("Measure segments help"), "helpdata", &wr, this, "", "")
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
    registerParameter(&active_projection);
    registerParameter(&whitelist);
    registerParameter(&showindex);
    registerParameter(&arrows_outside);
    registerParameter(&flip_side);
    registerParameter(&scale_sensitive);
    registerParameter(&local_locale);
    registerParameter(&rotate_anotation);
    registerParameter(&hide_back);
    registerParameter(&hide_arrows);
    registerParameter(&smallx100);
    registerParameter(&linked_items);
    registerParameter(&distance_projection);
    registerParameter(&angle_projection);
    registerParameter(&avoid_overlapping);
    registerParameter(&onbbox);
    registerParameter(&bboxonly);
    registerParameter(&centers);
    registerParameter(&maxmin);
    registerParameter(&helpdata);
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
    line_width.param_set_range(0, 999999.0);
    line_width.param_set_increments(0.1, 0.1);
    line_width.param_set_digits(2);
    helpline_distance.param_set_range(-999999.0, 999999.0);
    helpline_distance.param_set_increments(1, 1);
    helpline_distance.param_set_digits(2);
    helpline_overlap.param_set_range(-999999.0, 999999.0);
    helpline_overlap.param_set_increments(1, 1);
    helpline_overlap.param_set_digits(2);
    distance_projection.param_set_range(-999999.0, 999999.0);
    distance_projection.param_set_increments(1, 1);
    distance_projection.param_set_digits(5);
    angle_projection.param_set_range(0.0, 360.0);
    angle_projection.param_set_increments(90.0, 90.0);
    angle_projection.param_set_digits(2);
    locale_base = strdup(setlocale(LC_NUMERIC, nullptr));
    previous_size = 0;
    pagenumber = 0;
    helpdata.param_update_default(_("<b><big>General</big></b>\n"
                        "Display and position dimension lines and labels\n\n"
                        "<b><big>Projection</big></b>\n"
                        "Show a line with measurements based on the selected items\n\n"
                        "<b><big>Options</big></b>\n"
                        "Options for color, precision, label formatting and display\n\n"
                        "<b><big>Tips</big></b>\n"
                        "<b><i>Custom styling:</i></b> To further customize the styles, "
                        "use the XML editor to find out the class or ID, then use the "
                        "Style dialog to apply a new style.\n"
                        "<b><i>Blacklists:</i></b> allow to hide some segments or projection steps.\n"
                        "<b><i>Multiple Measure LPEs:</i></b> In the same object, in conjunction with blacklists,"
                        "this allows for labels and measurements with different orientations or additional projections.\n"
                        "<b><i>Set Defaults:</i></b> For every LPE, default values can be set at the bottom."));
}

LPEMeasureSegments::~LPEMeasureSegments() {
    doOnRemove(nullptr);
}

Gtk::Widget *
LPEMeasureSegments::newWidget()
{
    // use manage here, because after deletion of Effect object, others might still be pointing to this widget.
    Gtk::VBox * vbox = Gtk::manage( new Gtk::VBox() );
    vbox->set_border_width(0);
    vbox->set_homogeneous(false);
    vbox->set_spacing(0);
    Gtk::VBox *vbox0 = Gtk::manage(new Gtk::VBox());
    vbox0->set_border_width(5);
    vbox0->set_homogeneous(false);
    vbox0->set_spacing(2);
    Gtk::VBox *vbox1 = Gtk::manage(new Gtk::VBox());
    vbox1->set_border_width(5);
    vbox1->set_homogeneous(false);
    vbox1->set_spacing(2);
    Gtk::VBox *vbox2 = Gtk::manage(new Gtk::VBox());
    vbox2->set_border_width(5);
    vbox2->set_homogeneous(false);
    vbox2->set_spacing(2);
    //Help page
    Gtk::VBox *vbox3 = Gtk::manage(new Gtk::VBox());
    vbox3->set_border_width(5);
    vbox3->set_homogeneous(false);
    vbox3->set_spacing(2);
    std::vector<Parameter *>::iterator it = param_vector.begin();
    while (it != param_vector.end()) {
        if ((*it)->widget_is_visible) {
            Parameter * param = *it;
            Gtk::Widget * widg = param->param_newWidget();
            Glib::ustring * tip = param->param_getTooltip();
            if (widg) {
                if (       param->param_key == "linked_items") {
                    vbox1->pack_start(*widg, true, true, 2);
                } else if (param->param_key == "active_projection"   ||
                           param->param_key == "distance_projection" ||
                           param->param_key == "angle_projection"    ||
                           param->param_key == "maxmin"              ||
                           param->param_key == "centers"              ||
                           param->param_key == "bboxonly"            ||
                           param->param_key == "onbbox"                )
                {
                    vbox1->pack_start(*widg, false, true, 2);
                } else if (param->param_key == "precision"     ||
                           param->param_key == "coloropacity"  ||
                           param->param_key == "font"          ||
                           param->param_key == "format"        ||
                           param->param_key == "blacklist"     ||
                           param->param_key == "whitelist"     ||
                           param->param_key == "showindex"     ||
                           param->param_key == "local_locale"  ||
                           param->param_key == "smallx100"     ||
                           param->param_key == "hide_arrows"     )
                {
                    vbox2->pack_start(*widg, false, true, 2);
                } else if (param->param_key == "helpdata")
                {
                    vbox3->pack_start(*widg, false, true, 2);
                } else {
                    vbox0->pack_start(*widg, false, true, 2);
                }

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

    Gtk::Notebook * notebook = Gtk::manage(new Gtk::Notebook());
    notebook->append_page (*vbox0, Glib::ustring(_("General")));
    notebook->append_page (*vbox1, Glib::ustring(_("Projection")));
    notebook->append_page (*vbox2, Glib::ustring(_("Options")));
    notebook->append_page (*vbox3, Glib::ustring(_("Help")));
    vbox0->show_all();
    vbox1->show_all();
    vbox2->show_all();
    vbox3->show_all();
    vbox->pack_start(*notebook, true, true, 2);
    notebook->set_current_page(pagenumber);
    notebook->signal_switch_page().connect(sigc::mem_fun(*this, &LPEMeasureSegments::on_my_switch_page));
    if(Gtk::Widget* widg = defaultParamSet()) {
        //Wrap to make it more omogenious
        Gtk::VBox *vbox4 = Gtk::manage(new Gtk::VBox());
        vbox4->set_border_width(5);
        vbox4->set_homogeneous(false);
        vbox4->set_spacing(2);
        vbox4->pack_start(*widg, true, true, 2);
        vbox->pack_start(*vbox4, true, true, 2);
    }
    return dynamic_cast<Gtk::Widget *>(vbox);
}

void 
LPEMeasureSegments::on_my_switch_page(Gtk::Widget* page, guint page_number)
{
    if(!page->get_parent()->in_destruction()) {
        pagenumber = page_number;
    }
}

void
LPEMeasureSegments::createArrowMarker(Glib::ustring mode)
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document || !sp_lpe_item|| !sp_lpe_item->getId()) {
        return;
    }
    Glib::ustring lpobjid = this->lpeobj->getId();
    Glib::ustring itemid  = sp_lpe_item->getId();
    Glib::ustring style;
    style = Glib::ustring("fill:context-stroke;");
    Inkscape::SVGOStringStream os;
    os << SP_RGBA32_A_F(coloropacity.get_value());
    style = style + Glib::ustring(";fill-opacity:") + Glib::ustring(os.str());
    style = style + Glib::ustring(";stroke:none");
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    SPObject *elemref = nullptr;
    Inkscape::XML::Node *arrow = nullptr;
    if ((elemref = document->getObjectById(mode.c_str()))) {
        Inkscape::XML::Node *arrow= elemref->getRepr();
        if (arrow) {
            arrow->setAttribute("sodipodi:insensitive", "true");
            arrow->setAttribute("transform", nullptr);
            Inkscape::XML::Node *arrow_data = arrow->firstChild();
            if (arrow_data) {
                arrow_data->setAttribute("transform", nullptr);
                arrow_data->setAttribute("style", style.c_str());
            }
        }
    } else {
        arrow = xml_doc->createElement("svg:marker");
        arrow->setAttribute("id", mode.c_str());
        Glib::ustring classarrow = itemid;
        classarrow += " ";
        classarrow += lpobjid;
        classarrow += " measure-arrows-marker";
        arrow->setAttribute("class", classarrow.c_str());
        arrow->setAttribute("inkscape:stockid", mode.c_str());
        arrow->setAttribute("orient", "auto");
        arrow->setAttribute("refX", "0.0");
        arrow->setAttribute("refY", "0.0");

        arrow->setAttribute("sodipodi:insensitive", "true");
        /* Create <path> */
        Inkscape::XML::Node *arrow_path = xml_doc->createElement("svg:path");
        if (std::strcmp(mode.c_str(), "ArrowDIN-start") == 0) {
            arrow_path->setAttribute("d", "M -8,0 8,-2.11 8,2.11 z");
        } else if (std::strcmp(mode.c_str(), "ArrowDIN-end") == 0) {
            arrow_path->setAttribute("d", "M 8,0 -8,2.11 -8,-2.11 z");
        } else if (std::strcmp(mode.c_str(), "ArrowDINout-start") == 0) {
            arrow_path->setAttribute("d", "M 0,0 -16,2.11 -16,0.5 -26,0.5 -26,-0.5 -16,-0.5 -16,-2.11 z");
        } else {
            arrow_path->setAttribute("d", "M 0,0 16,-2.11 16,-0.5 26,-0.5 26,0.5 16,0.5 16,2.11 z");
        }
        Glib::ustring classarrowpath = itemid;
        classarrowpath += " ";
        classarrowpath += lpobjid;
        classarrowpath += " measure-arrows";
        arrow_path->setAttribute("class", classarrowpath.c_str());
        Glib::ustring arrowpath = mode + Glib::ustring("_path");
        arrow_path->setAttribute("id", arrowpath.c_str());
        arrow_path->setAttribute("style", style.c_str());
        arrow->addChild(arrow_path, nullptr);
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
    if (!document || !sp_lpe_item || !sp_lpe_item->getId()) {
        return;
    }
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *rtext = nullptr;

    Glib::ustring lpobjid = this->lpeobj->getId();
    Glib::ustring itemid  = sp_lpe_item->getId();
    Glib::ustring id = Glib::ustring("text-on-");
    id += Glib::ustring::format(counter);
    id += "-";
    id += lpobjid;
    SPObject *elemref = nullptr;
    Inkscape::XML::Node *rtspan = nullptr;
    elemref = document->getObjectById(id.c_str());
    if (elemref) {
        rtext = elemref->getRepr();
        sp_repr_set_svg_double(rtext, "x", pos[Geom::X]);
        sp_repr_set_svg_double(rtext, "y", pos[Geom::Y]);
        rtext->setAttribute("sodipodi:insensitive", "true");
        rtext->setAttribute("transform", nullptr);
    } else {
        rtext = xml_doc->createElement("svg:text");
        rtext->setAttribute("xml:space", "preserve");
        rtext->setAttribute("id", id.c_str());
        Glib::ustring classlabel = itemid;
        classlabel += " ";
        classlabel += lpobjid;
        classlabel += " measure-labels";
        rtext->setAttribute("class", classlabel.c_str());
        rtext->setAttribute("sodipodi:insensitive", "true");
        sp_repr_set_svg_double(rtext, "x", pos[Geom::X]);
        sp_repr_set_svg_double(rtext, "y", pos[Geom::Y]);
        rtspan = xml_doc->createElement("svg:tspan");
        rtspan->setAttribute("sodipodi:role", "line");
    }
    SPCSSAttr *css = sp_repr_css_attr_new();
    Inkscape::FontLister *fontlister = Inkscape::FontLister::get_instance();
    gchar * fontbutton_str = fontbutton.param_getSVGValue();
    fontlister->fill_css(css, Glib::ustring(fontbutton_str));
    g_free(fontbutton_str);
    std::stringstream font_size;
    setlocale (LC_NUMERIC, "C");
    font_size <<  fontsize << "pt";
    setlocale (LC_NUMERIC, locale_base);
    gchar c[32];
    sprintf(c, "#%06x", rgb32 >> 8);
    sp_repr_css_set_property (css, "fill",c);
    Inkscape::SVGOStringStream os;
    os << SP_RGBA32_A_F(coloropacity.get_value());
    sp_repr_css_set_property (css, "fill-opacity",os.str().c_str());
    if (!rtspan) {
        rtspan = rtext->firstChild();
    }
    sp_repr_css_set_property (css, "font-size",font_size.str().c_str());
    if (remove) {
        sp_repr_css_set_property (css, "display","none");
    }
    sp_repr_css_set_property (css, "font-size",font_size.str().c_str());
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    rtext->setAttribute("style", css_str.c_str());
    rtspan->setAttribute("style", css_str.c_str());
    rtspan->setAttribute("transform", nullptr);
    sp_repr_css_attr_unref (css);
    if (!elemref) {
        rtext->addChild(rtspan, nullptr);
        Inkscape::GC::release(rtspan);
    }
    length = Inkscape::Util::Quantity::convert(length / doc_scale, display_unit.c_str(), unit.get_abbreviation());
    if (local_locale) {
        setlocale (LC_NUMERIC, "");
    } else {
        setlocale (LC_NUMERIC, "C");
    }
    gchar length_str[64];
    bool x100 = false;
    if (smallx100 && length < 1 ) {
        length *=100;
        x100 = true;
        g_snprintf(length_str, 64, "%.*f", (int)precision - 2, length);
    } else {
        g_snprintf(length_str, 64, "%.*f", (int)precision, length);
    }
    setlocale (LC_NUMERIC, locale_base);
    gchar * format_str = format.param_getSVGValue();
    Glib::ustring label_value(format_str);
    g_free(format_str);
    size_t s = label_value.find(Glib::ustring("{measure}"),0);
    if(s < label_value.length()) {
        label_value.replace(s,s+9,length_str);
    }
    
    s = label_value.find(Glib::ustring("{unit}"),0);
    if(s < label_value.length()) {
        if (x100) {
            label_value.replace(s,s+6,"");
        } else {
            label_value.replace(s,s+6,unit.get_abbreviation());
        }
    }
    if (showindex) {
        label_value = Glib::ustring("[") + Glib::ustring::format(counter) + Glib::ustring("] ") + label_value;
    }
    if ( !valid ) {
        label_value = Glib::ustring(_("Non Uniform Scale"));
    }
    Inkscape::XML::Node *rstring = nullptr;
    if (!elemref) {
        rstring = xml_doc->createTextNode(label_value.c_str());
        rtspan->addChild(rstring, nullptr);
        Inkscape::GC::release(rstring);
    } else {
        rstring = rtspan->firstChild();
        rstring->setContent(label_value.c_str());
    }
    if (!elemref) {
        elemref = document->getRoot()->appendChildRepr(rtext);
        Inkscape::GC::release(rtext);
    } 
    Geom::OptRect bounds = SP_ITEM(elemref)->bounds(SPItem::GEOMETRIC_BBOX);
    if (bounds) {
        anotation_width = bounds->width() * 1.15;
        rtspan->setAttribute("style", nullptr);
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
        transform = nullptr;
    }
    rtext->setAttribute("transform", transform);
    g_free(transform);
}

void
LPEMeasureSegments::createLine(Geom::Point start,Geom::Point end, Glib::ustring name, size_t counter, bool main, bool remove, bool arrows)
{
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document || !sp_lpe_item || !sp_lpe_item->getId()) {
        return;
    }
    Glib::ustring lpobjid = this->lpeobj->getId();
    Glib::ustring itemid  = sp_lpe_item->getId(); 
    Glib::ustring id = name;
    id += Glib::ustring::format(counter);
    id += "-";
    id += lpobjid;
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    SPObject *elemref = document->getObjectById(id.c_str());
    Inkscape::XML::Node *line = nullptr;
    if (!main) {
        Geom::Ray ray(start, end);
        Geom::Coord angle = ray.angle();
        start = start + Point::polar(angle, helpline_distance );
        end = end + Point::polar(angle, helpline_overlap );
    }
    Geom::PathVector line_pathv;
    
    double k = (Geom::distance(start,end)/2.0) - (anotation_width/10.0) - (anotation_width/2.0);
    if (main && 
        std::abs(text_top_bottom) < fontsize/1.5 && 
        hide_back &&
        k > 0)
    {
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
    if (elemref) {
        line = elemref->getRepr();
        gchar * line_str = sp_svg_write_path( line_pathv );
        line->setAttribute("d" , line_str);
        line->setAttribute("transform", nullptr);
        g_free(line_str);
    } else {
        line = xml_doc->createElement("svg:path");
        line->setAttribute("id", id.c_str());
        if (main) {
            Glib::ustring classlinedim = itemid;
            classlinedim += " ";
            classlinedim += lpobjid;
            classlinedim += " measure-DIM-lines measure-lines";
            line->setAttribute("class", classlinedim.c_str());
        } else {
            Glib::ustring classlinehelper = itemid;
            classlinehelper += " ";
            classlinehelper += lpobjid;
            classlinehelper += " measure-helper-lines measure-lines";
            line->setAttribute("class", classlinehelper.c_str());
        }
        gchar * line_str = sp_svg_write_path( line_pathv );
        line->setAttribute("d" , line_str);
        g_free(line_str);
    }

    line->setAttribute("sodipodi:insensitive", "true");
    line_pathv.clear();
        
    Glib::ustring style;
    if (remove) {
        style ="display:none;";
    }
    if (main) {
        line->setAttribute("inkscape:label", "dinline");
        if (!hide_arrows) {
            if (arrows_outside) {
                style += "marker-start:url(#ArrowDINout-start);marker-end:url(#ArrowDINout-end);";
            } else {
                style += "marker-start:url(#ArrowDIN-start);marker-end:url(#ArrowDIN-end);";
            }
        }
    } else {
        line->setAttribute("inkscape:label", "dinhelpline");
    }
    std::stringstream stroke_w;
    setlocale (LC_NUMERIC, "C");
    
    double stroke_width = Inkscape::Util::Quantity::convert(line_width / doc_scale, "mm", display_unit.c_str());
    stroke_w <<  stroke_width;
    setlocale (LC_NUMERIC, locale_base);
    style  += "stroke-width:";
    style  += stroke_w.str();
    gchar c[32];
    sprintf(c, "#%06x", rgb32 >> 8);
    style += ";stroke:";
    style += Glib::ustring(c);
    Inkscape::SVGOStringStream os;
    os << SP_RGBA32_A_F(coloropacity.get_value());
    style = style + Glib::ustring(";stroke-opacity:") + Glib::ustring(os.str());
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css, style.c_str());
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    line->setAttribute("style", css_str.c_str());
    if (!elemref) {
        elemref = document->getRoot()->appendChildRepr(line);
        Inkscape::GC::release(line);
    }
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
    Inkscape::XML::Node *styleNode = nullptr;
    Inkscape::XML::Node* textNode = nullptr;
    Inkscape::XML::Node *root = SP_ACTIVE_DOCUMENT->getReprRoot();
    for (unsigned i = 0; i < root->childCount(); ++i) {
        if (Glib::ustring(root->nthChild(i)->name()) == "svg:style") {

            styleNode = root->nthChild(i);

            for (unsigned j = 0; j < styleNode->childCount(); ++j) {
                if (styleNode->nthChild(j)->type() == Inkscape::XML::TEXT_NODE) {
                    textNode = styleNode->nthChild(j);
                }
            }

            if (textNode == nullptr) {
                // Style element found but does not contain text node!
                std::cerr << "StyleDialog::_getStyleTextNode(): No text node!" << std::endl;
                textNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");
                styleNode->appendChild(textNode);
                Inkscape::GC::release(textNode);
            }
        }
    }

    if (styleNode == nullptr) {
        // Style element not found, create one
        styleNode = SP_ACTIVE_DOCUMENT->getReprDoc()->createElement("svg:style");
        textNode  = SP_ACTIVE_DOCUMENT->getReprDoc()->createTextNode("");

        styleNode->appendChild(textNode);
        Inkscape::GC::release(textNode);

        root->addChild(styleNode, nullptr);
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

Geom::PathVector 
LPEMeasureSegments::doEffect_path (Geom::PathVector const & path_in) {
    return path_in;
}

bool
LPEMeasureSegments::isWhitelist (size_t i, std::string listsegments, bool whitelist)
{
    size_t s = listsegments.find(std::to_string(i) + std::string(","), 0);
    if (s != std::string::npos) {
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

double getAngle(Geom::Point p1, Geom::Point p2, Geom::Point p3, bool flip_side, double fix_overlaps)
{
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

std::vector< Point > 
transformNodes(std::vector< Point > nodes, Geom::Affine transform)
{
    std::vector< Point > result;
    for (auto & node : nodes) {
        Geom::Point point = node;
        result.push_back(point * transform);
    }
    return result;
}

std::vector< Point > 
getNodes(SPItem * item, Geom::Affine transform, bool onbbox, bool centers, bool bboxonly)
{
    std::vector< Point > current_nodes;
    SPShape    * shape    = dynamic_cast<SPShape     *> (item);
    SPText     * text     = dynamic_cast<SPText      *> (item);
    SPGroup    * group    = dynamic_cast<SPGroup     *> (item);
    SPFlowtext * flowtext = dynamic_cast<SPFlowtext  *> (item);
    //TODO handle clones/use
    if (group) {
        std::vector<SPItem*> const item_list = sp_item_group_item_list(group);
        for (auto sub_item : item_list) {
            std::vector< Point > nodes = transformNodes(getNodes(sub_item, sub_item->transform, onbbox, centers, bboxonly), transform);
            current_nodes.insert(current_nodes.end(), nodes.begin(), nodes.end());
        }
    } else if (shape && !bboxonly) {
        SPCurve * c = shape->getCurve();
        current_nodes = transformNodes(c->get_pathvector().nodes(), transform);
        c->unref();
    } else if ((text || flowtext) && !bboxonly) {
        Inkscape::Text::Layout::iterator iter = te_get_layout(item)->begin();
        do {
            Inkscape::Text::Layout::iterator iter_next = iter;
            iter_next.nextGlyph(); // iter_next is one glyph ahead from iter
            if (iter == iter_next) {
                break;
            }
            // get path from iter to iter_next:
            SPCurve *curve = te_get_layout(item)->convertToCurves(iter, iter_next);
            iter = iter_next; // shift to next glyph
            if (!curve) {
                continue; // error converting this glyph
            }
            if (curve->is_empty()) { // whitespace glyph?
                curve->unref();
                continue;
            }
            std::vector< Point > letter_nodes = transformNodes(curve->get_pathvector().nodes(), transform);
            current_nodes.insert(current_nodes.end(),letter_nodes.begin(),letter_nodes.end());
            if (iter == te_get_layout(item)->end()) {
                break;
            }
        } while (true);
    } else {
        onbbox = true;
    }
    if (onbbox || centers) {
        Geom::OptRect bbox = item->geometricBounds(item->transform);
        if (bbox && onbbox) {
            current_nodes.push_back((*bbox).corner(0) * transform);
            current_nodes.push_back((*bbox).corner(1) * transform);
            current_nodes.push_back((*bbox).corner(2) * transform);
            current_nodes.push_back((*bbox).corner(3) * transform);
        }
        if (bbox && centers) {
            current_nodes.push_back((*bbox).midpoint() * transform);
        }
    }
    return current_nodes;
}

bool sortPoints (Geom::Point a,Geom::Point b) { 
    return (a[Geom::Y] < b[Geom::Y]); 
}

void
LPEMeasureSegments::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPLPEItem * splpeitem = const_cast<SPLPEItem *>(lpeitem);
    Glib::ustring lpobjid = this->lpeobj->getId();
    SPDocument * document = SP_ACTIVE_DOCUMENT;
    if (!document) {
        return;
    }
    //Avoid crashes on previews
    Inkscape::XML::Node *root = splpeitem->document->getReprRoot();
    Inkscape::XML::Node *root_origin = document->getReprRoot();
    if (root_origin != root) {
        return;
    }
    Geom::Affine parentaffinetransform = i2anc_affine(SP_OBJECT(lpeitem->parent), SP_OBJECT(document->getRoot()));
    Geom::Affine affinetransform = i2anc_affine(SP_OBJECT(lpeitem), SP_OBJECT(document->getRoot()));
    //Projection prepare
    Geom::PathVector pathvector;
    std::vector< Point > nodes;
    if (active_projection) {
        Geom::OptRect bbox = sp_lpe_item->geometricBounds();
        Geom::Point pojpoint = Geom::Point();
        if (bbox) {
            Geom::Point mid =  bbox->midpoint();
            double angle = Geom::rad_from_deg(angle_projection);
            Geom::Affine transform = affinetransform;
            transform *= Geom::Translate(mid).inverse();
            transform *= Geom::Rotate(angle).inverse();
            transform *= Geom::Translate(mid);
            std::vector< Point > current_nodes = getNodes(splpeitem, transform, onbbox, centers, bboxonly);
            nodes.insert(nodes.end(),current_nodes.begin(), current_nodes.end());
            std::vector<Point> result;
            Geom::OptRect bbox = sp_lpe_item->geometricBounds(sp_lpe_item->transform);
            Geom::Point pojpoint = Geom::Point();
            double maxdistance = -std::numeric_limits<double>::max();
            for (auto & node : nodes) {
                Geom::Point point = node;
                point *= Geom::Translate(mid).inverse();
                point *= Geom::Rotate(angle).inverse();
                point *= Geom::Translate(mid);
                if (point[Geom::X] > maxdistance) {
                    maxdistance = point[Geom::X];
                }
            }
            for (auto & iter : linked_items._vector) {
                SPObject *obj;
                if (iter->ref.isAttached() &&  iter->actived && (obj = iter->ref.getObject()) && SP_IS_ITEM(obj)) {
                    SPItem * item = dynamic_cast<SPItem *>(obj);
                    if (item) {
                        Geom::Affine affinetransform_sub = i2anc_affine(SP_OBJECT(item), SP_OBJECT(document->getRoot()));
                        Geom::Affine transform = affinetransform_sub ;
                        transform *= Geom::Translate(-mid);
                        transform *= Geom::Rotate(angle).inverse();
                        transform *= Geom::Translate(mid);
                        std::vector< Point > current_nodes = getNodes(item, transform, onbbox, centers, bboxonly);
                        nodes.insert(nodes.end(),current_nodes.begin(), current_nodes.end());
                    }
                }
            }

            for (auto & node : nodes) {
                Geom::Point point = node;
                double dproj = Inkscape::Util::Quantity::convert(distance_projection, display_unit.c_str(), unit.get_abbreviation());
                Geom::Coord xpos = maxdistance + dproj;
                result.emplace_back(xpos, point[Geom::Y]);
            }
            std::sort (result.begin(), result.end(), sortPoints);
            Geom::Path path;
            Geom::Point prevpoint(0,0);
            size_t counter = 0;
            bool started = false;
            for (auto & iter : result) {
                Geom::Point point = iter;
                if (Geom::are_near(prevpoint, point)){
                    continue;
                }
                if (!started) {
                    path.setInitial(point);
                    started = true;
                } else {
                    if (!maxmin) {
                        path.appendNew<Geom::LineSegment>(point);
                    }
                }
                prevpoint = point;
            }
            if (maxmin) {
                path.appendNew<Geom::LineSegment>(result[result.size()-1]);
            }
            pathvector.push_back(path);
            pathvector *= Geom::Translate(-mid);
            pathvector *= Geom::Rotate(angle);
            pathvector *= Geom::Translate(mid);
        }
    }

    //end projection prepare
    SPShape *shape = dynamic_cast<SPShape *>(splpeitem);
    if (shape) {
        //only check constrain viewbox on X
        Geom::Scale scaledoc = document->getDocumentScale();
        display_unit = document->getDisplayUnit()->abbr.c_str();
        doc_scale = Inkscape::Util::Quantity::convert( scaledoc[Geom::X], "px", display_unit.c_str() );
        if (doc_scale > 0) {
            doc_scale= 1.0/doc_scale;
        } else {
            doc_scale = 1.0;
        }
        guint32 color32 = coloropacity.get_value();
        bool colorchanged = false;
        if (color32 != rgb32) {
            colorchanged = true;
        }
        rgb32 = color32;
        SPCurve * c = nullptr;
        gchar * fontbutton_str = fontbutton.param_getSVGValue();
        Glib::ustring fontdesc_ustring = Glib::ustring(fontbutton_str);
        Pango::FontDescription fontdesc(fontdesc_ustring);
        fontsize = fontdesc.get_size()/(double)Pango::SCALE;
        fontsize *= document->getRoot()->c2p.inverse().expansionX();
        g_free(fontbutton_str);
        fontsize *= document->getRoot()->c2p.inverse().expansionX();
        c = shape->getCurve();
        Geom::Point prev_stored = Geom::Point(0,0);
        Geom::Point start_stored = Geom::Point(0,0);
        Geom::Point end_stored = Geom::Point(0,0); 
        Geom::Point next_stored = Geom::Point(0,0);
        if (!active_projection) {
            pathvector =  pathv_to_linear_and_cubic_beziers(c->get_pathvector());
            pathvector *= affinetransform;
        }
        c->unref();
        gchar * format_str = format.param_getSVGValue();
        if (Glib::ustring(format_str).empty()) {
            format.param_setValue(Glib::ustring("{measure}{unit}"));
        }
        g_free(format_str);
        size_t ncurves = pathvector.curveCount();
        items.clear();
        double start_angle_cross = 0;
        double end_angle_cross = 0;
        gint counter = -1;
        bool previous_fix_overlaps = true;
        for (size_t i = 0; i < pathvector.size(); i++) {
            size_t count = pathvector[i].size_default();
            if ( pathvector[i].closed()) {
              const Geom::Curve &closingline = pathvector[i].back_closed(); 
              if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                count = pathvector[i].size_open();
              }
            }
            for (size_t j = 0; j < count; j++) {
                counter++;
                gint fix_overlaps_degree = fix_overlaps;
                Geom::Point prev = Geom::Point(0,0);
                if (j == 0 && pathvector[i].closed()) {
                    prev = pathvector.pointAt(pathvector[i].size() - 1);
                } else if (j != 0) {
                    prev = pathvector[i].pointAt(j - 1);
                }
                Geom::Point start = pathvector[i].pointAt(j);
                Geom::Point end = pathvector[i].pointAt(j + 1);
                Geom::Point next = Geom::Point(0,0);
                if (pathvector[i].closed() && pathvector[i].size() == j+1){
                    end = pathvector[i].pointAt(0);
                    next = pathvector[i].pointAt(1);
                } else if (pathvector[i].size() > j + 1) {
                    next = pathvector[i].pointAt(j+2);
                }
                gchar * blacklist_str = blacklist.param_getSVGValue();
                std::string listsegments(std::string(blacklist_str) + std::string(","));
                listsegments.erase(std::remove(listsegments.begin(), listsegments.end(), ' '), listsegments.end());
                g_free(blacklist_str);
                if (isWhitelist(counter, listsegments, (bool)whitelist) && !Geom::are_near(start, end)) {
                    Glib::ustring idprev = Glib::ustring("infoline-on-start-");
                    idprev += Glib::ustring::format(counter-1);
                    idprev += "-";
                    idprev += lpobjid;
                    SPObject *elemref = document->getObjectById(idprev.c_str());
                    if (elemref){
                        SPPath* path = dynamic_cast<SPPath *>(elemref);
                        if (path) {
                            SPCurve* prevcurve = path->getCurve();
                            if (prevcurve) {
                                prev_stored = *prevcurve->first_point();
                            }
                            prevcurve->unref();
                        }
                    }
                    Glib::ustring idstart = Glib::ustring("infoline-on-start-");
                    idstart += Glib::ustring::format(counter);
                    idstart += "-";
                    idstart += lpobjid;
                    elemref = document->getObjectById(idstart.c_str());
                    if (elemref) {
                        SPPath* path = dynamic_cast<SPPath *>(elemref);
                        if (path) {
                            SPCurve* startcurve = path->getCurve();
                            if (startcurve) {
                                start_stored = *startcurve->first_point();
                            }
                            startcurve->unref();
                        }
                    }
                    Glib::ustring idend = Glib::ustring("infoline-on-end-");
                    idend += Glib::ustring::format(counter);
                    idend += "-";
                    idend += lpobjid;
                    elemref = document->getObjectById(idend.c_str());
                    if (elemref) {
                        SPPath* path = dynamic_cast<SPPath *>(elemref);
                        if (path) {
                            SPCurve* endcurve = path->getCurve();
                            if (endcurve) {
                                end_stored = *endcurve->first_point();
                            }
                            endcurve->unref();
                        }
                    }
                    Glib::ustring idnext = Glib::ustring("infoline-on-start-");
                    idnext += Glib::ustring::format(counter+1);
                    idnext += "-";
                    idnext += lpobjid;
                    elemref = document->getObjectById(idnext.c_str());
                    if (elemref) {
                        SPPath* path = dynamic_cast<SPPath *>(elemref);
                        if (path) {
                            SPCurve* nextcurve = path->getCurve();
                            if (nextcurve) {
                                next_stored = *nextcurve->first_point();
                            }
                            nextcurve->unref();
                        }
                    }
                    Glib::ustring infoline_on_start = "infoline-on-start-";
                    infoline_on_start += Glib::ustring::format(counter);
                    infoline_on_start += "-";
                    infoline_on_start += lpobjid;
                    
                    Glib::ustring infoline_on_end = "infoline-on-end-";
                    infoline_on_end += Glib::ustring::format(counter);
                    infoline_on_end += "-";
                    infoline_on_end += lpobjid;
                    
                    Glib::ustring infoline = "infoline-";
                    infoline += Glib::ustring::format(counter);
                    infoline += "-";
                    infoline += lpobjid;
                    
                    Glib::ustring texton = "text-on-";
                    texton += Glib::ustring::format(counter);
                    texton += "-";
                    texton += lpobjid;
                    items.push_back(infoline_on_start);
                    items.push_back(infoline_on_end);
                    items.push_back(infoline);
                    items.push_back(texton);
                    if (!hide_arrows) {
                        if (arrows_outside) {
                            items.emplace_back("ArrowDINout-start");
                            items.emplace_back("ArrowDINout-end");
                        } else {
                            items.emplace_back("ArrowDIN-start");
                            items.emplace_back("ArrowDIN-end");
                        }
                    }
                    if (((Geom::are_near(prev, prev_stored, 0.01) && Geom::are_near(next, next_stored, 0.01)) || 
                         fix_overlaps_degree == 180) &&
                        Geom::are_near(start, start_stored, 0.01) && 
                        Geom::are_near(end, end_stored, 0.01) && 
                        !this->upd_params &&
                        !colorchanged)
                    {
                        continue;
                    }
                    Geom::Point hstart = start;
                    Geom::Point hend = end;
                    bool remove = false;
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
                    } else if (fix_overlaps_degree != 180) {
                        start_angle_cross = getAngle( start, prev, end, flip_side, fix_overlaps_degree);
                        if (prev == Geom::Point(0,0)) {
                            start_angle_cross = 0;
                        }
                        end_angle_cross = getAngle(end, start, next, flip_side, fix_overlaps_degree);
                        if (next == Geom::Point(0,0)) {
                            end_angle_cross = 0;
                        }
                    }
                    if (remove) {
                        createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-"), counter, true, true, true);
                        createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-on-start-"), counter, true, true, true);
                        createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-on-end-"), counter, true, true, true);
                        createTextLabel(Geom::Point(), counter, 0, 0, true, true);
                        continue;
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
                    double position_turned_start = position / sin(start_angle_cross/2.0);
                    double length = Geom::distance(start,end);
                    if (fix_overlaps_degree != 180 && 
                        start_angle_cross != 0 && 
                        position_turned_start < length &&
                        previous_fix_overlaps) 
                    {
                        hstart = hstart - Point::polar(angle_cross - (start_angle_cross/2.0) - turn, position_turned_start);
                    } else {
                        hstart = hstart - Point::polar(angle_cross, position);
                    }
                    createLine(start, hstart, Glib::ustring("infoline-on-start-"), counter, false, false);
                    double position_turned_end = position / sin(end_angle_cross/2.0);
                    double endlength = Geom::distance(end,next);
                    if (fix_overlaps_degree != 180 && 
                        end_angle_cross != 0 && 
                        position_turned_end < length && 
                        position_turned_end < endlength) 
                    {
                        hend = hend - Point::polar(angle_cross + (end_angle_cross/2.0) + turn, position_turned_end);
                        previous_fix_overlaps = true;
                    } else {
                        hend = hend - Point::polar(angle_cross, position);
                        previous_fix_overlaps = false;
                    }
                    length = Geom::distance(start,end)  * scale;
                    Geom::Point pos = Geom::middle_point(hstart,hend);
                    if (!hide_arrows) {
                        if (arrows_outside) {
                            createArrowMarker(Glib::ustring("ArrowDINout-start"));
                            createArrowMarker(Glib::ustring("ArrowDINout-end"));
                        } else {
                            createArrowMarker(Glib::ustring("ArrowDIN-start"));
                            createArrowMarker(Glib::ustring("ArrowDIN-end"));
                        }
                    }
                    if (angle >= rad_from_deg(90) && angle < rad_from_deg(270)) {
                        pos = pos - Point::polar(angle_cross, text_top_bottom  + (fontsize/2.5));
                    } else {
                        pos = pos + Point::polar(angle_cross, text_top_bottom + (fontsize/2.5));
                    }
                    double parents_scale = (parentaffinetransform.expansionX() + parentaffinetransform.expansionY()) / 2.0;
                    if (!scale_sensitive) {
                        length /= parents_scale;
                    }
                    if ((anotation_width/2) > Geom::distance(hstart,hend)/2.0) {
                        if (avoid_overlapping) {
                            pos = pos - Point::polar(angle_cross, position + (anotation_width/2.0));
                            angle += Geom::rad_from_deg(90);
                        } else {
                            pos = pos - Point::polar(angle_cross, position);
                        }
                    }
                    if (!scale_sensitive && !parentaffinetransform.preservesAngles()) {
                        createTextLabel(pos, counter, length, angle, remove, false);
                    } else {
                        createTextLabel(pos, counter, length, angle, remove, true);
                    }
                    arrow_gap = 8 * Inkscape::Util::Quantity::convert(line_width / doc_scale, "mm", display_unit.c_str());
                    SPCSSAttr *css = sp_repr_css_attr_new();

                    setlocale (LC_NUMERIC, "C");
                    double width_line =  atof(sp_repr_css_property(css,"stroke-width","-1"));
                    setlocale (LC_NUMERIC, locale_base);
                    if (width_line > -0.0001) {
                         arrow_gap = 8 * Inkscape::Util::Quantity::convert(width_line/ doc_scale, "mm", display_unit.c_str());
                    }
                    if(flip_side) {
                       arrow_gap *= -1;
                    }
                    if(hide_arrows) {
                        arrow_gap *= 0;
                    }
                    createLine(end, hend, Glib::ustring("infoline-on-end-"), counter, false, false);
                    if (!arrows_outside) {
                        hstart = hstart + Point::polar(angle, arrow_gap);
                        hend = hend - Point::polar(angle, arrow_gap );
                    }
                    if ((Geom::distance(hstart,hend)/2.0) > (anotation_width/1.9) + arrow_gap)  {
                        createLine(hstart, hend, Glib::ustring("infoline-"), counter, true, false, true);
                    } else {
                        createLine(hstart, hend, Glib::ustring("infoline-"), counter, true, true, true);
                    }
                } else {
                    createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-"), counter, true, true, true);
                    createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-on-start-"), counter, true, true, true);
                    createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-on-end-"), counter, true, true, true);
                    createTextLabel(Geom::Point(), counter, 0, 0, true, true);
                }
            }
        }
        if (previous_size) {
            for (size_t counter = ncurves; counter < previous_size; counter++) {
                createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-"), counter, true, true, true);
                createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-on-start-"), counter, true, true, true);
                createLine(Geom::Point(), Geom::Point(), Glib::ustring("infoline-on-end-"), counter, true, true, true);
                createTextLabel(Geom::Point(), counter, 0, 0, true, true);
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
