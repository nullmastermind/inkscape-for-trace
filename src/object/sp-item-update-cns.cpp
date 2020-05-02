// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>
#include "satisfied-guide-cns.h"

#include "sp-item-update-cns.h"
#include "sp-guide.h"

void sp_item_update_cns(SPItem &item, SPDesktop const &desktop)
{
    std::vector<Inkscape::SnapCandidatePoint> snappoints;
    item.getSnappoints(snappoints, nullptr);
    /* TODO: Implement the ordering. */
    std::vector<SPGuideConstraint> found_cns;
    satisfied_guide_cns(desktop, snappoints, found_cns);
    /* effic: It might be nice to avoid an n^2 algorithm, but in practice n will be
       small enough that it's still usually more efficient. */

    for (auto cn : found_cns)
    {
        if ( std::find(item.constraints.begin(),
                  item.constraints.end(),
                  cn)
             == item.constraints.end() )
        {
            item.constraints.push_back(cn);
            cn.g->attached_items.emplace_back(&item, cn.snappoint_ix);
        }
    }
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
