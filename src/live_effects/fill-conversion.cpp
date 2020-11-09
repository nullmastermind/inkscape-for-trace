// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Fille/stroke conversion routines for LPEs which draw a stroke
 *
 * Authors:
 *   Liam P White
 *
 * Copyright (C) 2020 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "fill-conversion.h"

#include "document.h"
#include "desktop-style.h"
#include "object/sp-shape.h"
#include "object/sp-defs.h"
#include "object/sp-paint-server.h"
#include "svg/svg-color.h"
#include "svg/css-ostringstream.h"
#include "style.h"

static SPObject *generate_linked_fill(SPShape *source)
{
    // Create new fill and effect object
    SPDocument *doc = source->document;
    SPObject *defs  = doc->getDefs();

    Inkscape::XML::Node *effectRepr = doc->getReprDoc()->createElement("inkscape:path-effect");
    SPObject *effectObj             = nullptr;
    Inkscape::XML::Node *pathRepr   = doc->getReprDoc()->createElement("svg:path");
    SPObject *pathObj               = nullptr;

    gchar *effectTarget = nullptr;
    gchar *pathTarget   = nullptr;

    effectTarget = g_strdup_printf("#%s,0,1", source->getId());
    effectRepr->setAttribute("effect", "fill_between_many");
    effectRepr->setAttribute("method", "originald");
    effectRepr->setAttribute("linkedpaths", effectTarget);
    defs->appendChild(effectRepr);

    effectObj = doc->getObjectByRepr(effectRepr);

    pathTarget = g_strdup_printf("#%s", effectObj->getId());
    pathRepr->setAttribute("inkscape:original-d", "M 0,0");
    pathRepr->setAttribute("inkscape:path-effect", pathTarget);
    pathRepr->setAttribute("d", "M 0,0");

    SPObject *prev = source->getPrev();
    source->parent->addChild(pathRepr, prev ? prev->getRepr() : nullptr);

    pathObj = doc->getObjectByRepr(pathRepr);
    source->setAttribute("inkscape:linked-fill", pathObj->getId());

    g_free(effectTarget);
    g_free(pathTarget);

    return pathObj;
}

static SPObject *get_linked_fill(SPObject *source)
{
    SPDocument *doc = source->document;

    char const *linked_fill_id = source->getAttribute("inkscape:linked-fill");
    if (!linked_fill_id) {
        return nullptr;
    }

    return doc->getObjectById(linked_fill_id);
}

static void convert_fill_color(SPCSSAttr *css, SPObject *source)
{
    char c[64];

    sp_svg_write_color(c, sizeof(c), source->style->fill.value.color.toRGBA32(SP_SCALE24_TO_FLOAT(source->style->fill_opacity.value)));
    sp_repr_css_set_property(css, "fill", c);
}

static void convert_stroke_color(SPCSSAttr *css, SPObject *source)
{
    char c[64];

    sp_svg_write_color(c, sizeof(c), source->style->stroke.value.color.toRGBA32(SP_SCALE24_TO_FLOAT(source->style->stroke_opacity.value)));
    sp_repr_css_set_property(css, "fill", c);
}

static void revert_stroke_color(SPCSSAttr *css, SPObject *source)
{
    char c[64];

    sp_svg_write_color(c, sizeof(c), source->style->fill.value.color.toRGBA32(SP_SCALE24_TO_FLOAT(source->style->fill_opacity.value)));
    sp_repr_css_set_property(css, "stroke", c);
}

static void convert_fill_server(SPCSSAttr *css, SPObject *source)
{
    SPPaintServer *server = source->style->getFillPaintServer();

    if (server) {
        Glib::ustring str;
        str += "url(#";
        str += server->getId();
        str += ")";
        sp_repr_css_set_property(css, "fill", str.c_str());
    }
}

static void convert_stroke_server(SPCSSAttr *css, SPObject *source)
{
    SPPaintServer *server = source->style->getStrokePaintServer();

    if (server) {
        Glib::ustring str;
        str += "url(#";
        str += server->getId();
        str += ")";
        sp_repr_css_set_property(css, "fill", str.c_str());
    }
}

static void revert_stroke_server(SPCSSAttr *css, SPObject *source)
{
    SPPaintServer *server = source->style->getFillPaintServer();

    if (server) {
        Glib::ustring str;
        str += "url(#";
        str += server->getId();
        str += ")";
        sp_repr_css_set_property(css, "stroke", str.c_str());
    }
}

static bool has_fill(SPObject *source)
{
    return source->style->fill.isColor() || source->style->fill.isPaintserver();
}

static bool has_stroke(SPObject *source)
{
    return source->style->stroke.isColor() || source->style->stroke.isPaintserver();
}

namespace Inkscape {
namespace LivePathEffect {

void lpe_shape_convert_stroke_and_fill(SPShape *shape)
{
    if (has_fill(shape)) {
        SPCSSAttr *fill_css = sp_repr_css_attr_new();
        SPObject *linked = generate_linked_fill(shape);

        if (shape->style->fill.isColor()) {
            convert_fill_color(fill_css, shape);
        } else {
            convert_fill_server(fill_css, shape);
        }

        sp_desktop_apply_css_recursive(linked, fill_css, true);
        sp_repr_css_attr_unref(fill_css);
    }

    SPCSSAttr *stroke_css = sp_repr_css_attr_new();

    if (has_stroke(shape)) {
        if (shape->style->stroke.isColor()) {
            convert_stroke_color(stroke_css, shape);
        } else {
            convert_stroke_server(stroke_css, shape);
        }
    }

    sp_repr_css_set_property(stroke_css, "fill-rule", "nonzero");
    sp_repr_css_set_property(stroke_css, "stroke", "none");
    sp_desktop_apply_css_recursive(shape, stroke_css, true);
    sp_repr_css_attr_unref(stroke_css);
}

void lpe_shape_revert_stroke_and_fill(SPShape *shape, double width)
{
    SPObject *linked = get_linked_fill(shape);
    SPCSSAttr *css = sp_repr_css_attr_new();

    if (has_fill(shape)) {
        if (shape->style->fill.isColor()) {
            revert_stroke_color(css, shape);
        } else {
            revert_stroke_server(css, shape);
        }
    }

    if (linked != nullptr) {
        if (linked->style->fill.isColor()) {
            convert_fill_color(css, linked);
        } else {
            convert_fill_server(css, linked);
        }

        linked->deleteObject();
    } else {
        sp_repr_css_set_property(css, "fill", "none");
    }

    Inkscape::CSSOStringStream os;
    os << fabs(width);
    sp_repr_css_set_property(css, "stroke-width", os.str().c_str());

    sp_desktop_apply_css_recursive(shape, css, true);
    sp_repr_css_attr_unref(css);
}

}
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
