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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

/* Skeleton base class */

#define SP_TYPE_TAG (sp_tag_get_type())
#define SP_TAG(o) (G_TYPE_CHECK_INSTANCE_CAST((o), SP_TYPE_TAG, SPTag))
#define SP_IS_TAG(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), SP_TYPE_TAG))

class SPTag;
class SPTagClass;

class SPTag : public SPObject {
public:
    bool _expanded;
    
    bool expanded() const { return _expanded; }
    void setExpanded(bool isexpanded);

    void moveTo(SPObject *target, gboolean intoafter);
    
};

struct SPTagClass {
    SPObjectClass parent_class;
};

GType sp_tag_get_type();


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
