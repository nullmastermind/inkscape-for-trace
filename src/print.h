// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef PRINT_H_INKSCAPE
#define PRINT_H_INKSCAPE

/** \file
 * Frontend to printing
 */
/*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <2geom/forward.h>

namespace Gtk {
class Window;
}

class SPDocument;
class SPStyle;

namespace Inkscape {
namespace Extension {

class Print;

} // namespace Extension
} // namespace Inkscape

struct SPPrintContext {
    Inkscape::Extension::Print *module;

    unsigned int bind(Geom::Affine const &transform, float opacity);
    unsigned int release();
    unsigned int fill(Geom::PathVector const &pathv, Geom::Affine const &ctm, SPStyle const *style,
                      Geom::OptRect const &pbox, Geom::OptRect const &dbox, Geom::OptRect const &bbox);
    unsigned int stroke(Geom::PathVector const &pathv, Geom::Affine const &ctm, SPStyle const *style,
                        Geom::OptRect const &pbox, Geom::OptRect const &dbox, Geom::OptRect const &bbox);

    unsigned int image_R8G8B8A8_N(unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
                                  Geom::Affine const &transform, SPStyle const *style);

    unsigned int text(char const *text, Geom::Point p,
                               SPStyle const *style);

    void get_param(char *name, bool *value);
};


/* UI */
void sp_print_document(Gtk::Window& parentWindow, SPDocument *doc);
void sp_print_document_to_file(SPDocument *doc, char const *filename);


#endif /* !PRINT_H_INKSCAPE */

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
