// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "sp-filter.h"
#include "sp-filter-reference.h"

bool
SPFilterReference::_acceptObject(SPObject *obj) const
{
    return SP_IS_FILTER(obj) && URIReference::_acceptObject(obj);
    /* effic: Don't bother making this an inline function: _acceptObject is a virtual function,
       typically called from a context where the runtime type is not known at compile time. */
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
