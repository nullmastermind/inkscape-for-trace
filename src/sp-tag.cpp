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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "sp-tag.h"
#include "xml/repr.h"
#include <cstring>

#define DEBUG_TAG
#ifdef DEBUG_TAG
# define debug(f, a...) { g_print("%s(%d) %s:", \
                                  __FILE__,__LINE__,__FUNCTION__); \
                          g_print(f, ## a); \
                          g_print("\n"); \
                        }
#else
# define debug(f, a...) /**/
#endif

/* Tag base class */

static void sp_tag_class_init(SPTagClass *klass);
static void sp_tag_init(SPTag *tag);

static void sp_tag_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_tag_release(SPObject *object);
static void sp_tag_set(SPObject *object, unsigned int key, gchar const *value);
static void sp_tag_update(SPObject *object, SPCtx *ctx, guint flags);
static Inkscape::XML::Node *sp_tag_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static SPObjectClass *tag_parent_class;

GType
sp_tag_get_type()
{
    static GType tag_type = 0;

    if (!tag_type) {
        GTypeInfo tag_info = {
            sizeof(SPTagClass),
            NULL, NULL,
            (GClassInitFunc) sp_tag_class_init,
            NULL, NULL,
            sizeof(SPTag),
            16,
            (GInstanceInitFunc) sp_tag_init,
            NULL,    /* value_table */
        };
        tag_type = g_type_register_static(SP_TYPE_OBJECT, "SPTag", &tag_info, (GTypeFlags)0);
    }
    return tag_type;
}

static void
sp_tag_class_init(SPTagClass *klass)
{
    //GObjectClass *gobject_class = (GObjectClass *)klass;
    SPObjectClass *sp_object_class = (SPObjectClass *)klass;

    tag_parent_class = (SPObjectClass*)g_type_class_peek_parent(klass);

    sp_object_class->build = sp_tag_build;
    sp_object_class->release = sp_tag_release;
    sp_object_class->write = sp_tag_write;
    sp_object_class->set = sp_tag_set;
    sp_object_class->update = sp_tag_update;
}

static void
sp_tag_init(SPTag *tag)
{
}

/*
 * Move this SPItem into or after another SPItem in the doc
 * \param  target - the SPItem to move into or after
 * \param  intoafter - move to after the target (false), move inside (sublayer) of the target (true)
 */
void SPTag::moveTo(SPObject *target, gboolean intoafter) {

    Inkscape::XML::Node *target_ref = ( target ? target->getRepr() : NULL );
    Inkscape::XML::Node *our_ref = getRepr();
    gboolean first = FALSE;

    if (target_ref == our_ref) {
        // Move to ourself ignore
        return;
    }

    if (!target_ref) {
        // Assume move to the "first" in the top node, find the top node
        target_ref = our_ref;
        while (target_ref->parent() != target_ref->root()) {
            target_ref = target_ref->parent();
        }
        first = TRUE;
    }

    if (intoafter) {
        // Move this inside of the target at the end
        our_ref->parent()->removeChild(our_ref);
        target_ref->addChild(our_ref, NULL);
    } else if (target_ref->parent() != our_ref->parent()) {
        // Change in parent, need to remove and add
        our_ref->parent()->removeChild(our_ref);
        target_ref->parent()->addChild(our_ref, target_ref);
    } else if (!first) {
        // Same parent, just move
        our_ref->parent()->changeOrder(our_ref, target_ref);
    }
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPTag variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
static void
sp_tag_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
    object->readAttr( "inkscape:expanded" );
    
    if (((SPObjectClass *) tag_parent_class)->build) {
        ((SPObjectClass *) tag_parent_class)->build(object, document, repr);
    }
}

/**
 * Drops any allocated memory.
 */
static void
sp_tag_release(SPObject *object)
{
    /* deal with our children and our selves here */

    if (((SPObjectClass *) tag_parent_class)->release)
        ((SPObjectClass *) tag_parent_class)->release(object);
}

/**
 * Sets a specific value in the SPTag.
 */
static void
sp_tag_set(SPObject *object, unsigned int key, gchar const *value)
{
    SPTag *tag = SP_TAG(object);
    
    switch (key)
    {
        case SP_ATTR_INKSCAPE_EXPANDED:
            if ( value && !strcmp(value, "true") ) {
                tag->setExpanded(true);
            }
            break;
        default:
            if (((SPObjectClass *) tag_parent_class)->set) {
                ((SPObjectClass *) tag_parent_class)->set(object, key, value);
            }
            break;
    }
}

void SPTag::setExpanded(bool isexpanded) {
    if ( _expanded != isexpanded ){
        _expanded = isexpanded;
    }
}

/**
 * Receives update notifications.
 */
static void
sp_tag_update(SPObject *object, SPCtx *ctx, guint flags)
{
    //SPTag *tag = SP_TAG(object);

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    if (((SPObjectClass *) tag_parent_class)->update) {
        ((SPObjectClass *) tag_parent_class)->update(object, ctx, flags);
    }
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
static Inkscape::XML::Node *
sp_tag_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags)
{
    SPTag *tag = SP_TAG(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = doc->createElement("inkscape:tag");
    }
    
    // Inkscape-only object, not copied during an "plain SVG" dump:
    if (flags & SP_OBJECT_WRITE_EXT) {
        
        if (tag->_expanded) {
            repr->setAttribute("inkscape:expanded", "true");
        } else {
            repr->setAttribute("inkscape:expanded", NULL);
        }
    }

    if (((SPObjectClass *) tag_parent_class)->write) {
        ((SPObjectClass *) tag_parent_class)->write(object, doc, repr, flags);
    }

    return repr;
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
