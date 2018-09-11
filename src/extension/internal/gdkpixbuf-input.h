// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_EXTENSION_INTERNAL_GDKPIXBUF_INPUT_H
#define INKSCAPE_EXTENSION_INTERNAL_GDKPIXBUF_INPUT_H

#include "extension/implementation/implementation.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class GdkpixbufInput : Inkscape::Extension::Implementation::Implementation {
public:
    SPDocument *open(Inkscape::Extension::Input *mod,
                     char const *uri) override;
    static void init();
};

} } }  /* namespace Inkscape, Extension, Implementation */


#endif /* INKSCAPE_EXTENSION_INTERNAL_GDKPIXBUF_INPUT_H */

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
