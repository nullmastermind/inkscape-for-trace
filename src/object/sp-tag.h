// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_TAG_H_SEEN
#define SP_TAG_H_SEEN

/** \file
 * SVG <inkscape:tag> implementation
 *
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

/* Skeleton base class */

#define SP_TAG(o) (dynamic_cast<SPTag*>(o))
#define SP_IS_TAG(o) (dynamic_cast<SPTag*>(o) != NULL)

class SPTag;

class SPTag : public SPObject {
public:
    SPTag() = default;
    ~SPTag() override = default;

    void build(SPDocument * doc, Inkscape::XML::Node *repr) override;
    //virtual void release();
    void set(SPAttr key, const gchar* value) override;
    void update(SPCtx * ctx, unsigned flags) override;

    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) override;

    bool expanded() const { return _expanded; }
    void setExpanded(bool isexpanded);

    void moveTo(SPObject *target, gboolean intoafter);

private:
    bool _expanded;
};


#endif /* !SP_SKELETON_H_SEEN */

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
