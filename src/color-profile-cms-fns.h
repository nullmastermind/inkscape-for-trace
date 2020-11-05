// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLOR_PROFILE_CMS_FNS_H
#define SEEN_COLOR_PROFILE_CMS_FNS_H

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <lcms2.h>

#include "cms-color-types.h"

namespace Inkscape {

// Note: these can later be adjusted to adapt for lcms2:

class ColorSpaceSigWrapper : public ColorSpaceSig {
public :
    ColorSpaceSigWrapper( cmsColorSpaceSignature sig ) : ColorSpaceSig( static_cast<guint32>(sig) ) {}
    ColorSpaceSigWrapper( ColorSpaceSig const &other ) : ColorSpaceSig( other ) {}

    operator cmsColorSpaceSignature() const { return static_cast<cmsColorSpaceSignature>(value); }
};

class ColorProfileClassSigWrapper : public ColorProfileClassSig {
public :
    ColorProfileClassSigWrapper( cmsProfileClassSignature sig ) : ColorProfileClassSig( static_cast<guint32>(sig) ) {}
    ColorProfileClassSigWrapper( ColorProfileClassSig const &other ) : ColorProfileClassSig( other ) {}

    operator cmsProfileClassSignature() const { return static_cast<cmsProfileClassSignature>(value); }
};

cmsColorSpaceSignature asICColorSpaceSig(ColorSpaceSig const & sig);
cmsProfileClassSignature asICColorProfileClassSig(ColorProfileClassSig const & sig);

} // namespace Inkscape


#endif // !SEEN_COLOR_PROFILE_CMS_FNS_H

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
