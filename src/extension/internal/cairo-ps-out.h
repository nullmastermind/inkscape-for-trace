// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A quick hack to use the print output to write out a file.  This
 * then makes 'save as...' PS.
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Ulf Erikson <ulferikson@users.sf.net>
 *   Adib Taraben <theAdib@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef EXTENSION_INTERNAL_CAIRO_PS_OUT_H
#define EXTENSION_INTERNAL_CAIRO_PS_OUT_H

#include "extension/implementation/implementation.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class CairoPsOutput : Inkscape::Extension::Implementation::Implementation {

public:
    bool check(Inkscape::Extension::Extension *module) override;
    void save(Inkscape::Extension::Output *mod,
              SPDocument *doc,
              gchar const *filename) override;
    static void init();
    bool textToPath(Inkscape::Extension::Print *ext) override;

};

class CairoEpsOutput : Inkscape::Extension::Implementation::Implementation {

public:
    bool check(Inkscape::Extension::Extension *module) override;
    void save(Inkscape::Extension::Output *mod,
              SPDocument *doc,
              gchar const *uri) override;
    static void init();
    bool textToPath(Inkscape::Extension::Print *ext) override;

};

} } }  /* namespace Inkscape, Extension, Implementation */

#endif /* !EXTENSION_INTERNAL_CAIRO_PS_OUT_H */

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
