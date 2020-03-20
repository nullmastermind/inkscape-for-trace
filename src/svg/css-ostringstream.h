// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SVG_CSS_OSTRINGSTREAM_H_INKSCAPE
#define SVG_CSS_OSTRINGSTREAM_H_INKSCAPE

#include <sstream>
#include <type_traits>

namespace Inkscape {

/**
 * A thin wrapper around std::ostringstream, but writing floating point numbers in the format
 * required by CSS: `.' as decimal separator, no `e' notation, no nan or inf.
 */
class CSSOStringStream {
private:
    std::ostringstream ostr;

public:
    CSSOStringStream();

    template <typename T,
              // disable this template for float and double
              typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
    CSSOStringStream &operator<<(T const &arg)
    {
        ostr << arg;
        return *this;
    }

    CSSOStringStream &operator<<(double);

    std::string str() const {
        return ostr.str();
    }

    std::streamsize precision() const {
        return ostr.precision();
    }

    std::streamsize precision(std::streamsize p) {
        return ostr.precision(p);
    }
};

}


#endif /* !SVG_CSS_OSTRINGSTREAM_H_INKSCAPE */

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
