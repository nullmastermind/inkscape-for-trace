/*
 * This is the code that moves all of the SVG loading and saving into
 * the module format.  Really Inkscape is built to handle these formats
 * internally, so this is just calling those internal functions.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ted Gould <ted@gould.cx>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002-2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm.h>

#include <giomm/file.h>
#include <vector>
#include <giomm/file.h>

#include "document.h"
#include "inkscape.h"
#include "preferences.h"
#include "extension/output.h"
#include "extension/input.h"
#include "extension/system.h"
#include "file.h"
#include "svg.h"
#include "file.h"
#include "display/cairo-utils.h"
#include "extension/system.h"
#include "extension/output.h"
#include "xml/attribute-record.h"
#include "xml/simple-document.h"

#include "object/sp-image.h"
#include "object/sp-root.h"
#include "util/units.h"
#include "selection-chemistry.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace Extension {
namespace Internal {

#include "clear-n_.h"


using Inkscape::Util::List;
using Inkscape::XML::AttributeRecord;
using Inkscape::XML::Node;

/*
 * Removes all sodipodi and inkscape elements and attributes from an xml tree. 
 * used to make plain svg output.
 */
static void pruneExtendedNamespaces( Inkscape::XML::Node *repr )
{
    if (repr) {
        if ( repr->type() == Inkscape::XML::ELEMENT_NODE ) {
            std::vector<gchar const*> attrsRemoved;
            for ( List<AttributeRecord const> it = repr->attributeList(); it; ++it ) {
                const gchar* attrName = g_quark_to_string(it->key);
                if ((strncmp("inkscape:", attrName, 9) == 0) || (strncmp("sodipodi:", attrName, 9) == 0)) {
                    attrsRemoved.push_back(attrName);
                }
            }
            // Can't change the set we're iterating over while we are iterating.
            for ( std::vector<gchar const*>::iterator it = attrsRemoved.begin(); it != attrsRemoved.end(); ++it ) {
                repr->setAttribute(*it, nullptr);
            }
        }

        std::vector<Inkscape::XML::Node *> nodesRemoved;
        for ( Node *child = repr->firstChild(); child; child = child->next() ) {
            if((strncmp("inkscape:", child->name(), 9) == 0) || strncmp("sodipodi:", child->name(), 9) == 0) {
                nodesRemoved.push_back(child);
            } else {
                pruneExtendedNamespaces(child);
            }
        }
        for ( std::vector<Inkscape::XML::Node *>::iterator it = nodesRemoved.begin(); it != nodesRemoved.end(); ++it ) {
            repr->removeChild(*it);
        }
    }
}

/*
 * Similar to the above sodipodi and inkscape prune, but used on all documents
 * to remove problematic elements (for example Adobe's i:pgf tag) only removes
 * known garbage tags.
 */
static void pruneProprietaryGarbage( Inkscape::XML::Node *repr )
{
    if (repr) {
        std::vector<Inkscape::XML::Node *> nodesRemoved;
        for ( Node *child = repr->firstChild(); child; child = child->next() ) { 
            if((strncmp("i:pgf", child->name(), 5) == 0)) {
                nodesRemoved.push_back(child);
                g_warning( "An Adobe proprietary tag was found which is known to cause issues. It was removed before saving.");
            } else {
                pruneProprietaryGarbage(child);
            }
        }
        for ( std::vector<Inkscape::XML::Node *>::iterator it = nodesRemoved.begin(); it != nodesRemoved.end(); ++it ) { 
            repr->removeChild(*it);
        }
    }
}

/**
    \return   None
    \brief    What would an SVG editor be without loading/saving SVG
              files.  This function sets that up.

    For each module there is a call to Inkscape::Extension::build_from_mem
    with a rather large XML file passed in.  This is a constant string
    that describes the module.  At the end of this call a module is
    returned that is basically filled out.  The one thing that it doesn't
    have is the key function for the operation.  And that is linked at
    the end of each call.
*/
void
Svg::init()
{
    /* SVG in */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("SVG Input") "</name>\n"
            "<id>" SP_MODULE_KEY_INPUT_SVG "</id>\n"
            "<param name='import_mode_svg' type='optiongroup' appearance='full' _gui-text='" N_("SVG Image Import Type:") "' >\n"
                    "<_option value='include' >" N_("Include SVG image as editable object(s) in the current file") "</_option>\n"
                    "<_option value='embed' >" N_("Embed the SVG file in a image tag (not editable in this document)") "</_option>\n"
                    "<_option value='link' >" N_("Link the SVG file in a image tag (not editable in this document).") "</_option>\n"
                  "</param>\n"
            "<param name='svgdpi' type='float' precision='2' min='1' max='999999' gui-text='DPI for rendered SVG'>96.00</param>\n"
            "<param name='scale' appearance='minimal' type='optiongroup' _gui-text='" N_("Image Rendering Mode:") "' _gui-description='" N_("When an image is upscaled, apply smoothing or keep blocky (pixelated). (Will not work in all browsers.)") "' >\n"
                    "<_option value='auto' >" N_("None (auto)") "</_option>\n"
                    "<_option value='optimizeQuality' >" N_("Smooth (optimizeQuality)") "</_option>\n"
                    "<_option value='optimizeSpeed' >" N_("Blocky (optimizeSpeed)") "</_option>\n"
                  "</param>\n"

            "<param name=\"do_not_ask\" _gui-description='" N_("Hide the dialog next time and always apply the same actions.") "' _gui-text=\"" N_("Don't ask again") "\" type=\"boolean\" >%s</param>\n"
            "<input>\n"
                "<extension>.svg</extension>\n"
                "<mimetype>image/svg+xml</mimetype>\n"
                "<filetypename>" N_("Scalable Vector Graphic (*.svg)") "</filetypename>\n"
                "<filetypetooltip>" N_("Inkscape native file format and W3C standard") "</filetypetooltip>\n"
                "<output_extension>" SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "</output_extension>\n"
            "</input>\n"
        "</inkscape-extension>", new Svg());
    
    /* SVG out Inkscape */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("SVG Output Inkscape") "</name>\n"
            "<id>" SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "</id>\n"
            "<output>\n"
                "<extension>.svg</extension>\n"
                "<mimetype>image/x-inkscape-svg</mimetype>\n"
                "<filetypename>" N_("Inkscape SVG (*.svg)") "</filetypename>\n"
                "<filetypetooltip>" N_("SVG format with Inkscape extensions") "</filetypetooltip>\n"
                "<dataloss>false</dataloss>\n"
            "</output>\n"
        "</inkscape-extension>", new Svg());

    /* SVG out */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("SVG Output") "</name>\n"
            "<id>" SP_MODULE_KEY_OUTPUT_SVG "</id>\n"
            "<output>\n"
                "<extension>.svg</extension>\n"
                "<mimetype>image/svg+xml</mimetype>\n"
                "<filetypename>" N_("Plain SVG (*.svg)") "</filetypename>\n"
                "<filetypetooltip>" N_("Scalable Vector Graphics format as defined by the W3C") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>", new Svg());

    return;
}


/**
    \return    A new document just for you!
    \brief     This function takes in a filename of a SVG document and
               turns it into a SPDocument.
    \param     mod   Module to use
    \param     uri   The path or URI to the file (UTF-8)

    This function is really simple, it just calls sp_document_new...
    That's BS, it does all kinds of things for importing documents
    that probably should be in a separate function.

    Most of the import code was copied from gdkpixpuf-input.cpp.
*/
SPDocument *
Svg::open (Inkscape::Extension::Input *mod, const gchar *uri)
{
    // This is only used at the end... but it should go here once uri stuff is fixed.
    auto file = Gio::File::create_for_uri(uri);
    const auto path = file->get_path();

    // Fixing this means fixing a whole string of things.
    // if (path.empty()) {
    //     // We lied, the uri wasn't a uri, try as path.
    //     file = Gio::File::create_for_path(uri);
    // }

    // std::cout << "Svg::open: uri in: " << uri << std::endl;
    // std::cout << "         : uri:    " << file->get_uri() << std::endl;
    // std::cout << "         : scheme: " << file->get_uri_scheme() << std::endl;
    // std::cout << "         : path:   " << file->get_path() << std::endl;
    // std::cout << "         : parse:  " << file->get_parse_name() << std::endl;
    // std::cout << "         : base:   " << file->get_basename() << std::endl;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Get import preferences.
    bool ask_svg                   = prefs->getBool(  "/dialogs/import/ask_svg");
    Glib::ustring import_mode_svg  = prefs->getString("/dialogs/import/import_mode_svg");
    Glib::ustring scale            = prefs->getString("/dialogs/import/scale");

    // If we popped up a window asking about import preferences, get values from
    // there and update preferences.
    if(mod->get_gui() && ask_svg) {
        ask_svg         = !mod->get_param_bool("do_not_ask");
        import_mode_svg =  mod->get_param_optiongroup("import_mode_svg");
        scale           =  mod->get_param_optiongroup("scale");

        prefs->setBool(  "/dialogs/import/ask_svg",         ask_svg);
        prefs->setString("/dialogs/import/import_mode_svg", import_mode_svg );
        prefs->setString("/dialogs/import/scale",           scale );
    }

    // Do we "import" as <image>?
    if (prefs->getBool("/options/onimport", false) && import_mode_svg != "include") {
        // We import!

        // New wrapper document.
        SPDocument * doc = SPDocument::createNewDoc (nullptr, true, true);

        // Imported document
        // SPDocument * ret = SPDocument::createNewDoc(file->get_uri().c_str(), true);
        SPDocument * ret = SPDocument::createNewDoc(uri, true);

        // Create image node
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *image_node = xml_doc->createElement("svg:image");

        // Set default value as we honor "preserveAspectRatio".
        image_node->setAttribute("preserveAspectRatio", "none");

        double svgdpi = mod->get_param_float("svgdpi");
        image_node->setAttribute("inkscape:svg-dpi", Glib::ustring::format(svgdpi));

        // What is display unit doing here?
        Glib::ustring display_unit = doc->getDisplayUnit()->abbr;
        double width  = ret->getWidth().value(display_unit);
        double height = ret->getHeight().value(display_unit);
        image_node->setAttribute("width", Glib::ustring::format(width));
        image_node->setAttribute("height", Glib::ustring::format(height));

        // This is actually "image-rendering"
        Glib::ustring scale = prefs->getString("/dialogs/import/scale");
        if( scale != "auto") {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property(css, "image-rendering", scale.c_str());
            sp_repr_css_set(image_node, css, "style");
            sp_repr_css_attr_unref( css );
        }

        // Do we embed or link?
        if (import_mode_svg == "embed") {
            std::unique_ptr<Inkscape::Pixbuf> pb(Inkscape::Pixbuf::create_from_file(uri, svgdpi));
            if(pb) {
                sp_embed_svg(image_node, uri);
            }
        } else {
            // Convert filename to uri (why do we need to do this, we claimed it was already a uri).
            gchar* _uri = g_filename_to_uri(uri, nullptr, nullptr);
            if(_uri) {
                // if (strcmp(_uri, uri) != 0) {
                //     std::cout << "Svg::open: _uri != uri! " << _uri << ":" << uri << std::endl;
                // }
                image_node->setAttribute("xlink:href", _uri);
                g_free(_uri);
            } else {
                image_node->setAttribute("xlink:href", uri);
            }
        }

        // Add the image to a layer.
        Inkscape::XML::Node *layer_node = xml_doc->createElement("svg:g");
        layer_node->setAttribute("inkscape:groupmode", "layer");
        layer_node->setAttribute("inkscape:label", "Image");
        doc->getRoot()->appendChildRepr(layer_node);
        layer_node->appendChild(image_node);
        Inkscape::GC::release(image_node);
        Inkscape::GC::release(layer_node);
        fit_canvas_to_drawing(doc);

        // Set viewBox if it doesn't exist. What is display unit doing here?
        if (!doc->getRoot()->viewBox_set) {
            doc->setViewBox(Geom::Rect::from_xywh(0, 0, doc->getWidth().value(doc->getDisplayUnit()), doc->getHeight().value(doc->getDisplayUnit())));
        }
        return doc;
    }

    // We are not importing as <image>. Open as new document.

    // Try to open non-local file (when does this occur?).
    if (!file->get_uri_scheme().empty()) {
        if (path.empty()) {
            try {
                char *contents;
                gsize length;
                file->load_contents(contents, length);
                return SPDocument::createNewDocFromMem(contents, length, 1);
            } catch (Gio::Error &e) {
                g_warning("Could not load contents of non-local URI %s\n", uri);
                return nullptr;
            }
        } else {
            // Do we ever get here and does this actually work?
            uri = path.c_str();
        }
    }

    SPDocument *doc = SPDocument::createNewDoc(uri, true);
    // SPDocument *doc = SPDocument::createNewDoc(file->get_uri().c_str(), true);
    return doc;
}

/**
    \return    None
    \brief     This is the function that does all of the SVG saves in
               Inkscape.  It detects whether it should do a Inkscape
               namespace save internally.
    \param     mod   Extension to use.
    \param     doc   Document to save.
    \param     uri   The filename to save the file to.

    This function first checks its parameters, and makes sure that
    we're getting good data.  It also checks the module ID of the
    incoming module to figure out whether this save should include
    the Inkscape namespace stuff or not.  The result of that comparison
    is stored in the exportExtensions variable.

    If there is not to be Inkscape name spaces a new document is created
    without.  (I think, I'm not sure on this code)

    All of the internally referenced imageins are also set to relative
    paths in the file.  And the file is saved.

    This really needs to be fleshed out more, but I don't quite understand
    all of this code.  I just stole it.
*/
void
Svg::save(Inkscape::Extension::Output *mod, SPDocument *doc, gchar const *filename)
{
    g_return_if_fail(doc != nullptr);
    g_return_if_fail(filename != nullptr);
    Inkscape::XML::Document *rdoc = doc->rdoc;

    bool const exportExtensions = ( !mod->get_id()
      || !strcmp (mod->get_id(), SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE)
      || !strcmp (mod->get_id(), SP_MODULE_KEY_OUTPUT_SVGZ_INKSCAPE));

    // We prune the in-use document and deliberately loose data, because there
    // is no known use for this data at the present time.
    pruneProprietaryGarbage(rdoc->root());

    if (!exportExtensions) {
        // We make a duplicate document so we don't prune the in-use document
        // and loose data. Perhaps the user intends to save as inkscape-svg next.
        Inkscape::XML::Document *new_rdoc = new Inkscape::XML::SimpleDocument();

        // Comments and PI nodes are not included in this duplication
        // TODO: Move this code into xml/document.h and duplicate rdoc instead of root. 
        new_rdoc->setAttribute("version", "1.0");
        new_rdoc->setAttribute("standalone", "no");

        // Get a new xml repr for the svg root node
        Inkscape::XML::Node *root = rdoc->root()->duplicate(new_rdoc);

        // Add the duplicated svg node as the document's rdoc
        new_rdoc->appendChild(root);
        Inkscape::GC::release(root);

        pruneExtendedNamespaces(root);
        rdoc = new_rdoc;
    }

    if (!sp_repr_save_rebased_file(rdoc, filename, SP_SVG_NS_URI,
                                   doc->getBase(), //
                                   m_detachbase ? nullptr : filename)) {
        throw Inkscape::Extension::Output::save_failed();
    }

    if (!exportExtensions) {
        Inkscape::GC::release(rdoc);
    }

    return;
}

} } }  /* namespace inkscape, module, implementation */

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
