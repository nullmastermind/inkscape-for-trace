/**
 * \file
 * \brief helper functions for gettext
 *//*
 * Authors:
 *   Eduard Braun <eduard.braun2@gmx.de>
 *
 * Copyright 2017 Authors
 *
 * This file is part of Inkscape.
 *
 * Inkscape is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Inkscape is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Inkscape.  If not, see <http://www.gnu.org/licenses/>.
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