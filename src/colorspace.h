// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_COLOR_SPACE_H
#define SEEN_COLOR_SPACE_H

/*
 * Authors:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2013 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#if HAVE_STDINT_H
# include <cstdint>
#endif

#include <vector>
#include <string>


namespace Inkscape
{

class ColorProfile;

} // namespace Inkscape

namespace colorspace
{

class Component
{
public:
    Component();
    Component(std::string name, std::string tip, guint scale);

    std::string name;
    std::string tip;
    guint scale;
};

std::vector<Component> getColorSpaceInfo( uint32_t space );

std::vector<Component> getColorSpaceInfo( Inkscape::ColorProfile *prof );

} // namespace colorspace

#endif // SEEN_COLOR_SPACE_H
