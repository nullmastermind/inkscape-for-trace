// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SATISFIED_GUIDE_CNS_H
#define SEEN_SATISFIED_GUIDE_CNS_H

#include <2geom/forward.h>
#include <vector>

#include "snap-candidate.h"

class SPDesktop;
class SPGuideConstraint;

void satisfied_guide_cns(SPDesktop const &desktop,
                         std::vector<Inkscape::SnapCandidatePoint> const &snappoints,
                         std::vector<SPGuideConstraint> &cns);


#endif // SEEN_SATISFIED_GUIDE_CNS_H

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
