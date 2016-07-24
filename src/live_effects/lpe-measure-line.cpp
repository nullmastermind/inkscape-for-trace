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
#include "desktop.h"
#include "document.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

using namespace Geom;
namespace Inkscape {
namespace LivePathEffect {

LPEMeasureLine::LPEMeasureLine(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    fontselector(_("Font Selector:"), _("Font Selector"), "fontselector", &wr, this, " ")
{
    registerParameter(&fontselector);
    rtext = NULL;
    fontlister = Inkscape::FontLister::get_instance();
}

LPEMeasureLine::~LPEMeasureLine() {}

void
LPEMeasureLine::doBeforeEffect (SPLPEItem const* lpeitem)
{
     if (SP_ACTIVE_DESKTOP) {
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
        Inkscape::URI SVGElem_uri(((Glib::ustring)"#" + (Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_mlsize").c_str());
        Inkscape::URIReference* SVGElemRef = new Inkscape::URIReference(desktop->doc());
        SVGElemRef->attach(SVGElem_uri);
        SPObject *elemref = NULL;
        Inkscape::XML::Node *rtspan = NULL;
        if (elemref = SVGElemRef->getObject()) {
            rtext = elemref->getRepr();
            sp_repr_set_svg_double(rtext, "x", 0);
            sp_repr_set_svg_double(rtext, "y", 0);
        } else {
            rtext = xml_doc->createElement("svg:text");
            rtext->setAttribute("xml:space", "preserve");
            rtext->setAttribute("id", ((Glib::ustring)SP_OBJECT(lpeitem)->getId() + (Glib::ustring)"_mlsize").c_str());
            /* Set style */
            sp_repr_set_svg_double(rtext, "x", 0);
            sp_repr_set_svg_double(rtext, "y", 0);
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
        std::stringstream lenghtstr;
        lenghtstr.imbue(std::locale::classic());
        lenghtstr <<  lenght;
        Inkscape::XML::Node *rstring = NULL;
        if (!elemref) {
            rstring = xml_doc->createTextNode(lenghtstr.str().c_str());
            rtspan->addChild(rstring, NULL);
            Inkscape::GC::release(rstring);
        } else {
            rstring = rtspan->firstChild();
            rstring->setContent(lenghtstr.str().c_str());
        }
        SPObject * text_obj = NULL;
        if (!elemref) {
            text_obj = SP_OBJECT(desktop->currentLayer()->appendChildRepr(rtext));
            Inkscape::GC::release(rtext);
        } else {
            text_obj = desktop->currentLayer()->get_child_by_repr(rtext);
        }
        text_obj->updateRepr();
    }
}

Geom::PathVector
LPEMeasureLine::doEffect_path(Geom::PathVector const &path_in)
{
    lenght = Geom::distance(path_in.initialPoint(), path_in.finalPoint());
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
