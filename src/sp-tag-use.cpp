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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cstring>
#include <string>

#include <glibmm/i18n.h>
#include "display/drawing-group.h"
#include "attributes.h"
#include "document.h"
#include "sp-object-repr.h"
#include "uri.h"
#include "xml/repr.h"
#include "preferences.h"
#include "style.h"
#include "sp-symbol.h"
#include "sp-tag-use.h"
#include "sp-tag-use-reference.h"

/* fixme: */

static void sp_tag_use_class_init(SPTagUseClass *classname);
static void sp_tag_use_init(SPTagUse *use);
static void sp_tag_use_finalize(GObject *obj);

static void sp_tag_use_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_tag_use_release(SPObject *object);
static void sp_tag_use_set(SPObject *object, unsigned key, gchar const *value);
static Inkscape::XML::Node *sp_tag_use_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);
static void sp_tag_use_update(SPObject *object, SPCtx *ctx, guint flags);

static void sp_tag_use_href_changed(SPObject *old_ref, SPObject *ref, SPTagUse *use);

static SPObjectClass *parent_class;


GType
sp_tag_use_get_type(void)
{
    static GType use_type = 0;
    if (!use_type) {
        GTypeInfo use_info = {
            sizeof(SPTagUseClass),
            NULL,   /* base_init */
            NULL,   /* base_finalize */
            (GClassInitFunc) sp_tag_use_class_init,
            NULL,   /* class_finalize */
            NULL,   /* class_data */
            sizeof(SPTagUse),
            16,     /* n_preallocs */
            (GInstanceInitFunc) sp_tag_use_init,
            NULL,   /* value_table */
        };
        use_type = g_type_register_static(SP_TYPE_OBJECT, "SPTagUse", &use_info, (GTypeFlags)0);
    }
    return use_type;
}

static void
sp_tag_use_class_init(SPTagUseClass *classname)
{
    GObjectClass *gobject_class = (GObjectClass *) classname;
    SPObjectClass *sp_object_class = (SPObjectClass *) classname;

    parent_class = (SPObjectClass*)g_type_class_peek_parent(classname);
    
    gobject_class->finalize = sp_tag_use_finalize;

    sp_object_class->build = sp_tag_use_build;
    sp_object_class->release = sp_tag_use_release;
    sp_object_class->set = sp_tag_use_set;
    sp_object_class->write = sp_tag_use_write;
    sp_object_class->update = sp_tag_use_update;
}

static void
sp_tag_use_init(SPTagUse *use)
{
    use->href = NULL;

    new (&use->_changed_connection) sigc::connection();

    use->ref = new SPTagUseReference(use);

    use->_changed_connection = use->ref->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_tag_use_href_changed), use));
}

static void
sp_tag_use_finalize(GObject *obj)
{
    SPTagUse *use = reinterpret_cast<SPTagUse *>(obj);

    if (use->child) {
        use->detach(use->child);
        use->child = NULL;
    }

    use->ref->detach();
    delete use->ref;
    use->ref = 0;

    use->_changed_connection.~connection();

}

static void
sp_tag_use_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
    if (((SPObjectClass *) parent_class)->build) {
        (* ((SPObjectClass *) parent_class)->build)(object, document, repr);
    }

    object->readAttr( "xlink:href" );

    // We don't need to create child here:
    // reading xlink:href will attach ref, and that will cause the changed signal to be emitted,
    // which will call sp_tag_use_href_changed, and that will take care of the child
}

static void
sp_tag_use_release(SPObject *object)
{
    SPTagUse *use = SP_TAG_USE(object);

    if (use->child) {
        object->detach(use->child);
        use->child = NULL;
    }

    use->_changed_connection.disconnect();

    g_free(use->href);
    use->href = NULL;

    use->ref->detach();

    if (((SPObjectClass *) parent_class)->release) {
        ((SPObjectClass *) parent_class)->release(object);
    }
}

static void
sp_tag_use_set(SPObject *object, unsigned key, gchar const *value)
{
    SPTagUse *use = SP_TAG_USE(object);

    switch (key) {
        case SP_ATTR_XLINK_HREF: {
            if ( value && use->href && ( strcmp(value, use->href) == 0 ) ) {
                /* No change, do nothing. */
            } else {
                g_free(use->href);
                use->href = NULL;
                if (value) {
                    // First, set the href field, because sp_tag_use_href_changed will need it.
                    use->href = g_strdup(value);

                    // Now do the attaching, which emits the changed signal.
                    try {
                        use->ref->attach(Inkscape::URI(value));
                    } catch (Inkscape::BadURIException &e) {
                        g_warning("%s", e.what());
                        use->ref->detach();
                    }
                } else {
                    use->ref->detach();
                }
            }
            break;
        }

        default:
            if (((SPObjectClass *) parent_class)->set) {
                ((SPObjectClass *) parent_class)->set(object, key, value);
            }
            break;
    }
}

static Inkscape::XML::Node *
sp_tag_use_write(SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    SPTagUse *use = SP_TAG_USE(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("inkscape:tagref");
    }

    if (((SPObjectClass *) (parent_class))->write) {
        ((SPObjectClass *) (parent_class))->write(object, xml_doc, repr, flags);
    }

    if (use->ref->getURI()) {
        gchar *uri_string = use->ref->getURI()->toString();
        repr->setAttribute("xlink:href", uri_string);
        g_free(uri_string);
    }

    return repr;
}

/**
 * Returns the ultimate original of a SPTagUse (i.e. the first object in the chain of its originals
 * which is not an SPTagUse). If no original is found, NULL is returned (it is the responsibility
 * of the caller to make sure that this is handled correctly).
 *
 * Note that the returned is the clone object, i.e. the child of an SPTagUse (of the argument one for
 * the trivial case) and not the "true original".
 */
SPItem *
sp_tag_use_root(SPTagUse *use)
{
    SPObject *orig = use->child;
    while (orig && SP_IS_TAG_USE(orig)) {
        orig = SP_TAG_USE(orig)->child;
    }
    if (!orig || !SP_IS_ITEM(orig))
        return NULL;
    return SP_ITEM(orig);
}

static void
sp_tag_use_href_changed(SPObject */*old_ref*/, SPObject */*ref*/, SPTagUse *use)
{
    if (use->href) {
        SPItem *refobj = use->ref->getObject();
        if (refobj) {
            Inkscape::XML::Node *childrepr = refobj->getRepr();
            GType type = sp_repr_type_lookup(childrepr);
            g_return_if_fail(type > G_TYPE_NONE);
            if (g_type_is_a(type, SP_TYPE_ITEM)) {
                use->child = (SPObject*) g_object_new(type, 0);
                use->attach(use->child, use->lastChild());
                sp_object_unref(use->child, use);
                (use->child)->invoke_build(use->document, childrepr, TRUE);

            }
        }
    }
}

static void
sp_tag_use_update(SPObject *object, SPCtx *ctx, unsigned flags)
{
    if (((SPObjectClass *) (parent_class))->update)
        ((SPObjectClass *) (parent_class))->update(object, ctx, flags);
}

SPItem *sp_tag_use_get_original(SPTagUse *use)
{
    SPItem *ref = NULL;
    if (use){
        if (use->ref){
            ref = use->ref->getObject();
        }
    }
    return ref;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
