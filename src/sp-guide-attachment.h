// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_GUIDE_ATTACHMENT_H
#define SEEN_SP_GUIDE_ATTACHMENT_H

#include "object/sp-item.h"

class SPGuideAttachment {
public:
    SPItem *item;
    int snappoint_ix;

public:
    SPGuideAttachment() :
        item(static_cast<SPItem *>(nullptr)),
        snappoint_ix(0)
    { }

    SPGuideAttachment(SPItem *i, int s) :
        item(i),
        snappoint_ix(s)
    { }

    bool operator==(SPGuideAttachment const &o) const {
        return ( ( item == o.item )
                 && ( snappoint_ix == o.snappoint_ix ) );
    }

    bool operator!=(SPGuideAttachment const &o) const {
        return !(*this == o);
    }
};

#endif // SEEN_SP_GUIDE_ATTACHMENT_H

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
