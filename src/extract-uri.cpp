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
#include <glib.h>

#include "extract-uri.h"

// FIXME: kill this ugliness when we have a proper CSS parser

// Functions as per 4.3.4 of CSS 2.1
// http://www.w3.org/TR/CSS21/syndata.html#uri
std::string extract_uri(char const *s, char const **endptr)
{
    std::string result;

    if (!s)
        return result;

    gchar const *sb = s;
    if ( strlen(sb) < 4 || strncmp(sb, "url", 3) != 0 ) {
        return result;
    }

    sb += 3;

    if ( endptr ) {
        *endptr = nullptr;
    }

    // This first whitespace technically is not allowed.
    // Just left in for now for legacy behavior.
    while ( ( *sb == ' ' ) ||
            ( *sb == '\t' ) )
    {
        sb++;
    }

    if ( *sb == '(' ) {
        sb++;
        while ( ( *sb == ' ' ) ||
                ( *sb == '\t' ) )
        {
            sb++;
        }

        gchar delim = ')';
        if ( (*sb == '\'' || *sb == '"') ) {
            delim = *sb;
            sb++;
        }

        if (!*sb) {
            return result;
        }

        gchar const* se = sb;
        while ( *se && (*se != delim) ) {
            se++;
        }

        // we found the delimiter
        if ( *se ) {
            if ( delim == ')' ) {
                if ( endptr ) {
                    *endptr = se + 1;
                }

                // back up for any trailing whitespace
                while (se > sb && g_ascii_isspace(se[-1]))
                {
                    se--;
                }

                result = std::string(sb, se);
            } else {
                gchar const* tail = se + 1;
                while ( ( *tail == ' ' ) ||
                        ( *tail == '\t' ) )
                {
                    tail++;
                }
                if ( *tail == ')' ) {
                    if ( endptr ) {
                        *endptr = tail + 1;
                    }
                    result = std::string(sb, se);
                }
            }
        }
    }

    return result;
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
