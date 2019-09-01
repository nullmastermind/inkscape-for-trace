// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <set>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/pixbufformat.h>
#include <glib/gprintf.h>
#include <glibmm/i18n.h>

#include "document.h"
#include "document-undo.h"
#include "gdkpixbuf-input.h"
#include "image-resolution.h"
#include "preferences.h"
#include "selection-chemistry.h"

#include "display/cairo-utils.h"

#include "extension/input.h"
#include "extension/system.h"

#include "io/dir-util.h"

#include "object/sp-image.h"
#include "object/sp-root.h"

#include "util/units.h"

namespace Inkscape {

namespace Extension {
namespace Internal {

SPDocument *
GdkpixbufInput::open(Inkscape::Extension::Input *mod, char const *uri)
{
    // Determine whether the image should be embedded
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool ask =            prefs->getBool(  "/dialogs/import/ask");
    bool forcexdpi =      prefs->getBool(  "/dialogs/import/forcexdpi");
    Glib::ustring link  = prefs->getString("/dialogs/import/link");
    Glib::ustring scale = prefs->getString("/dialogs/import/scale");

    // If we asked about import preferences, get values and update preferences.
    if (ask) {
        ask       = !mod->get_param_bool("do_not_ask");
        forcexdpi = (strcmp(mod->get_param_optiongroup("dpi"), "from_default") == 0);
        link      =  mod->get_param_optiongroup("link");
        scale     =  mod->get_param_optiongroup("scale");

        prefs->setBool(  "/dialogs/import/ask",       ask      );
        prefs->setBool(  "/dialogs/import/forcexdpi", forcexdpi);
        prefs->setString("/dialogs/import/link",      link     );
        prefs->setString("/dialogs/import/scale",     scale    );
    }

    bool embed = (link == "embed");
 
    SPDocument *doc = nullptr;
    std::unique_ptr<Inkscape::Pixbuf> pb(Inkscape::Pixbuf::create_from_file(uri));

    // TODO: the pixbuf is created again from the base64-encoded attribute in SPImage.
    // Find a way to create the pixbuf only once.

    if (pb) {
        doc = SPDocument::createNewDoc(nullptr, TRUE, TRUE);
        bool saved = DocumentUndo::getUndoSensitive(doc);
        DocumentUndo::setUndoSensitive(doc, false); // no need to undo in this temporary document

        double width = pb->width();
        double height = pb->height();
        double defaultxdpi = prefs->getDouble("/dialogs/import/defaultxdpi/value", Inkscape::Util::Quantity::convert(1, "in", "px"));

        ImageResolution *ir = nullptr;
        double xscale = 1;
        double yscale = 1;


        if (!ir && !forcexdpi) {
            ir = new ImageResolution(uri);
        }
        if (ir && ir->ok()) {
            xscale = 960.0 / round(10.*ir->x());  // round-off to 0.1 dpi
            yscale = 960.0 / round(10.*ir->y());
            // prevent crash on image with too small dpi (bug 1479193)
            if (ir->x() <= .05)
                xscale = 960.0;
            if (ir->y() <= .05)
                yscale = 960.0;
        } else {
            xscale = 96.0 / defaultxdpi;
            yscale = 96.0 / defaultxdpi;
        }

        width *= xscale;
        height *= yscale;

        delete ir; // deleting NULL is safe

        // Create image node
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *image_node = xml_doc->createElement("svg:image");
        sp_repr_set_svg_double(image_node, "width", width);
        sp_repr_set_svg_double(image_node, "height", height);

        // Set default value as we honor "preserveAspectRatio".
        image_node->setAttribute("preserveAspectRatio", "none");

        // This is actually 'image-rendering'.
        if( scale.compare( "auto" ) != 0 ) {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property(css, "image-rendering", scale.c_str());
            sp_repr_css_set(image_node, css, "style");
            sp_repr_css_attr_unref( css );
        }

        if (embed) {
            sp_embed_image(image_node, pb.get());
        } else {
            // convert filename to uri
            gchar* _uri = g_filename_to_uri(uri, nullptr, nullptr);
            if(_uri) {
                image_node->setAttribute("xlink:href", _uri);
                g_free(_uri);
            } else {
                image_node->setAttribute("xlink:href", uri);
            }
        }

        // Add it to the current layer
        Inkscape::XML::Node *layer_node = xml_doc->createElement("svg:g");
        layer_node->setAttribute("inkscape:groupmode", "layer");
        layer_node->setAttribute("inkscape:label", "Image");
        doc->getRoot()->appendChildRepr(layer_node);
        layer_node->appendChild(image_node);
        Inkscape::GC::release(image_node);
        Inkscape::GC::release(layer_node);
        fit_canvas_to_drawing(doc);
        
        // Set viewBox if it doesn't exist
        if (!doc->getRoot()->viewBox_set) {
            // std::cerr << "Viewbox not set, setting" << std::endl;
            doc->setViewBox(Geom::Rect::from_xywh(0, 0, doc->getWidth().value(doc->getDisplayUnit()), doc->getHeight().value(doc->getDisplayUnit())));
        }
        
        // restore undo, as now this document may be shown to the user if a bitmap was opened
        DocumentUndo::setUndoSensitive(doc, saved);
    } else {
        printf("GdkPixbuf loader failed\n");
    }

    return doc;
}

#include "clear-n_.h"

void
GdkpixbufInput::init()
{
    static std::vector< Gdk::PixbufFormat > formatlist = Gdk::Pixbuf::get_formats();
    for (auto i: formatlist) {
        GdkPixbufFormat *pixformat = i.gobj();

        gchar *name =        gdk_pixbuf_format_get_name(pixformat);
        gchar *description = gdk_pixbuf_format_get_description(pixformat);
        gchar **extensions =  gdk_pixbuf_format_get_extensions(pixformat);
        gchar **mimetypes =   gdk_pixbuf_format_get_mime_types(pixformat);

        for (int i = 0; extensions[i] != nullptr; i++) {
        for (int j = 0; mimetypes[j] != nullptr; j++) {

            /* thanks but no thanks, we'll handle SVG extensions... */
            if (strcmp(extensions[i], "svg") == 0) {
                continue;
            }
            if (strcmp(extensions[i], "svgz") == 0) {
                continue;
            }
            if (strcmp(extensions[i], "svg.gz") == 0) {
                continue;
            }
            gchar *caption = g_strdup_printf(_("%s bitmap image import"), name);

            gchar *xmlString = g_strdup_printf(
                "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
                  "<name>%s</name>\n"
                  "<id>org.inkscape.input.gdkpixbuf.%s</id>\n"

                  "<param name='link' type='optiongroup' gui-text='" N_("Image Import Type:") "' gui-description='" N_("Embed results in stand-alone, larger SVG files. Link references a file outside this SVG document and all files must be moved together.") "' >\n"
                    "<option value='embed' >" N_("Embed") "</option>\n"
                    "<option value='link' >" N_("Link") "</option>\n"
                  "</param>\n"

                  "<param name='dpi' type='optiongroup' gui-text='" N_("Image DPI:") "' gui-description='" N_("Take information from file or use default bitmap import resolution as defined in the preferences.") "' >\n"
                    "<option value='from_file' >" N_("From file") "</option>\n"
                    "<option value='from_default' >" N_("Default import resolution") "</option>\n"
                  "</param>\n"

                  "<param name='scale' type='optiongroup' gui-text='" N_("Image Rendering Mode:") "' gui-description='" N_("When an image is upscaled, apply smoothing or keep blocky (pixelated). (Will not work in all browsers.)") "' >\n"
                    "<option value='auto' >" N_("None (auto)") "</option>\n"
                    "<option value='optimizeQuality' >" N_("Smooth (optimizeQuality)") "</option>\n"
                    "<option value='optimizeSpeed' >" N_("Blocky (optimizeSpeed)") "</option>\n"
                  "</param>\n"

                  "<param name=\"do_not_ask\" gui-description='" N_("Hide the dialog next time and always apply the same actions.") "' gui-text=\"" N_("Don't ask again") "\" type=\"bool\" >false</param>\n"
                  "<input>\n"
                    "<extension>.%s</extension>\n"
                    "<mimetype>%s</mimetype>\n"
                    "<filetypename>%s (*.%s)</filetypename>\n"
                    "<filetypetooltip>%s</filetypetooltip>\n"
                  "</input>\n"
                "</inkscape-extension>",
                caption,
                extensions[i],
                extensions[i],
                mimetypes[j],
                name,
                extensions[i],
                description
                );

            Inkscape::Extension::build_from_mem(xmlString, new GdkpixbufInput());
            g_free(xmlString);
            g_free(caption);
        }}

        g_free(name);
        g_free(description);
        g_strfreev(mimetypes);
        g_strfreev(extensions);
    }
}

} } }  /* namespace Inkscape, Extension, Implementation */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
