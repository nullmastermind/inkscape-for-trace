// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include <glib.h>

#include "svg/strip-trailing-zeros.h"

std::string
strip_trailing_zeros(std::string str)
{
    std::string::size_type p_ix = str.find('.');
    if (p_ix != std::string::npos) {
        std::string::size_type e_ix = str.find('e', p_ix);
        /* N.B. In some contexts (e.g. CSS) it is an error for a number to contain `e'.  fixme:
         * Default to avoiding `e', e.g. using sprintf(str, "%17f", d).  Add a new function that
         * allows use of `e' and use that function only where the spec allows it.
         */
        std::string::size_type nz_ix = str.find_last_not_of('0', (e_ix == std::string::npos
                                                                  ? e_ix
                                                                  : e_ix - 1));
        if (nz_ix == std::string::npos || nz_ix < p_ix || nz_ix >= e_ix) {
            g_error("have `.' but couldn't find non-0");
        } else {
            str.erase(str.begin() + (nz_ix == p_ix
                                     ? p_ix
                                     : nz_ix + 1),
                      (e_ix == std::string::npos
                       ? str.end()
                       : str.begin() + e_ix));
        }
    }
    return str;
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
