// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_STRINGSTREAM_H
#define INKSCAPE_STRINGSTREAM_H

#include <sstream>
#include <string>
#include <type_traits>

#include <2geom/forward.h>

namespace Inkscape {

class SVGOStringStream {
private:
    std::ostringstream ostr;

public:
    SVGOStringStream();

    template <typename T,
              // disable this template for float and double
              typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
    SVGOStringStream &operator<<(T const &arg)
    {
        static_assert(!std::is_base_of<Geom::Point, T>::value, "");
        ostr << arg;
        return *this;
    }

    SVGOStringStream &operator<<(double);
    SVGOStringStream &operator<<(Geom::Point const &);

    std::string str() const {
        return ostr.str();
    }
    
    void str (std::string &s) {
        ostr.str(s);
    }

    std::streamsize precision() const {
        return ostr.precision();
    }

    std::streamsize precision(std::streamsize p) {
        return ostr.precision(p);
    }

    std::ios::fmtflags setf(std::ios::fmtflags fmtfl) {
        return ostr.setf(fmtfl);
    }

    std::ios::fmtflags setf(std::ios::fmtflags fmtfl, std::ios::fmtflags mask) {
        return ostr.setf(fmtfl, mask);
    }

    void unsetf(std::ios::fmtflags mask) {
        ostr.unsetf(mask);
    }
};

class SVGIStringStream:public std::istringstream {

public:
    SVGIStringStream();
    SVGIStringStream(const std::string &str);
};

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
