// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/** \file
 * Frontend to printing
 */
/*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Kees Cook <kees@outflux.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * This code is in public domain
 */

#include "print.h"

#include "desktop.h"
#include "document.h"
#include "inkscape.h"

#include "display/drawing-item.h"
#include "display/drawing.h"

#include "extension/print.h"
#include "extension/system.h"

#include "object/sp-item.h"
#include "object/sp-root.h"

#include "ui/dialog/print.h"


unsigned int
SPPrintContext::bind(Geom::Affine const &transform, float opacity)
{
    return module->bind(transform, opacity);
}

unsigned int
SPPrintContext::release()
{
    return module->release();
}

unsigned int
SPPrintContext::comment(char const *comment)
{
    return module->comment(comment);
}

unsigned int
SPPrintContext::fill(Geom::PathVector const &pathv, Geom::Affine const &ctm, SPStyle const *style,
                     Geom::OptRect const &pbox, Geom::OptRect const &dbox, Geom::OptRect const &bbox)
{
    return module->fill(pathv, ctm, style, pbox, dbox, bbox);
}

unsigned int
SPPrintContext::stroke(Geom::PathVector const &pathv, Geom::Affine const &ctm, SPStyle const *style,
                       Geom::OptRect const &pbox, Geom::OptRect const &dbox, Geom::OptRect const &bbox)
{
    return module->stroke(pathv, ctm, style, pbox, dbox, bbox);
}

unsigned int
SPPrintContext::image_R8G8B8A8_N(guchar *px, unsigned int w, unsigned int h, unsigned int rs,
                                 Geom::Affine const &transform, SPStyle const *style)
{
    return module->image(px, w, h, rs, transform, style);
}

unsigned int SPPrintContext::text(char const *text, Geom::Point p,
                           SPStyle const *style)
{
    return module->text(text, p, style);
}

/* UI */

void
sp_print_document(Gtk::Window& parentWindow, SPDocument *doc)
{
    doc->ensureUpToDate();

    // Build arena
    SPItem      *base = doc->getRoot();

    // Run print dialog
    Inkscape::UI::Dialog::Print printop(doc,base);
    Gtk::PrintOperationResult res = printop.run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG, parentWindow);
    (void)res; // TODO handle this
}

void sp_print_document_to_file(SPDocument *doc, gchar const *filename)
{
    doc->ensureUpToDate();

    Inkscape::Extension::Print *mod = Inkscape::Extension::get_print(SP_MODULE_KEY_PRINT_PS);
    SPPrintContext context;
    gchar const *oldconst = mod->get_param_string("destination");
    gchar *oldoutput = g_strdup(oldconst);

    mod->set_param_string("destination", (gchar *)filename);

/* Start */
    context.module = mod;
    /* fixme: This has to go into module constructor somehow */
    /* Create new drawing */
    mod->base = doc->getRoot();
    Inkscape::Drawing drawing;
    mod->dkey = SPItem::display_key_new(1);
    mod->root = (mod->base)->invoke_show(drawing, mod->dkey, SP_ITEM_SHOW_DISPLAY);
    drawing.setRoot(mod->root);
    /* Print document */
    mod->begin(doc);
    (mod->base)->invoke_print(&context);
    mod->finish();
    /* Release drawing items */
    (mod->base)->invoke_hide(mod->dkey);
    mod->base = nullptr;
    mod->root = nullptr; // should be deleted by invoke_hide
/* end */

    mod->set_param_string("destination", oldoutput);
    g_free(oldoutput);
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
