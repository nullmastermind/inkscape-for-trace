// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/coord.h>

#include "satisfied-guide-cns.h"

#include "desktop.h"

#include "object/sp-guide.h"
#include "object/sp-namedview.h"

void satisfied_guide_cns(SPDesktop const &desktop,
                         std::vector<Inkscape::SnapCandidatePoint> const &snappoints,
                         std::vector<SPGuideConstraint> &cns)
{
    SPNamedView const &nv = *desktop.getNamedView();
    for(auto guide : nv.guides) {
        SPGuide &g = *guide;
        for (unsigned int i = 0; i < snappoints.size(); ++i) {
            if (Geom::are_near(g.getDistanceFrom(snappoints[i].getPoint()), 0, 1e-2)) {
                cns.emplace_back(&g, i);
            }
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
