// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "registry.h"

namespace Inkscape {
namespace UI {
namespace Widget {

//===================================================

//---------------------------------------------------

Registry::Registry() : _updating(false) {}

Registry::~Registry() = default;

bool
Registry::isUpdating()
{
    return _updating;
}

void
Registry::setUpdating (bool upd)
{
    _updating = upd;
}

void Registry::setDesktop(SPDesktop *desktop)
{ //
    _desktop = desktop;
}

//====================================================


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
