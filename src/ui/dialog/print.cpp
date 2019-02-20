// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Print dialog.
 */
/* Authors:
 *   Kees Cook <kees@outflux.net>
 *   Abhishek Sharma
 *   Patrick McDermott
 *
 * Copyright (C) 2007 Kees Cook
 * Copyright (C) 2017 Patrick McDermott
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cmath>

#include <gtkmm.h>

#include "inkscape.h"
#include "preferences.h"
#include "print.h"

#include "extension/internal/cairo-render-context.h"
#include "extension/internal/cairo-renderer.h"
#include "document.h"

#include "util/units.h"
#include "helper/png-write.h"
#include "svg/svg-color.h"

#include <glibmm/i18n.h>


namespace Inkscape {
namespace UI {
namespace Dialog {

Print::Print(SPDocument *doc, SPItem *base) :
    _doc (doc),
    _base (base)
{
    g_assert (_doc);
    g_assert (_base);

    _printop = Gtk::PrintOperation::create();

    // set up dialog title, based on document name
    const Glib::ustring jobname = _doc->getDocumentName() ? _doc->getDocumentName() : _("SVG Document");
    Glib::ustring title = _("Print");
    title += " ";
    title += jobname;
    _printop->set_job_name(title);

    _printop->set_unit(Gtk::UNIT_POINTS);
    Glib::RefPtr<Gtk::PageSetup> page_setup = Gtk::PageSetup::create();

    // get document width, height, and orientation
    // height must be larger than width, as in GTK+'s known paper sizes
    gdouble doc_width;
    gdouble doc_height;
    if (_doc->getWidth().value("pt") > _doc->getHeight().value("pt")) {
        page_setup->set_orientation(Gtk::PAGE_ORIENTATION_LANDSCAPE);
        doc_width = _doc->getHeight().value("pt");
        doc_height = _doc->getWidth().value("pt");
    } else {
        page_setup->set_orientation(Gtk::PAGE_ORIENTATION_PORTRAIT);
        doc_width = _doc->getWidth().value("pt");
        doc_height = _doc->getHeight().value("pt");
    }

    // attempt to match document size against known paper sizes
    Gtk::PaperSize paper_size("custom", "custom", doc_width, doc_height, Gtk::UNIT_POINTS);
    std::vector<Gtk::PaperSize> known_sizes = Gtk::PaperSize::get_paper_sizes(false);
    for (auto& size : known_sizes) {
        if (fabs(size.get_width(Gtk::UNIT_POINTS) - doc_width) >= 1.0) {
            // width (short edge) doesn't match
            continue;
        }
        if (fabs(size.get_height(Gtk::UNIT_POINTS) - doc_height) >= 1.0) {
            // height (short edge) doesn't match
            continue;
        }
        // size matches
        paper_size = size;
        break;
    }

    page_setup->set_paper_size(paper_size);
    _printop->set_default_page_setup(page_setup);
    _printop->set_use_full_page(true);

    // set up signals
    _workaround._doc = _doc;
    _workaround._base = _base;
    _workaround._tab = &_tab;
    _printop->signal_create_custom_widget().connect(sigc::mem_fun(*this, &Print::create_custom_widget));
    _printop->signal_begin_print().connect(sigc::mem_fun(*this, &Print::begin_print));
    _printop->signal_draw_page().connect(sigc::mem_fun(*this, &Print::draw_page));

    // build custom preferences tab
    _printop->set_custom_tab_label(_("Rendering"));
}

void Print::draw_page(const Glib::RefPtr<Gtk::PrintContext>& context, int /*page_nr*/)
{
    // TODO: If the user prints multiple copies we render the whole page for each copy
    //       It would be more efficient to render the page once (e.g. in "begin_print")
    //       and simply print this result as often as necessary

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    //printf("%s %d\n",__FUNCTION__, page_nr);

    if (_workaround._tab->as_bitmap()) {
        // Render as exported PNG
        prefs->setBool("/dialogs/printing/asbitmap", true);
        gdouble width = (_workaround._doc)->getWidth().value("px");
        gdouble height = (_workaround._doc)->getHeight().value("px");
        gdouble dpi = _workaround._tab->bitmap_dpi();
        prefs->setDouble("/dialogs/printing/dpi", dpi);
        
        std::string tmp_png;
        std::string tmp_base = "inkscape-print-png-XXXXXX";

        int tmp_fd;
        if ( (tmp_fd = Glib::file_open_tmp(tmp_png, tmp_base)) >= 0) {
            close(tmp_fd);

            guint32 bgcolor = 0x00000000;
            Inkscape::XML::Node *nv = sp_repr_lookup_name (_workaround._doc->rroot, "sodipodi:namedview");
            if (nv && nv->attribute("pagecolor")){
                bgcolor = sp_svg_read_color(nv->attribute("pagecolor"), 0xffffff00);
            }
            if (nv && nv->attribute("inkscape:pageopacity")){
                double opacity = 1.0;
                sp_repr_get_double (nv, "inkscape:pageopacity", &opacity);
                bgcolor |= SP_COLOR_F_TO_U(opacity);
            }

            sp_export_png_file(_workaround._doc, tmp_png.c_str(), 0.0, 0.0,
                width, height,
                (unsigned long)(Inkscape::Util::Quantity::convert(width, "px", "in") * dpi),
                (unsigned long)(Inkscape::Util::Quantity::convert(height, "px", "in") * dpi),
                dpi, dpi, bgcolor, nullptr, nullptr, true, std::vector<SPItem*>());

            // This doesn't seem to work:
            //context->set_cairo_context ( Cairo::Context::create (Cairo::ImageSurface::create_from_png (tmp_png) ), dpi, dpi );
            //
            // so we'll use a surface pattern blat instead...
            //
            // but the C++ interface isn't implemented in cairomm:
            //context->get_cairo_context ()->set_source_surface(Cairo::ImageSurface::create_from_png (tmp_png) );
            //
            // so do it in C:
            {
                Cairo::RefPtr<Cairo::ImageSurface> png = Cairo::ImageSurface::create_from_png (tmp_png);
                cairo_t *cr = gtk_print_context_get_cairo_context (context->gobj());
                cairo_matrix_t m;
                cairo_get_matrix(cr, &m);
                cairo_scale(cr, Inkscape::Util::Quantity::convert(1, "in", "pt") / dpi, Inkscape::Util::Quantity::convert(1, "in", "pt") / dpi);
                // FIXME: why is the origin offset??
                cairo_set_source_surface(cr, png->cobj(), 0, 0);
                cairo_paint(cr);
                cairo_set_matrix(cr, &m);
            }

            // Clean up
            unlink (tmp_png.c_str());
        }
        else {
            g_warning("%s", _("Could not open temporary PNG for bitmap printing"));
        }
    }
    else {
        // Render as vectors
        prefs->setBool("/dialogs/printing/asbitmap", false);
        Inkscape::Extension::Internal::CairoRenderer renderer;
        Inkscape::Extension::Internal::CairoRenderContext *ctx = renderer.createContext();

        // ctx->setPSLevel(CAIRO_PS_LEVEL_3);
        ctx->setTextToPath(false);
        ctx->setFilterToBitmap(true);
        ctx->setBitmapResolution(72);

        cairo_t *cr = gtk_print_context_get_cairo_context (context->gobj());
        cairo_surface_t *surface = cairo_get_target(cr);
        cairo_matrix_t ctm;
        cairo_get_matrix(cr, &ctm);

        bool ret = ctx->setSurfaceTarget (surface, true, &ctm);
        if (ret) {
            ret = renderer.setupDocument (ctx, _workaround._doc, TRUE, 0., nullptr);
            if (ret) {
                renderer.renderItem(ctx, _workaround._base);
                ctx->finish(false);  // do not finish the cairo_surface_t - it's owned by our GtkPrintContext!
            }
            else {
                g_warning("%s", _("Could not set up Document"));
            }
        }
        else {
            g_warning("%s", _("Failed to set CairoRenderContext"));
        }

        // Clean up
        renderer.destroyContext(ctx);
    }

}

Gtk::Widget *Print::create_custom_widget()
{
    //printf("%s\n",__FUNCTION__);
    return &_tab;
}

void Print::begin_print(const Glib::RefPtr<Gtk::PrintContext>&)
{
    //printf("%s\n",__FUNCTION__);
    _printop->set_n_pages(1);
}

Gtk::PrintOperationResult Print::run(Gtk::PrintOperationAction, Gtk::Window &parent_window)
{
    // Remember to restore the previous print settings
    _printop->set_print_settings(SP_ACTIVE_DESKTOP->printer_settings._gtk_print_settings);
    _printop->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG, parent_window);
    SP_ACTIVE_DESKTOP->printer_settings._gtk_print_settings = _printop->get_print_settings();
    return Gtk::PRINT_OPERATION_RESULT_APPLY;
}


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
