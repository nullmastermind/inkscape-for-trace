// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Provides `const_char_ptr`
 */
/*
 * Authors:
 *    Sergei Izmailov <sergei.a.izmailov@gmail.com>
 *
 * Copyright (C) 2020 Sergei Izmailov
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INKSCAPE_UTIL_CONST_CHAR_PTR_H
#define SEEN_INKSCAPE_UTIL_CONST_CHAR_PTR_H
#include <glibmm/ustring.h>
#include <string>
#include "share.h"

namespace Inkscape {
namespace Util {

/**
 * Non-owning reference to 'const char*'
 * Main-purpose: avoid overloads of type `f(char*, str&)`, `f(str&, char*)`, `f(char*, char*)`, ...
 */
class const_char_ptr{
public:
    const_char_ptr() noexcept: m_data(nullptr){};
    const_char_ptr(std::nullptr_t): const_char_ptr() {};
    const_char_ptr(const char* const data) noexcept: m_data(data) {};
    const_char_ptr(const Glib::ustring& str) noexcept: const_char_ptr(str.c_str()) {};
    const_char_ptr(const std::string& str) noexcept: const_char_ptr(str.c_str()) {};
    const_char_ptr(const ptr_shared& shared) : const_char_ptr(static_cast<const char* const>(shared)) {};

    const char * data() const noexcept { return m_data; }
private:
    const char * const m_data = nullptr;
};
}
}
#endif // SEEN_INKSCAPE_UTIL_CONST_CHAR_PTR_H
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