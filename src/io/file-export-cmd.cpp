// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File export from the command line. This code, greatly modified, use to be in main.cpp. It should
 * be replaced by code shared with the file dialog (Gio::Actions?).
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * Git blame shows that bulia byak is the main author of the original export code from
 * main.cpp. Other authors of note include Nicolas Dufour, Vinicius dos Santos Oliveira, and Bob
 * Jamison; none of whom bothered to add their names to the copyright of main.cc.
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "file-export-cmd.h"

#include <boost/algorithm/string.hpp>
#include <png.h> // PNG export

#include "document.h"
#include "object/object-set.h"
#include "object/sp-item.h"
#include "object/sp-root.h"
#include "object/sp-text.h"
#include "object/sp-flowtext.h"
#include "object/sp-namedview.h"
#include "object/sp-object-group.h"
#include "path-chemistry.h" // sp_item_list_to_curves
#include "text-editing.h" // te_update_layout_now_recursive
#include "selection-chemistry.h" // fit_canvas_to_drawing
#include "svg/svg-color.h" // Background color
#include "helper/png-write.h" // PNG Export

#include "extension/extension.h"
#include "extension/system.h"
#include "extension/db.h"
#include "extension/output.h"
#include "extension/init.h"


// Temporary dependency : once all compilers we want to support have support for
// C++17 std::filesystem (with #include <filesystem> ) then we drop this dep
// (Dev meeting, 2020-09-25)

#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;

InkFileExportCmd::InkFileExportCmd()
    : export_overwrite(false)
    , export_area_drawing(false)
    , export_area_page(false)
    , export_margin(0)
    , export_area_snap(false)
    , export_use_hints(false)
    , export_width(0)
    , export_height(0)
    , export_dpi(0)
    , export_ignore_filters(false)
    , export_text_to_path(false)
    , export_ps_level(3)
    , export_pdf_level("1.5")
    , export_latex(false)
    , export_id_only(false)
    , export_background_opacity(-1) // default is unset != actively set to 0
    , export_plain_svg(false)
{
}

void
InkFileExportCmd::do_export(SPDocument* doc, std::string filename_in)
{
    std::string export_type_filename;
    std::vector<Glib::ustring> export_type_list;

    // Get export type from filename supplied with --export-filename
    if (!export_filename.empty() && export_filename != "-") {
        auto fn = filesystem::path(export_filename);
        if (!fn.has_extension()) {
            if (export_type.empty() && export_extension.empty()) {
                std::cerr << "InkFileExportCmd::do_export: No export type specified. "
                          << "Append a supported file extension to filename provided with --export-filename or "
                          << "provide one or more extensions separately using --export-type" << std::endl;
                return;
            } else {
                // no extension is fine if --export-type is given
                // explicitly stated extensions are handled later
            }
        } else {
            export_type_filename = fn.extension().string().substr(1);
            boost::algorithm::to_lower(export_type_filename);
            export_filename = (fn.parent_path() / fn.stem()).string();
        }
    }

    // Get export type(s) from string supplied with --export-type
    if (!export_type.empty()) {
        export_type_list = Glib::Regex::split_simple("[,;]", export_type);
    }

    // Determine actual type(s) for export.
    if (export_use_hints) {
        // Override type if --export-use-hints is used (hints presume PNG export for now)
        // TODO: There's actually no reason to presume. We could allow to export to any format using hints!
        if (export_id.empty() && !export_area_drawing) {
            std::cerr << "InkFileExportCmd::do_export: "
                      << "--export-use-hints can only be used with --export-id or --export-area-drawing." << std::endl;
            return;
        }
        if (export_type_list.size() > 1 || (export_type_list.size() == 1 && export_type_list[0] != "png")) {
            std::cerr << "InkFileExportCmd::do_export: --export-use-hints can only be used with PNG export! "
                      << "Ignoring --export-type=" << export_type << "." << std::endl;
        }
        if (!export_filename.empty()) {
            std::cerr << "InkFileExportCmd::do_export: --export-filename is ignored when using --export-use-hints!" << std::endl;
        }
        export_type_list.clear();
        export_type_list.emplace_back("png");
    } else if (export_type_list.empty()) {
        if (!export_type_filename.empty()) {
            export_type_list.emplace_back(export_type_filename); // use extension from filename
        } else if (!export_extension.empty()) {
            // guess export type from extension
            auto ext =
                dynamic_cast<Inkscape::Extension::Output *>(Inkscape::Extension::db.get(export_extension.data()));
            if (ext) {
                export_type_list.emplace_back(std::string(ext->get_extension()).substr(1));
            } else {
                std::cerr << "InkFileExportCmd::do_export: "
                          << "The supplied --export-extension was not found. Specify a file extension "
                          << "to get a list of available extensions for this file type.";
                return;
            }
        } else {
            export_type_list.emplace_back("svg"); // fall-back to SVG by default
        }
    }
    // check if multiple export files are requested, but --export_extension was supplied
    if (!export_extension.empty() && export_type_list.size() != 1) {
        std::cerr
            << "InkFileExportCmd::do_export: You may only specify one export type if --export-extension is supplied";
        return;
    }
    Inkscape::Extension::DB::OutputList extension_list;
    Inkscape::Extension::db.get_output_list(extension_list);

    for (auto const &Type : export_type_list) {
        // use lowercase type for following comparisons
        auto type = Type.lowercase();
        g_info("exporting '%s' to type '%s'", filename_in.c_str(), type.c_str());

        export_type_current = type;

        // Check for consistency between extension of --export-filename and --export-type if both are given
        if (!export_type_filename.empty() && (type != export_type_filename)) {
            std::cerr << "InkFileExportCmd::do_export: "
                      << "Ignoring extension of export filename (" << export_type_filename << ") "
                      << "as it does not match the current export type (" << type << ")." << std::endl;
        }
        bool export_extension_forced = !export_extension.empty();
        // For PNG export, there is no extension, so the method below can not be used.
        if (type == "png") {
            if (!export_extension_forced) {
                do_export_png(doc, filename_in);
            } else {
                std::cerr << "InkFileExportCmd::do_export: "
                          << "The parameter --export-extension is invalid for PNG export" << std::endl;
            }
            continue;
        }
        // for SVG export, we let the do_export_svg function handle the selection of the extension, unless
        // an extension ID was explicitly given. This makes handling of --export-plain-svg easier (which
        // should also work when multiple file types are given, unlike --export-extension)
        if (type == "svg" && !export_extension_forced) {
            do_export_svg(doc, filename_in);
            continue;
        }

        bool extension_for_fn_exists = false;
        bool exported = false;
        // if no extension is found, the entire list of extensions is walked through,
        // so we can use the same loop to construct the list of available formats for the error messsage
        std::list<std::string> filetypes({".svg", ".png", ".ps", ".eps", ".pdf"});
        std::list<std::string> exts_for_fn;
        for (auto oext : extension_list) {
            if (oext->deactivated()) {
                continue;
            }
            auto name = Glib::ustring(oext->get_extension()).lowercase();
            filetypes.emplace_back(name);
            if (name == "." + type) {
                extension_for_fn_exists = true;
                exts_for_fn.emplace_back(oext->get_id());
                if (!export_extension_forced ||
                    (export_extension_forced && export_extension == Glib::ustring(oext->get_id()).lowercase())) {
                    if (type == "svg") {
                        do_export_svg(doc, filename_in, *oext);
                    } else if (type == "ps") {
                        do_export_ps_pdf(doc, filename_in, "image/x-postscript", *oext);
                    } else if (type == "eps") {
                        do_export_ps_pdf(doc, filename_in, "image/x-e-postscript", *oext);
                    } else if (type == "pdf") {
                        do_export_ps_pdf(doc, filename_in, "application/pdf", *oext);
                    } else {
                        do_export_extension(doc, filename_in, oext);
                    }
                    exported = true;
                    break;
                }
            }
        }
        if (!exported) {
            if (export_extension_forced && extension_for_fn_exists) {
                // the located extension for this file type did not match the provided --export-extension parameter
                std::cerr << "InkFileExportCmd::do_export: "
                          << "The supplied extension ID (" << export_extension
                          << ") does not match any of the extensions "
                          << "available for this file type." << std::endl
                          << "Supported IDs for this file type: [";
                copy(exts_for_fn.begin(), exts_for_fn.end(), std::ostream_iterator<std::string>(std::cerr, ", "));
                std::cerr << "\b\b]" << std::endl;
            } else {
                std::cerr << "InkFileExportCmd::do_export: Unknown export type: " << type << ". Allowed values: [";
                filetypes.sort();
                filetypes.unique();
                copy(filetypes.begin(), filetypes.end(), std::ostream_iterator<std::string>(std::cerr, ", "));
                std::cerr << "\b\b]" << std::endl;
            }
        }
    }
}

// File names use std::string. HTML5 and presumably SVG 2 allows UTF-8 characters. Do we need to convert "object_id" here?
std::string
InkFileExportCmd::get_filename_out(std::string filename_in, std::string object_id)
{
    // Pipe out
    if (export_filename == "-") {
        return "-";
    }

    auto const export_type_current_native = Glib::filename_from_utf8(export_type_current);

    // Use filename provided with --export-filename if given (and append proper extension).
    if (!export_filename.empty()) {
        return export_filename + "." + export_type_current_native;
    }

    // Check for pipe
    if (filename_in == "-") {
        return "-";
    }

    // Construct output filename from input filename and export_type.
    auto extension_pos = filename_in.find_last_of('.');
    if (extension_pos == std::string::npos) {
        std::cerr << "InkFileExportCmd::get_filename_out: cannot determine input file type from filename extension: " << filename_in << std::endl;
        return (std::string());
    }

    std::string extension = filename_in.substr(extension_pos+1);
    if (export_overwrite && export_type_current_native == extension) {
        return filename_in;
    } else {
        std::string tag;
        if (export_type_current_native == extension) {
            tag = "_out";
        }
        if (!object_id.empty()) {
            tag = "_" + object_id;
        }
        return filename_in.substr(0, extension_pos) + tag + "." + export_type_current_native;
    }

    // We need a valid file name to write to unless we're using PNG export hints.
    // if (!(export_type == "png" && export_use_hints)) {

    //     // Check for file name.
    //     if (filename_out.empty()) {
    //         std::cerr << "InkFileExportCmd::do_export: Could not determine output file name!" << std::endl;
    //         return (std::string());
    //     }

    //     // Check if directory exists.
    //     std::string directory = Glib::path_get_dirname(filename_out);
    //     if (!Glib::file_test(directory, Glib::FILE_TEST_IS_DIR)) {
    //         std::cerr << "InkFileExportCmd::do_export: File path includes directory that does not exist! " << directory << std::endl;
    //         return (std::string());
    //     }
    // }
}
/**
 *  Perform an SVG export
 *
 *  \param doc Document to export.
 *  \param filename_in Filename for export
 */
int InkFileExportCmd::do_export_svg(SPDocument *doc, std::string const &filename_in)
{
    Inkscape::Extension::Output *oext;
    if (export_plain_svg) {
        oext =
            dynamic_cast<Inkscape::Extension::Output *>(Inkscape::Extension::db.get("org.inkscape.output.svg.plain"));
    } else {
        oext = dynamic_cast<Inkscape::Extension::Output *>(
            Inkscape::Extension::db.get("org.inkscape.output.svg.inkscape"));
    }
    return do_export_svg(doc, filename_in, *oext);
}
/**
 *  Perform an SVG export
 *
 *  \param doc Document to export.
 *  \param filename_in Filename for export
 *  \param extension Output extension used for exporting
 */
int InkFileExportCmd::do_export_svg(SPDocument *doc, std::string const &filename_in,
                                    Inkscape::Extension::Output &extension)
{
    // Start with options that are once per document.
    if (export_text_to_path) {
        std::vector<SPItem*> items;
        SPRoot *root = doc->getRoot();
        doc->ensureUpToDate();
        for (auto& iter: root->children) {
            SPItem* item = (SPItem*) &iter;
            if (! (SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item) || SP_IS_GROUP(item))) {
                continue;
            }

            te_update_layout_now_recursive(item);
            items.push_back(item);
        }

        std::vector<SPItem*> selected;  // Not used
        std::vector<Inkscape::XML::Node*> to_select;  // Not used

        sp_item_list_to_curves(items, selected, to_select);
    }

    if (export_margin != 0) {
        gdouble margin = export_margin;
        doc->ensureUpToDate();
        SPNamedView *nv;
        Inkscape::XML::Node *nv_repr;
        if ((nv = sp_document_namedview(doc, nullptr)) && (nv_repr = nv->getRepr())) {
            sp_repr_set_svg_double(nv_repr, "fit-margin-top", margin);
            sp_repr_set_svg_double(nv_repr, "fit-margin-left", margin);
            sp_repr_set_svg_double(nv_repr, "fit-margin-right", margin);
            sp_repr_set_svg_double(nv_repr, "fit-margin-bottom", margin);
        }
    }

    if (export_area_drawing) {
        fit_canvas_to_drawing(doc, export_margin != 0 ? true : false);
    } else if (export_area_page || export_id.empty() ) {
        if (export_margin) {
            doc->ensureUpToDate();
            doc->fitToRect(*(doc->preferredBounds()), true);
        }
    }


    // Export each object in list (or root if empty).  Use ';' so in future it could be possible to selected multiple objects to export together.
    std::vector<Glib::ustring> objects = Glib::Regex::split_simple("\\s*;\\s*", export_id);
    if (objects.empty()) {
        objects.emplace_back(); // So we do loop at least once for root.
    }

    for (auto object : objects) {

        std::string filename_out = get_filename_out(filename_in, Glib::filename_from_utf8(object));
        if (filename_out.empty()) {
            return 1;
        }

        if(!object.empty()) {
            doc->ensureUpToDate();

            // "crop" the document to the specified object, cleaning as we go.
            SPObject *obj = doc->getObjectById(object);
            if (obj == nullptr) {
                std::cerr << "InkFileExportCmd::do_export_svg: Object " << object << " not found in document, nothing to export." << std::endl;
                return 1;
            }
            if (export_id_only) {
                // If -j then remove all other objects to complete the "crop"
                doc->getRoot()->cropToObject(obj);
            }
            if (!(export_area_page || export_area_drawing)) {
                Inkscape::ObjectSet s(doc);
                s.set(obj);
                s.fitCanvas(export_margin ? true : false);
            }
        }
        g_assert(std::string(extension.get_extension()) == ".svg");
        try {
            Inkscape::Extension::save(dynamic_cast<Inkscape::Extension::Extension *>(&extension), doc,
                                      filename_out.c_str(), false, false, false,
                                      export_plain_svg ? Inkscape::Extension::FILE_SAVE_METHOD_SAVE_COPY
                                                       : Inkscape::Extension::FILE_SAVE_METHOD_INKSCAPE_SVG);
        } catch (Inkscape::Extension::Output::save_failed &e) {
            std::cerr << "InkFileExportCmd::do_export_svg: Failed to save " << (export_plain_svg ? "" : "Inkscape")
                      << " SVG to: " << filename_out << std::endl;
            return 1;
        }
    }
    return 0;
}

guint32 InkFileExportCmd::get_bgcolor(SPDocument *doc) {
    guint32 bgcolor = 0x00000000;
    if (!export_background.empty()) {
        // override the page color
        bgcolor = sp_svg_read_color(export_background.c_str(), 0xffffff00);
        // default is opaque if a color is given on commandline
        if (export_background_opacity < -.5 ) {
            export_background_opacity = 255;
        }
    } else {
        // read from namedview
        Inkscape::XML::Node *nv = doc->getReprNamedView();
        if (nv && nv->attribute("pagecolor")){
            bgcolor = sp_svg_read_color(nv->attribute("pagecolor"), 0xffffff00);
        }
    }

    if (export_background_opacity > -.5) { // if the value is manually set
        if (export_background_opacity > 1.0) {
            float value = CLAMP (export_background_opacity, 1.0f, 255.0f);
            bgcolor |= (guint32) floor(value);
        } else {
            float value = CLAMP (export_background_opacity, 0.0f, 1.0f);
            bgcolor |= SP_COLOR_F_TO_U(value);
        }
    } else {
        Inkscape::XML::Node *nv = doc->getReprNamedView();
        if (nv && nv->attribute("inkscape:pageopacity")){
            double opacity = 1.0;
            sp_repr_get_double (nv, "inkscape:pageopacity", &opacity);
            bgcolor |= SP_COLOR_F_TO_U(opacity);
        } // else it's transparent
    }
    return bgcolor;
}

/**
 *  Perform a PNG export
 *
 *  \param doc Document to export.
 *  \param filename_in filename for export
 */
int
InkFileExportCmd::do_export_png(SPDocument *doc, std::string const &filename_in)
{
    bool filename_from_hint = false;
    gdouble dpi = 0.0;
    guint32 bgcolor = get_bgcolor(doc);

    // Export each object in list (or root if empty).  Use ';' so in future it could be possible to selected multiple objects to export together.
    std::vector<Glib::ustring> objects = Glib::Regex::split_simple("\\s*;\\s*", export_id);
    if (objects.empty()) {
        objects.emplace_back(); // So we do loop at least once for root.
    }

    for (auto object_id : objects) {

        std::string filename_out = get_filename_out(filename_in, Glib::filename_from_utf8(object_id));

        std::vector<SPItem*> items;

        // Find export object. (Either root or object with specified id.)
        SPObject *object = doc->getRoot();
        if (!object_id.empty()) {
            object = doc->getObjectById(object_id);
        }

        if (!object) {
            std::cerr << "InkFileExport::do_export_png: "
                      << "Object with id=\"" << object_id
                      << "\" was not found in the document. Skipping." << std::endl;
            continue;
        }

        if (!SP_IS_ITEM (object)) {
            std::cerr << "InkFileExportCmd::do_export_png: "
                      << "Object with id=\"" << object_id
                      << "\" is not a visible item. Skipping." << std::endl;
            continue;
        }

        items.push_back(SP_ITEM(object)); // There is only one item, why do this?

        if (export_id_only) {
            std::cerr << "Exporting only object with id=\""
                      << object_id << "\"; all other objects hidden." << std::endl;
        }

        // Find file name and dpi from hints.
        if (export_use_hints) {

            // Retrieve export filename hint.
            const gchar *fn_hint = object->getRepr()->attribute("inkscape:export-filename");
            if (fn_hint) {
                filename_out = fn_hint;
                filename_from_hint = true;
            } else {
                std::cerr << "InkFileExport::do_export_png: "
                          << "Export filename hint not found for object " << object_id << ". Skipping." << std::endl;
                continue;
            }

            // Retrieve export dpi hint. Only xdpi as ydpi is always the same now.
            const gchar *dpi_hint = object->getRepr()->attribute("inkscape:export-xdpi");
            if (dpi_hint) {
                if (export_dpi || export_width || export_height) {
                    std::cerr << "InkFileExport::do_export_png: "
                              << "Using bitmap dimensions from the command line "
                              << "(--export-dpi, --export-width, or --export-height). "
                              << "DPI hint " << dpi_hint << " is ignored." << std::endl;
                } else {
                    dpi = atof(dpi_hint);
                }
            } else {
                std::cerr << "InkFileExport::do_export_png: "
                          << "Export DPI hint not found for the object." << std::endl;
            }
        }

        // ------------------------- File name -------------------------

        // Check we have a filename.
        if (filename_out.empty()) {
            std::cerr << "InkFileExport::do_export_png: "
                      << "No valid export filename given and no filename hint. Skipping." << std::endl;
            continue;
        }

        if (filename_from_hint) {
            //Make relative paths go from the document location, if possible:
            if (!Glib::path_is_absolute(filename_out) && doc->getDocumentURI()) {
                std::string dirname = Glib::path_get_dirname(doc->getDocumentURI());
                if (!dirname.empty()) {
                    filename_out = Glib::build_filename(dirname, filename_out);
                }
            }
        }

        // Check if directory exists
        std::string directory = Glib::path_get_dirname(filename_out);
        if (!Glib::file_test(directory, Glib::FILE_TEST_IS_DIR)) {
            std::cerr << "File path " << filename_out << " includes directory that doesn't exist. Skipping." << std::endl;
            continue;
        }

        // -------------------------- DPI -------------------------------

        if (export_dpi != 0.0 && dpi == 0.0) {
            dpi = export_dpi;
            if ((dpi < 0.1) || (dpi > 10000.0)) {
                std::cerr << "InkFileExport::do_export_png: "
                          << "DPI value " << export_dpi
                          << " out of range [0.1 - 10000.0]. Skipping.";
                continue;
            }
        }

        // default dpi
        if (dpi == 0.0) {
            dpi = Inkscape::Util::Quantity::convert(1, "in", "px");
        }

        // -------------------------  Area -------------------------------

        Geom::Rect area;
        doc->ensureUpToDate();

        // Three choices: 1. Command-line export_area  2. Page area  3. Drawing area
        if (!export_area.empty()) {

            // Export area command-line

            /* Try to parse area (given in SVG pixels) */
            gdouble x0,y0,x1,y1;
            if (sscanf(export_area.c_str(), "%lg:%lg:%lg:%lg", &x0, &y0, &x1, &y1) != 4) {
                g_warning("Cannot parse export area '%s'; use 'x0:y0:x1:y1'. Nothing exported.", export_area.c_str());
                return 1; // If it fails once, it will fail for all objects.
            }
            area = Geom::Rect(Geom::Interval(x0,x1), Geom::Interval(y0,y1));

        } else if (export_area_page || (!export_area_drawing && object_id.empty())) {

            // Export area page (explicit or if no object is given).
            Geom::Point origin(doc->getRoot()->x.computed, doc->getRoot()->y.computed);
            area = Geom::Rect(origin, origin + doc->getDimensions());

        } else {

            // Export area drawing (explicit or if object is given).
            Geom::OptRect areaMaybe = static_cast<SPItem *>(object)->documentVisualBounds();
            if (areaMaybe) {
                area = *areaMaybe;
            } else {
                std::cerr << "InkFileExport::do_export_png: "
                          << "Unable to determine a valid bounding box. Skipping." << std::endl;
                continue;
            }
        }

        if (export_area_snap) {
            area = area.roundOutwards();
        }
        // End finding area.

        // -------------------------- Width and Height ---------------------------------

        unsigned long int width = 0;
        unsigned long int height = 0;
        double xdpi = dpi;
        double ydpi = dpi;

        if (export_height != 0) {
            height = export_height;
            if ((height < 1) || (height > PNG_UINT_31_MAX)) {
                std::cerr << "InkFileExport::do_export_png: "
                          << "Export height " << height << " out of range (1 to " << PNG_UINT_31_MAX << ")" << std::endl;
                continue;
            }
            ydpi = Inkscape::Util::Quantity::convert(height, "in", "px") / area.height();
            xdpi = ydpi;
            dpi = ydpi;
        }

        if (export_width != 0) {
            width = export_width;
            if ((width < 1) || (width > PNG_UINT_31_MAX)) {
                std::cerr << "InkFileExport::do_export_png: "
                          << "Export width " << width << " out of range (1 to " << PNG_UINT_31_MAX << ")." << std::endl;
                continue;
            }
            xdpi = Inkscape::Util::Quantity::convert(width, "in", "px") / area.width();
            ydpi = export_height ? ydpi : xdpi;
            dpi = xdpi;
        }

        if (width == 0) {
            width = (unsigned long int) (Inkscape::Util::Quantity::convert(area.width(), "px", "in") * dpi + 0.5);
        }

        if (height == 0) {
            height = (unsigned long int) (Inkscape::Util::Quantity::convert(area.height(), "px", "in") * dpi + 0.5);
        }

        if ((width < 1) || (height < 1) || (width > PNG_UINT_31_MAX) || (height > PNG_UINT_31_MAX)) {
            std::cerr << "InkFileExport::do_export_png: Dimensions " << width << "x" << height << " are out of range (1 to " << PNG_UINT_31_MAX << ")." << std::endl;
            continue;
        }

        // -------------------------- Bit Depth and Color Type --------------------

        int bit_depth = 8; // default of sp_export_png_file function
        int color_type = PNG_COLOR_TYPE_RGB_ALPHA; // default of sp_export_png_file function

        if (!export_png_color_mode.empty()) {
            // data as in ui/dialog/export.cpp:
            const std::map<std::string, std::pair<int, int>> color_modes = {
                {"Gray_1", {PNG_COLOR_TYPE_GRAY, 1}},
                {"Gray_2", {PNG_COLOR_TYPE_GRAY, 2}},
                {"Gray_4", {PNG_COLOR_TYPE_GRAY, 4}},
                {"Gray_8", {PNG_COLOR_TYPE_GRAY, 8}},
                {"Gray_16", {PNG_COLOR_TYPE_GRAY, 16}},
                {"RGB_8", {PNG_COLOR_TYPE_RGB, 8}},
                {"RGB_16", {PNG_COLOR_TYPE_RGB, 16}},
                {"GrayAlpha_8", {PNG_COLOR_TYPE_GRAY_ALPHA, 8}},
                {"GrayAlpha_16", {PNG_COLOR_TYPE_GRAY_ALPHA, 16}},
                {"RGBA_8", {PNG_COLOR_TYPE_RGB_ALPHA, 8}},
                {"RGBA_16", {PNG_COLOR_TYPE_RGB_ALPHA, 16}},
            };
            auto it = color_modes.find(export_png_color_mode);
            if (it == color_modes.end()) {
                std::cerr << "InkFileExport::do_export_png: "
                          << "Color mode " << export_png_color_mode << " is invalid. It must be one of Gray_1/Gray_2/Gray_4/Gray_8/Gray_16/RGB_8/RGB_16/GrayAlpha_8/GrayAlpha_16/RGBA_8/RGBA_16." << std::endl;
                continue;
            } else {
                std::tie(color_type, bit_depth) = it->second;
            }
        }

        // ----------------------  Generate the PNG -------------------------------

        // Do we really need to print this?
        std::cerr << "Background RRGGBBAA: " << std::hex << bgcolor << std::dec << std::endl;
        std::cerr << "Area "
                  << area[Geom::X][0] << ":" << area[Geom::Y][0] << ":"
                  << area[Geom::X][1] << ":" << area[Geom::Y][1] << " exported to "
                  << width << " x " << height << " pixels (" << dpi << " dpi)" << std::endl;

        reverse(items.begin(),items.end()); // But there was only one item!

        if( sp_export_png_file(doc, filename_out.c_str(), area, width, height, xdpi, ydpi,
                               bgcolor, nullptr, nullptr, true, export_id_only ? items : std::vector<SPItem*>(),
                               false, color_type, bit_depth) == 1 ) {
        } else {
            std::cerr << "InkFileExport::do_export_png: Failed to export to " << filename_out << std::endl;
            continue;
        }

    } // End loop over objects.
    return 0;
}


/**
 *  Perform a PDF/PS/EPS export
 *
 *  \param doc Document to export.
 *  \param filename File to write to.
 *  \param mime MIME type to export as.
 */
int
InkFileExportCmd::do_export_ps_pdf(SPDocument* doc, std::string const &filename_in, std::string mime_type)
{
    // Check if we support mime type.
    Inkscape::Extension::DB::OutputList o;
    Inkscape::Extension::db.get_output_list(o);
    Inkscape::Extension::DB::OutputList::const_iterator i = o.begin();
    while (i != o.end() && strcmp( (*i)->get_mimetype(), mime_type.c_str() ) != 0) {
        ++i;
    }

    if (i == o.end()) {
        std::cerr << "InkFileExportCmd::do_export_ps_pdf: Could not find an extension to export to MIME type: " << mime_type << std::endl;
        return 1;
    }
    return do_export_ps_pdf(doc, filename_in, mime_type, *dynamic_cast<Inkscape::Extension::Output *>(*i));
}

/**
 *  Perform a PDF/PS/EPS export
 *
 *  \param doc Document to export.
 *  \param filename File to write to.
 *  \param mime MIME type to export as.
 *  \param Extension used for exporting
 */
int InkFileExportCmd::do_export_ps_pdf(SPDocument *doc, std::string const &filename_in, std::string mime_type,
                                       Inkscape::Extension::Output &extension)
{
    // check if the passed extension conforms to the mime type.
    assert(std::string(extension.get_mimetype()) == mime_type);
    // Start with options that are once per document.

    // Set export options.
    if (export_text_to_path) {
        extension.set_param_optiongroup("textToPath", "paths");
    } else if (export_latex) {
        extension.set_param_optiongroup("textToPath", "LaTeX");
    } else {
        extension.set_param_optiongroup("textToPath", "embed");
    }

    if (export_ignore_filters) {
        extension.set_param_bool("blurToBitmap", false);
    } else {
        extension.set_param_bool("blurToBitmap", true);

        gdouble dpi = 96.0;
        if (export_dpi) {
            dpi = export_dpi;
            if ((dpi < 1) || (dpi > 10000.0)) {
                g_warning("DPI value %lf out of range [1 - 10000]. Using 96 dpi instead.", export_dpi);
                dpi = 96;
            }
        }

        extension.set_param_int("resolution", (int)dpi);
    }

    extension.set_param_float("bleed", export_margin);

    // handle --export-pdf-version
    if (mime_type == "application/pdf") {
        bool set_export_pdf_version_fail = true;
        const gchar *pdfver_param_name = "PDFversion";
        if (!export_pdf_level.empty()) {
            // combine "PDF " and the given command line
            std::string version_gui_string = std::string("PDF-") + export_pdf_level.raw();
            try {
                // first, check if the given pdf version is selectable in the ComboBox
                if (extension.get_param_optiongroup_contains("PDFversion", version_gui_string.c_str())) {
                    extension.set_param_optiongroup(pdfver_param_name, version_gui_string.c_str());
                    set_export_pdf_version_fail = false;
                } else {
                    g_warning("Desired PDF export version \"%s\" not supported! Hint: input one of the versions found in the pdf export dialog e.g. \"1.4\".",
                              export_pdf_level.c_str());
                }
            } catch (...) {
                // can be thrown along the way:
                // throw Extension::param_not_exist();
                // throw Extension::param_not_enum_param();
                g_warning("Parameter or Enum \"%s\" might not exist", pdfver_param_name);
            }
        }

        // set default pdf export version to 1.4, also if something went wrong
        if(set_export_pdf_version_fail) {
            extension.set_param_optiongroup(pdfver_param_name, "PDF-1.4");
        }
    }

    if (mime_type == "image/x-postscript" || mime_type == "image/x-e-postscript") {
        if ( export_ps_level < 2 || export_ps_level > 3 ) {
            g_warning("Only supported PostScript levels are 2 and 3."
                      " Defaulting to 2.");
            export_ps_level = 2;
        }

        extension.set_param_optiongroup("PSlevel", (export_ps_level == 3) ? "PS3" : "PS2");
    }


    // Export each object in list (or root if empty).  Use ';' so in future it could be possible to selected multiple objects to export together.
    std::vector<Glib::ustring> objects = Glib::Regex::split_simple("\\s*;\\s*", export_id);
    if (objects.empty()) {
        objects.emplace_back(); // So we do loop at least once for root.
    }

    for (auto object : objects) {

        std::string filename_out = get_filename_out(filename_in, Glib::filename_from_utf8(object));
        if (filename_out.empty()) {
            return 1;
        }

        // Export only object with given id.
        if (!object.empty()) {
            SPObject *o = doc->getObjectById(object);
            if (o == nullptr) {
                std::cerr << "InkFileExportCmd::do_export_ps_pdf: Object " << object << " not found in document, nothing to export." << std::endl;
                return 1;
            }
            extension.set_param_string("exportId", object.c_str());
        } else {
            extension.set_param_string("exportId", "");
        }

        // Set export area.
        if (export_area_page && export_area_drawing) {
            std::cerr << "You cannot use --export-area-page and --export-area-drawing at the same time; only the former will take effect." << std::endl;;
            export_area_drawing = false;
        }

        if (export_area_drawing) {
            extension.set_param_optiongroup("area", "drawing");
        }

        if (export_area_page) {
            if (export_type == "eps") {
                std::cerr << "EPS cannot have its bounding box extend beyond its content, so if your drawing is smaller than the page, --export-area-page will clip it to drawing." << std::endl;
            }
            extension.set_param_optiongroup("area", "page");
        }

        if (!export_area_drawing && !export_area_page) {
            // Neither is set.
            if (export_type == "eps" || !object.empty()) {
                // Default to drawing for EPS or if object is specified (latter matches behavior for other export types).
                extension.set_param_optiongroup("area", "drawing");
            } else {
                extension.set_param_optiongroup("area", "page");
            }
        }

        try {
            extension.save(doc, filename_out.c_str());
        } catch(...) {
            std::cerr << "Failed to save PS/EPS/PDF to: " << filename_out << std::endl;
            return 1;
        }
    }

    return 0;
}

/**
 *  Export a document using an export extension
 *
 *  \param doc Document to export.
 *  \param filename to export to.
 *  \param output extension used for export
 */
int InkFileExportCmd::do_export_extension(SPDocument *doc, std::string const &filename_in,
                                          Inkscape::Extension::Output *extension)
{
    std::string filename_out = get_filename_out(filename_in);
    if (extension) {
        extension->set_state(Inkscape::Extension::Extension::STATE_LOADED);
        extension->save(doc, filename_out.c_str());
    }
    return 0;
}

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
