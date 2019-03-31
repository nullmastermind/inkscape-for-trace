// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * \brief helper functions for gettext
 *//*
 * Authors:
 * see git history
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_GETTEXT_HELPER_H
#define SEEN_GETTEXT_HELPER_H

namespace Inkscape {
     void initialize_gettext();
     void bind_textdomain_codeset_utf8();
     void bind_textdomain_codeset_console();
}

#endif // SEEN_GETTEXT_HELPER_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace .0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
