#ifndef __SP_TAG_USE_H__
#define __SP_TAG_USE_H__

/*
 * SVG <inkscape:tagref> implementation
 *
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <stddef.h>
#include <sigc++/sigc++.h>
#include "svg/svg-length.h"
#include "sp-object.h"


#define SP_TYPE_TAG_USE            (sp_tag_use_get_type ())
#define SP_TAG_USE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_TAG_USE, SPTagUse))
#define SP_TAG_USE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_TAG_USE, SPTagUseClass))
#define SP_IS_TAG_USE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_TAG_USE))
#define SP_IS_TAG_USE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_TAG_USE))

class SPTagUse;
class SPTagUseClass;
class SPTagUseReference;

struct SPTagUse : public SPObject {
    // item built from the original's repr (the visible clone)
    // relative to the SPUse itself, it is treated as a child, similar to a grouped item relative to its group
    SPObject *child;

    gchar *href;

    // the reference to the original object
    SPTagUseReference *ref;
    sigc::connection _changed_connection;
};

struct SPTagUseClass {
    SPObjectClass parent_class;
};

GType sp_tag_use_get_type (void);

SPItem *sp_tag_use_unlink (SPTagUse *use);
SPItem *sp_tag_use_get_original (SPTagUse *use);

SPItem *sp_tag_use_root(SPTagUse *use);
#endif
