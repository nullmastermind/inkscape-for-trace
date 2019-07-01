// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Helpers for SPItem -> gdk_pixbuf related stuff
 *
 * Authors:
 *   John Cliff <simarilius@yahoo.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 John Cliff
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/transforms.h>

#include "helper/png-write.h"
#include "display/cairo-utils.h"
#include "display/drawing.h"
#include "display/drawing-context.h"
#include "document.h"
#include "object/sp-root.h"
#include "object/sp-defs.h"
#include "object/sp-use.h"
#include "util/units.h"
#include "inkscape.h"

#include "helper/pixbuf-ops.h"

#include <gdk/gdk.h>

// TODO look for copy-n-paste duplication of this function:
/**
 * Hide all items except @item, recursively, skipping groups and defs.
 */
static void hide_other_items_recursively(SPObject *o, SPItem *i, unsigned dkey)
{
    SPItem *item = dynamic_cast<SPItem *>(o);
    if ( item
         && !dynamic_cast<SPDefs *>(item)
         && !dynamic_cast<SPRoot *>(item)
         && !dynamic_cast<SPGroup *>(item)
         && !dynamic_cast<SPUse *>(item)
         && (i != o) )
    {
        item->invoke_hide(dkey);
    }

    // recurse
    if (i != o) {
        for (auto& child: o->children) {
            hide_other_items_recursively(&child, i, dkey);
        }
    }
}


// The following is a mutation of the flood fill code, the marker preview, and random other samplings.
// The dpi settings don't do anything yet, but I want them to, and was wanting to keep reasonably close
// to the call for the interface to the png writing.

bool sp_export_jpg_file(SPDocument *doc, gchar const *filename,
                        double x0, double y0, double x1, double y1,
                        unsigned width, unsigned height, double xdpi, double ydpi,
                        unsigned long bgcolor, double quality, SPItem* item)
{
    std::unique_ptr<Inkscape::Pixbuf> pixbuf(
        sp_generate_internal_bitmap(doc, filename, x0, y0, x1, y1,
            width, height, xdpi, ydpi, bgcolor, item));

    gchar c[32];
    g_snprintf(c, 32, "%f", quality);
    gboolean saved = gdk_pixbuf_save(pixbuf->getPixbufRaw(), filename, "jpeg", nullptr, "quality", c, NULL);
 
    return saved;
}


/**
    generates a bitmap from given items
    the bitmap is stored in RAM and not written to file
    @param x0       area left in document coordinates
    @param y0       area top in document coordinates
    @param x1       area right in document coordinates
    @param y1       area bottom in document coordinates
    @param width    bitmap width in pixels
    @param height   bitmap height in pixels
    @param xdpi
    @param ydpi
    @return the created GdkPixbuf structure or NULL if no memory is allocable
*/
Inkscape::Pixbuf *sp_generate_internal_bitmap(SPDocument *doc, gchar const */*filename*/,
                                       double x0, double y0, double x1, double y1,
                                       unsigned width, unsigned height, double xdpi, double ydpi,
                                       unsigned long /*bgcolor*/,
                                       SPItem *item_only)

{
    if (width == 0 || height == 0) return nullptr;

    Inkscape::Pixbuf *inkpb = nullptr;
    /* Create new drawing for offscreen rendering*/
    Inkscape::Drawing drawing;
    drawing.setExact(true);
    unsigned dkey = SPItem::display_key_new(1);

    doc->ensureUpToDate();

    Geom::Rect screen=Geom::Rect(Geom::Point(x0,y0), Geom::Point(x1, y1));

    Geom::Point origin = screen.min();

    Geom::Scale scale(Inkscape::Util::Quantity::convert(xdpi, "px", "in"), Inkscape::Util::Quantity::convert(ydpi, "px", "in"));
    Geom::Affine affine = scale * Geom::Translate(-origin * scale);

    /* Create ArenaItems and set transform */
    Inkscape::DrawingItem *root = doc->getRoot()->invoke_show( drawing, dkey, SP_ITEM_SHOW_DISPLAY);
    root->setTransform(affine);
    drawing.setRoot(root);

    // We show all and then hide all items we don't want, instead of showing only requested items,
    // because that would not work if the shown item references something in defs
    if (item_only) {
        hide_other_items_recursively(doc->getRoot(), item_only, dkey);
        // TODO: The following line forces 100% opacity as required by sp_asbitmap_render() in cairo-renderer.cpp
        //       Make it conditional if 'item_only' is ever used by other callers which need to retain opacity 
        item_only->get_arenaitem(dkey)->setOpacity(1.0);
    }

    Geom::IntRect final_bbox = Geom::IntRect::from_xywh(0, 0, width, height);
    drawing.update(final_bbox);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
        Inkscape::DrawingContext dc(surface, Geom::Point(0,0));

        // render items
        drawing.render(dc, final_bbox, Inkscape::DrawingItem::RENDER_BYPASS_CACHE);

        inkpb = new Inkscape::Pixbuf(surface);
    }
    else
    {
        long long size = (long long) height * (long long) cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
        g_warning("sp_generate_internal_bitmap: not enough memory to create pixel buffer. Need %lld.", size);
        cairo_surface_destroy(surface);
    }
    doc->getRoot()->invoke_hide(dkey);

//    gdk_pixbuf_save (pixbuf, "C:\\temp\\internal.jpg", "jpeg", NULL, "quality","100", NULL);

    return inkpb;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
