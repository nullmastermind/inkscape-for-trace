// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::XML::InvalidOperationException - invalid operation for node type
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Copyright 2004-2005 MenTaLguY <mental@rydia.net>
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_XML_INVALID_OPERATION_EXCEPTION_H
#define SEEN_INKSCAPE_XML_INVALID_OPERATION_EXCEPTION_H

#include <exception>
#include <stdexcept>

namespace Inkscape {

namespace XML {

class InvalidOperationException : public std::logic_error {
public:
    InvalidOperationException(std::string const &message) :
        std::logic_error(message)
    { }
};

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
