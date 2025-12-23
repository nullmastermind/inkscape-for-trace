// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for working with objects without GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-object.h"

#include <giomm.h> // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>
#include <iostream>

#include "actions-helper.h"
#include "document-undo.h"
#include "inkscape-application.h"
#include "inkscape.h" // Inkscape::Application
#include "path/path-simplify.h"
#include "selection.h" // Selection
#include "trace/autotrace/inkscape-autotrace.h"
#include "trace/depixelize/inkscape-depixelize.h"
#include "trace/potrace/inkscape-potrace.h"

#include "display/cairo-utils.h" // For Inkscape::Pixbuf
#include "object/sp-image.h"     // For SPImage
#include "xml/repr.h"            // For sp_repr_get_double
#include <2geom/transforms.h>    // For Geom::Translate, Geom::Scale, Geom::Affine

// Headless-compatible bitmap tracing implementation.
// This function works without requiring SP_ACTIVE_DESKTOP by using the app's
// active selection and document directly, suitable for CLI batch processing.
void selection_trace(const Glib::VariantBase &value, InkscapeApplication *app)
{
    // Parse parameters from the action string
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);
    std::vector<Glib::ustring> settings = Glib::Regex::split_simple(",", s.get());

    if (settings.size() < 7) {
        std::cerr << "selection_trace: requires 7 parameters: scans,smooth,stack,removeBackground,speckles,smoothCorners,optimize" << std::endl;
        return;
    }

    auto scans = std::stoi(settings[0]);           // Number of colors/scans
    auto smooth = settings[1] == "true";           // Smooth tracing
    auto stack = settings[2] == "true";            // Stack scan results
    auto removeBackground = settings[3] == "true"; // Remove background
    auto speckles = std::stoi(settings[4]);        // Speckle suppression (turdsize)
    auto smoothCorners = std::stof(settings[5]);   // Corner smoothing (alphamax)
    auto optimize = std::stof(settings[6]);        // Path optimization (opttolerance)

    // Get document and selection without requiring a desktop
    SPDocument *document = app->get_active_document();
    if (!document) {
        std::cerr << "selection_trace: No active document" << std::endl;
        return;
    }

    Inkscape::Selection *selection = app->get_active_selection();
    if (!selection) {
        std::cerr << "selection_trace: No active selection" << std::endl;
        return;
    }

    if (selection->isEmpty()) {
        std::cerr << "selection_trace: Selection is empty" << std::endl;
        return;
    }

    // Find the first SPImage in the selection
    SPImage *img = nullptr;
    auto items = selection->items();
    for (auto i = items.begin(); i != items.end(); ++i) {
        if (SP_IS_IMAGE(*i)) {
            img = SP_IMAGE(*i);
            break;
        }
    }

    if (!img) {
        std::cerr << "selection_trace: No image found in selection" << std::endl;
        return;
    }

    // Ensure the document (and image pixbuf) is up to date
    document->ensureUpToDate();

    if (!img->pixbuf) {
        std::cerr << "selection_trace: Image has no bitmap data" << std::endl;
        return;
    }

    // Get pixbuf from the image, converting format if needed
    GdkPixbuf *raw_pb = img->pixbuf->getPixbufRaw(false);
    GdkPixbuf *trace_pb = gdk_pixbuf_copy(raw_pb);
    if (img->pixbuf->pixelFormat() == Inkscape::Pixbuf::PF_CAIRO) {
        convert_pixels_argb32_to_pixbuf(
            gdk_pixbuf_get_pixels(trace_pb),
            gdk_pixbuf_get_width(trace_pb),
            gdk_pixbuf_get_height(trace_pb),
            gdk_pixbuf_get_rowstride(trace_pb));
    }
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Glib::wrap(trace_pb, false);

    if (!pixbuf) {
        std::cerr << "selection_trace: Failed to get pixbuf from image" << std::endl;
        return;
    }

    // Create and configure the Potrace tracing engine
    Inkscape::Trace::Potrace::PotraceTracingEngine pte(
        Inkscape::Trace::Potrace::TRACE_QUANT_COLOR,
        false,      // invert
        64,         // quantization colors (internal)
        0.45,       // brightness threshold
        0.,         // brightness floor
        .65,        // canny high threshold
        scans,
        stack,
        smooth,
        removeBackground
    );
    pte.potraceParams->opticurve = true;
    pte.potraceParams->opttolerance = optimize;
    pte.potraceParams->alphamax = smoothCorners;
    pte.potraceParams->turdsize = speckles;

    // Perform the trace
    std::vector<Inkscape::Trace::TracingEngineResult> results = pte.trace(pixbuf);
    int nrPaths = results.size();

    if (nrPaths < 1) {
        std::cerr << "selection_trace: Tracing produced no paths" << std::endl;
        return;
    }

    // Get image position and size for transforming result paths
    Inkscape::XML::Node *imgRepr = img->getRepr();
    Inkscape::XML::Node *par = imgRepr->parent();

    double x = 0.0, y = 0.0, width = 0.0, height = 0.0, dval = 0.0;
    if (sp_repr_get_double(imgRepr, "x", &dval)) x = dval;
    if (sp_repr_get_double(imgRepr, "y", &dval)) y = dval;
    if (sp_repr_get_double(imgRepr, "width", &dval)) width = dval;
    if (sp_repr_get_double(imgRepr, "height", &dval)) height = dval;

    double iwidth = (double)pixbuf->get_width();
    double iheight = (double)pixbuf->get_height();
    double iwscale = width / iwidth;
    double ihscale = height / iheight;

    // Compute transform: scale then translate, combined with image's own transform
    Geom::Translate trans(x, y);
    Geom::Scale scal(iwscale, ihscale);
    Geom::Affine tf(scal * trans);
    tf *= img->transform;

    // Create new path elements from trace results
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    Inkscape::XML::Node *groupRepr = nullptr;

    // If multiple paths, wrap them in a group
    if (nrPaths > 1) {
        groupRepr = xml_doc->createElement("svg:g");
        par->addChild(groupRepr, imgRepr);
    }

    long totalNodeCount = 0L;
    for (auto &result : results) {
        totalNodeCount += result.getNodeCount();

        Inkscape::XML::Node *pathRepr = xml_doc->createElement("svg:path");
        pathRepr->setAttributeOrRemoveIfEmpty("style", result.getStyle());
        pathRepr->setAttributeOrRemoveIfEmpty("d", result.getPathData());

        if (nrPaths > 1) {
            groupRepr->addChild(pathRepr, nullptr);
        } else {
            par->addChild(pathRepr, imgRepr);
        }

        // Apply the transform from the image to the new path
        SPObject *reprobj = document->getObjectByRepr(pathRepr);
        if (reprobj) {
            SPItem *newItem = SP_ITEM(reprobj);
            newItem->doWriteTransform(tf);
        }

        if (nrPaths == 1) {
            selection->clear();
            selection->add(pathRepr);
        }
        Inkscape::GC::release(pathRepr);
    }

    // If we have a group, select it
    if (nrPaths > 1) {
        selection->clear();
        selection->add(groupRepr);
        Inkscape::GC::release(groupRepr);
    }

    // Commit the change for undo
    Inkscape::DocumentUndo::done(document, 0, "Trace bitmap (headless)");

    std::cerr << "selection_trace: Done. " << totalNodeCount << " nodes created in " << nrPaths << " path(s)" << std::endl;
}

// No sanity checking is done... should probably add.
void object_set_attribute(const Glib::VariantBase &value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", s.get());
    if (tokens.size() != 2) {
        std::cerr << "action:object_set_attribute: requires 'attribute name, attribute value'" << std::endl;
        return;
    }

    auto selection = app->get_active_selection();
    if (selection->isEmpty()) {
        std::cerr << "action:object_set_attribute: selection empty!" << std::endl;
        return;
    }

    // Should this be a selection member function?
    auto items = selection->items();
    for (auto i = items.begin(); i != items.end(); ++i) {
        Inkscape::XML::Node *repr = (*i)->getRepr();
        repr->setAttribute(tokens[0], tokens[1]);
    }

    // Needed to update repr (is this the best way?).
    Inkscape::DocumentUndo::done(app->get_active_document(), 0, "ActionObjectSetAttribute");
}

// No sanity checking is done... should probably add.
void object_set_property(const Glib::VariantBase &value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", s.get());
    if (tokens.size() != 2) {
        std::cerr << "action:object_set_property: requires 'property name, property value'" << std::endl;
        return;
    }

    auto selection = app->get_active_selection();
    if (selection->isEmpty()) {
        std::cerr << "action:object_set_property: selection empty!" << std::endl;
        return;
    }

    // Should this be a selection member function?
    auto items = selection->items();
    for (auto i = items.begin(); i != items.end(); ++i) {
        Inkscape::XML::Node *repr = (*i)->getRepr();
        SPCSSAttr *css = sp_repr_css_attr(repr, "style");
        sp_repr_css_set_property(css, tokens[0].c_str(), tokens[1].c_str());
        sp_repr_css_set(repr, css, "style");
        sp_repr_css_attr_unref(css);
    }

    // Needed to update repr (is this the best way?).
    Inkscape::DocumentUndo::done(app->get_active_document(), 0, "ActionObjectSetProperty");
}

void object_unlink_clones(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document = app->get_active_document();
    selection->setDocument(document);

    selection->unlink();
}

void object_to_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document = app->get_active_document();
    selection->setDocument(document);

    selection->toCurves(); // TODO: Rename toPaths()
}

void object_stroke_to_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document = app->get_active_document();
    selection->setDocument(document);

    selection->strokesToPaths();
}

void object_simplify_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document = app->get_active_document();
    selection->setDocument(document);

    selection->simplifyPaths();
}

std::vector<std::vector<Glib::ustring>> raw_data_object = {
    // clang-format off
    {"app.object-set-attribute",      N_("Set Attribute"),         "Object",     N_("Set or update an attribute of selected objects; usage: object-set-attribute:attribute name, attribute value;")},
    {"app.object-set-property",       N_("Set Property"),          "Object",     N_("Set or update a property on selected objects; usage: object-set-property:property name, property value;")},
    {"app.object-unlink-clones",      N_("Unlink Clones"),         "Object",     N_("Unlink clones and symbols")                          },
    {"app.object-to-path",            N_("Object To Path"),        "Object",     N_("Convert shapes to paths")                            },
    {"app.object-stroke-to-path",     N_("Stroke to Path"),        "Object",     N_("Convert strokes to paths")                           },
    {"app.object-simplify-path",      N_("Simplify Path"),         "Object",     N_("Simplify paths, reducing node counts")               }
    // clang-format on
};

void add_actions_object(InkscapeApplication *app)
{
    Glib::VariantType Bool(Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)

    // clang-format off
    gapp->add_action_with_parameter( "object-set-attribute",     String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_set_attribute),      app));
    gapp->add_action_with_parameter( "selection-trace",          String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&selection_trace),           app));
    gapp->add_action_with_parameter( "object-set-property",      String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_set_property),       app));
    gapp->add_action(                "object-unlink-clones",             sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_unlink_clones),      app));
    gapp->add_action(                "object-to-path",                   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_to_path),            app));
    gapp->add_action(                "object-stroke-to-path",            sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_stroke_to_path),     app));
    gapp->add_action(                "object-simplify-path",             sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_simplify_path),      app));
    // clang-format on

#endif

    app->get_action_extra_data().add_data(raw_data_object);
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
