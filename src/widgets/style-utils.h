/**
 * @file
 * Common utility functions for manipulating style.
 */
/* Author:
 *   Shlomi Fish <shlomif@cpan.org>
 *
 * Copyright (C) 2016 Shlomi Fish
 *
 * Released under the Expat licence.
 */

#ifndef SEEN_DIALOGS_STYLE_UTILS_H
#define SEEN_DIALOGS_STYLE_UTILS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "desktop-style.h"

namespace Inkscape {
    inline bool is_query_style_updateable(const int style) {
        return (style == QUERY_STYLE_MULTIPLE_DIFFERENT || style == QUERY_STYLE_NOTHING);
    }
} // namespace Inkscape

#endif // SEEN_DIALOGS_STYLE_UTILS_H

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
