// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::IO::HTTP - make internet requests using libsoup
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_IO_HTTP_H
#define SEEN_INKSCAPE_IO_HTTP_H

#include <functional>
#include <glibmm/ustring.h>

/**
 * simple libsoup based resource API
 */

namespace Inkscape {
namespace IO {
namespace HTTP {

    Glib::ustring get_file(Glib::ustring uri, unsigned int timeout=0, std::function<void(Glib::ustring)> func=nullptr);

}
}
}

#endif
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
