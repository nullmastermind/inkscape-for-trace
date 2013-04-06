/*
 * Copyright (C) Johan Engelen 2007-2008 <j.b.c.engelen@utwente.nl>
 *   Abhishek Sharma
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpeobject.h"

#include "live_effects/effect.h"

#include "xml/repr.h"
#include "xml/node-event-vector.h"
#include "sp-object.h"
#include "attributes.h"
#include "document.h"
#include "document-private.h"

#include <glibmm/i18n.h>

//#define LIVEPATHEFFECT_VERBOSE

#include "sp-factory.h"

namespace {
	SPObject* createLivepatheffect() {
		return new LivePathEffectObject();
	}

	bool livepatheffectRegistered = SPFactory::instance().registerObject("inkscape:path-effect", createLivepatheffect);
}

static void livepatheffect_on_repr_attr_changed (Inkscape::XML::Node * repr, const gchar *key, const gchar *oldval, const gchar *newval, bool is_interactive, void * data);

static Inkscape::XML::NodeEventVector const livepatheffect_repr_events = {
    NULL, /* child_added */
    NULL, /* child_removed */
    livepatheffect_on_repr_attr_changed,
    NULL, /* content_changed */
    NULL  /* order_changed */
};


LivePathEffectObject::LivePathEffectObject() : SPObject(), CObject(this) {
#ifdef LIVEPATHEFFECT_VERBOSE
    g_message("Init livepatheffectobject");
#endif

    delete this->cobject;
    this->cobject = this;

    this->effecttype = Inkscape::LivePathEffect::INVALID_LPE;
    this->lpe = NULL;

    this->effecttype_set = false;
}

LivePathEffectObject::~LivePathEffectObject() {
}

/**
 * Virtual build: set livepatheffect attributes from its associated XML node.
 */
void LivePathEffectObject::build(SPDocument *document, Inkscape::XML::Node *repr) {
	LivePathEffectObject* object = this;

    g_assert(object != NULL);
    g_assert(SP_IS_OBJECT(object));

    CObject::build(document, repr);

    object->readAttr( "effect" );

    if (repr) {
        repr->addListener (&livepatheffect_repr_events, object);
    }

    /* Register ourselves, is this necessary? */
//    document->addResource("path-effect", object);
}

/**
 * Virtual release of livepatheffect members before destruction.
 */
void LivePathEffectObject::release() {
	LivePathEffectObject* object = this;

    LivePathEffectObject *lpeobj = LIVEPATHEFFECT(object);

    object->getRepr()->removeListenerByData(object);


/*
    if (object->document) {
        // Unregister ourselves
        sp_document_removeResource(object->document, "livepatheffect", object);
    }

    if (gradient->ref) {
        gradient->modified_connection.disconnect();
        gradient->ref->detach();
        delete gradient->ref;
        gradient->ref = NULL;
    }

    gradient->modified_connection.~connection();

*/

    if (lpeobj->lpe) {
        delete lpeobj->lpe;
        lpeobj->lpe = NULL;
    }
    lpeobj->effecttype = Inkscape::LivePathEffect::INVALID_LPE;

    CObject::release();
}

/**
 * Virtual set: set attribute to value.
 */
void LivePathEffectObject::set(unsigned key, gchar const *value) {
	LivePathEffectObject* object = this;

    LivePathEffectObject *lpeobj = LIVEPATHEFFECT(object);
#ifdef LIVEPATHEFFECT_VERBOSE
    g_print("Set livepatheffect");
#endif
    switch (key) {
        case SP_PROP_PATH_EFFECT:
            if (lpeobj->lpe) {
                delete lpeobj->lpe;
                lpeobj->lpe = NULL;
            }

            if ( value && Inkscape::LivePathEffect::LPETypeConverter.is_valid_key(value) ) {
                lpeobj->effecttype = Inkscape::LivePathEffect::LPETypeConverter.get_id_from_key(value);
                lpeobj->lpe = Inkscape::LivePathEffect::Effect::New(lpeobj->effecttype, lpeobj);
                lpeobj->effecttype_set = true;
            } else {
                lpeobj->effecttype = Inkscape::LivePathEffect::INVALID_LPE;
                lpeobj->effecttype_set = false;
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    }

    CObject::set(key, value);
}

/**
 * Virtual write: write object attributes to repr.
 */
Inkscape::XML::Node* LivePathEffectObject::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	LivePathEffectObject* object = this;

    LivePathEffectObject *lpeobj = LIVEPATHEFFECT(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("inkscape:path-effect");
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || lpeobj->lpe) {
        repr->setAttribute("effect", Inkscape::LivePathEffect::LPETypeConverter.get_key(lpeobj->effecttype).c_str());

        lpeobj->lpe->writeParamsToSVG();
    }

    CObject::write(xml_doc, repr, flags);

    return repr;
}

static void
livepatheffect_on_repr_attr_changed ( Inkscape::XML::Node * /*repr*/,
                                      const gchar *key,
                                      const gchar */*oldval*/,
                                      const gchar *newval,
                                      bool /*is_interactive*/,
                                      void * data )
{
#ifdef LIVEPATHEFFECT_VERBOSE
    g_print("livepatheffect_on_repr_attr_changed");
#endif

    if (!data)
        return;

    LivePathEffectObject *lpeobj = (LivePathEffectObject*) data;
    if (!lpeobj->get_lpe())
        return;

    lpeobj->get_lpe()->setParameter(key, newval);

    lpeobj->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * If this has other users, create a new private duplicate and return it
 * returns 'this' when no forking was necessary (and therefore no duplicate was made)
 * Check out sp_lpe_item_fork_path_effects_if_necessary !
 */
LivePathEffectObject *LivePathEffectObject::fork_private_if_necessary(unsigned int nr_of_allowed_users)
{
    if (hrefcount > nr_of_allowed_users) {
        SPDocument *doc = this->document;
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *dup_repr = this->getRepr()->duplicate(xml_doc);

        doc->getDefs()->getRepr()->addChild(dup_repr, NULL);
        LivePathEffectObject *lpeobj_new = LIVEPATHEFFECT( doc->getObjectByRepr(dup_repr) );

        Inkscape::GC::release(dup_repr);
        return lpeobj_new;
    }
    return this;
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
