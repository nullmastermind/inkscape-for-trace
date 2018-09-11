// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_STOP_H
#define SEEN_SP_STOP_H

/** \file
 * SPStop: SVG <stop> implementation.
 */
/*
 * Authors:
 */

#include "sp-object.h"
#include "color.h"

typedef unsigned int guint32;

namespace Glib {
class ustring;
}

#define SP_STOP(obj) (dynamic_cast<SPStop*>((SPObject*)obj))
#define SP_IS_STOP(obj) (dynamic_cast<const SPStop*>((SPObject*)obj) != NULL)

/** Gradient stop. */
class SPStop : public SPObject {
public:
	SPStop();
	~SPStop() override;

    /// \todo fixme: Should be SPSVGPercentage
    float offset;

    bool currentColor;

    Glib::ustring * path_string;
    //SPCurve path;

    SPStop* getNextStop();
    SPStop* getPrevStop();

    SPColor getColor() const;
    gfloat getOpacity() const;
    guint32 get_rgba32() const;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void set(SPAttributeEnum key, const char* value) override;
        void modified(guint flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};


#endif /* !SEEN_SP_STOP_H */

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
