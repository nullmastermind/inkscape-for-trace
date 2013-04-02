/*
 * SVG <path> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   David Turner <novalis@gnu.org>
 *   Abhishek Sharma
 *   Johan Engelen
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 1999-2012 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glibmm/i18n.h>

#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "live_effects/lpeobject-reference.h"
#include "sp-lpe-item.h"

#include "display/curve.h"
#include <2geom/pathvector.h>
#include <2geom/bezier-curve.h>
#include <2geom/hvlinesegment.h>
#include "helper/geom-curves.h"

#include "svg/svg.h"
#include "xml/repr.h"
#include "attributes.h"

#include "sp-path.h"
#include "sp-guide.h"

#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-style.h"
#include "event-context.h"
#include "inkscape.h"
#include "style.h"
#include "message-stack.h"
#include "selection.h"

#define noPATH_VERBOSE

#include "sp-factory.h"

namespace {
	SPObject* createPath() {
		return new SPPath();
	}

	bool pathRegistered = SPFactory::instance().registerObject("svg:path", createPath);
}


static void sp_path_finalize(GObject *obj);

G_DEFINE_TYPE(SPPath, sp_path, G_TYPE_OBJECT);

/**
 *  Does the object-oriented work of initializing the class structure
 *  including parent class, and registers function pointers for
 *  the functions build, set, write, and set_transform.
 */
static void
sp_path_class_init(SPPathClass * klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;
    gobject_class->finalize = sp_path_finalize;
}


gint SPPath::nodesInPath() const
{
    return _curve ? _curve->nodes_in_path() : 0;
}

gchar* CPath::description() {
	SPPath* item = this->sppath;

    int count = SP_PATH(item)->nodesInPath();
    if (SP_IS_LPE_ITEM(item) && sp_lpe_item_has_path_effect(SP_LPE_ITEM(item))) {

        Glib::ustring s;

        PathEffectList effect_list =  sp_lpe_item_get_effect_list(SP_LPE_ITEM(item));
        for (PathEffectList::iterator it = effect_list.begin(); it != effect_list.end(); ++it)
        {
            LivePathEffectObject *lpeobj = (*it)->lpeobject;
            if (!lpeobj || !lpeobj->get_lpe())
                break;
            if (s.empty())
                s = lpeobj->get_lpe()->getName();
            else
                s = s + ", " + lpeobj->get_lpe()->getName();
        }

        return g_strdup_printf(ngettext("<b>Path</b> (%i node, path effect: %s)",
                                        "<b>Path</b> (%i nodes, path effect: %s)",count), count, s.c_str());
    } else {
        return g_strdup_printf(ngettext("<b>Path</b> (%i node)",
                                        "<b>Path</b> (%i nodes)",count), count);
    }
}

void CPath::convert_to_guides() {
	SPPath* item = this->sppath;
    SPPath *path = item;

    if (!path->_curve) {
        return;
    }

    std::list<std::pair<Geom::Point, Geom::Point> > pts;

    Geom::Affine const i2dt(path->i2dt_affine());

    Geom::PathVector const & pv = path->_curve->get_pathvector();
    for(Geom::PathVector::const_iterator pit = pv.begin(); pit != pv.end(); ++pit) {
        for(Geom::Path::const_iterator cit = pit->begin(); cit != pit->end_default(); ++cit) {
            // only add curves for straight line segments
            if( is_straight_curve(*cit) )
            {
                pts.push_back(std::make_pair(cit->initialPoint() * i2dt, cit->finalPoint() * i2dt));
            }
        }
    }

    sp_guide_pt_pairs_to_guides(item->document, pts);
}

CPath::CPath(SPPath* path) : CShape(path) {
	this->sppath = path;
}

CPath::~CPath() {
}

SPPath::SPPath() : SPShape(), connEndPair(this) {
	SPPath* path = this;

	path->cpath = new CPath(path);
	path->typeHierarchy.insert(typeid(SPPath));

	delete path->cshape;
	path->cshape = path->cpath;
	path->clpeitem = path->cpath;
	path->citem = path->cpath;
	path->cobject = path->cpath;

    //new (&path->connEndPair) SPConnEndPair(path);
}

/**
 * Initializes an SPPath.
 */
static void
sp_path_init(SPPath *path)
{
	new (path) SPPath();
}

static void
sp_path_finalize(GObject *obj)
{
    SPPath *path = (SPPath *) obj;

    path->connEndPair.~SPConnEndPair();
}

void CPath::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPPath* object = this->sppath;

    /* Are these calls actually necessary? */
    object->readAttr( "marker" );
    object->readAttr( "marker-start" );
    object->readAttr( "marker-mid" );
    object->readAttr( "marker-end" );

    sp_conn_end_pair_build(object);

    CShape::build(document, repr);

    object->readAttr( "inkscape:original-d" );
    object->readAttr( "d" );

    /* d is a required attribute */
    gchar const *d = object->getAttribute("d", NULL);
    if (d == NULL) {
        object->setKeyValue( sp_attribute_lookup("d"), "");
    }
}

void CPath::release() {
	SPPath* object = this->sppath;
    SPPath *path = object;

    path->connEndPair.release();

    CShape::release();
}


void CPath::set(unsigned int key, const gchar* value) {
	SPPath* object = this->sppath;
    SPPath *path = (SPPath *) object;

    switch (key) {
        case SP_ATTR_INKSCAPE_ORIGINAL_D:
                if (value) {
                    Geom::PathVector pv = sp_svg_read_pathv(value);
                    SPCurve *curve = new SPCurve(pv);
                    if (curve) {
                        path->set_original_curve(curve, TRUE, true);
                        curve->unref();
                    }
                } else {
                    path->set_original_curve(NULL, TRUE, true);
                }
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
       case SP_ATTR_D:
                if (value) {
                    Geom::PathVector pv = sp_svg_read_pathv(value);
                    SPCurve *curve = new SPCurve(pv);
                    if (curve) {
                        ((SPShape *) path)->setCurve(curve, TRUE);
                        curve->unref();
                    }
                } else {
                    ((SPShape *) path)->setCurve(NULL, TRUE);
                }
                object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_PROP_MARKER:
        case SP_PROP_MARKER_START:
        case SP_PROP_MARKER_MID:
        case SP_PROP_MARKER_END:
            sp_shape_set_marker(object, key, value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_CONNECTOR_TYPE:
        case SP_ATTR_CONNECTOR_CURVATURE:
        case SP_ATTR_CONNECTION_START:
        case SP_ATTR_CONNECTION_END:
        case SP_ATTR_CONNECTION_START_POINT:
        case SP_ATTR_CONNECTION_END_POINT:
            path->connEndPair.setAttr(key, value);
            break;
        default:
            CShape::set(key, value);
            break;
    }
}

Inkscape::XML::Node* CPath::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPPath* object = this->sppath;
	SPShape *shape = (SPShape *) object;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:path");
    }

#ifdef PATH_VERBOSE
g_message("sp_path_write writes 'd' attribute");
#endif
    if ( shape->_curve != NULL ) {
        gchar *str = sp_svg_write_path(shape->_curve->get_pathvector());
        repr->setAttribute("d", str);
        g_free(str);
    } else {
        repr->setAttribute("d", NULL);
    }

    if (flags & SP_OBJECT_WRITE_EXT) {
        if ( shape->_curve_before_lpe != NULL ) {
            gchar *str = sp_svg_write_path(shape->_curve_before_lpe->get_pathvector());
            repr->setAttribute("inkscape:original-d", str);
            g_free(str);
        } else {
            repr->setAttribute("inkscape:original-d", NULL);
        }
    }

    SP_PATH(shape)->connEndPair.writeRepr(repr);

    CShape::write(xml_doc, repr, flags);

    return repr;
}

void CPath::update(SPCtx *ctx, guint flags) {
	SPPath* object = this->sppath;

	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
	        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B; // since we change the description, it's not a "just translation" anymore
	    }

	    CShape::update(ctx, flags);

	    SPPath *path = SP_PATH(object);
	    path->connEndPair.update();
}


Geom::Affine CPath::set_transform(Geom::Affine const &transform) {
	SPPath* item = this->sppath;

    if (!SP_IS_PATH(item)) {
        return Geom::identity();
    }
    SPPath *path = SP_PATH(item);

    if (!path->_curve) { // 0 nodes, nothing to transform
        return Geom::identity();
    }

    // Transform the original-d path if this is a valid LPE item, other else the (ordinary) path
    if (path->_curve_before_lpe && sp_lpe_item_has_path_effect_recursive(SP_LPE_ITEM(item))) {
        if (sp_lpe_item_has_path_effect_of_type(SP_LPE_ITEM(item), Inkscape::LivePathEffect::CLONE_ORIGINAL)) {
            // if path has the CLONE_ORIGINAL LPE applied, don't write the transform to the pathdata, but write it 'unoptimized'
            return transform;
        } else {
            path->_curve_before_lpe->transform(transform);
        }
    } else {
        path->_curve->transform(transform);
    }

    // Adjust stroke
    item->adjust_stroke(transform.descrim());

    // Adjust pattern fill
    item->adjust_pattern(transform);

    // Adjust gradient fill
    item->adjust_gradient(transform);

    // Adjust LPE
    item->adjust_livepatheffect(transform);

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);

    // nothing remains - we've written all of the transform, so return identity
    return Geom::identity();
}


void CPath::update_patheffect(bool write) {
	SPPath* lpeitem = this->sppath;
    SPShape * const shape = (SPShape *) lpeitem;

    Inkscape::XML::Node *repr = shape->getRepr();

#ifdef PATH_VERBOSE
g_message("sp_path_update_patheffect");
#endif

    if (shape->_curve_before_lpe && sp_lpe_item_has_path_effect_recursive(lpeitem)) {
        SPCurve *curve = shape->_curve_before_lpe->copy();
        /* if a path has an lpeitem applied, then reset the curve to the _curve_before_lpe.
         * This is very important for LPEs to work properly! (the bbox might be recalculated depending on the curve in shape)*/
        shape->setCurveInsync(curve, TRUE);

        bool success = sp_lpe_item_perform_path_effect(SP_LPE_ITEM(shape), curve);
        if (success && write) {
            // could also do shape->getRepr()->updateRepr();  but only the d attribute needs updating.
#ifdef PATH_VERBOSE
g_message("sp_path_update_patheffect writes 'd' attribute");
#endif
            if ( shape->_curve != NULL ) {
                gchar *str = sp_svg_write_path(shape->_curve->get_pathvector());
                repr->setAttribute("d", str);
                g_free(str);
            } else {
                repr->setAttribute("d", NULL);
            }
        } else if (!success) {
            // LPE was unsuccesfull. Read the old 'd'-attribute.
            if (gchar const * value = repr->attribute("d")) {
                Geom::PathVector pv = sp_svg_read_pathv(value);
                SPCurve *oldcurve = new SPCurve(pv);
                if (oldcurve) {
                    shape->setCurve(oldcurve, TRUE);
                    oldcurve->unref();
                }
            }
        }
        shape->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        curve->unref();
    }
}


/**
 * Adds a original_curve to the path.  If owner is specified, a reference
 * will be made, otherwise the curve will be copied into the path.
 * Any existing curve in the path will be unreferenced first.
 * This routine triggers reapplication of an effect if present
 * and also triggers a request to update the display. Does not write
 * result to XML when write=false.
 */
void SPPath::set_original_curve (SPCurve *new_curve, unsigned int owner, bool write)
{
    if (_curve_before_lpe) {
        _curve_before_lpe = _curve_before_lpe->unref();
    }
    if (new_curve) {
        if (owner) {
            _curve_before_lpe = new_curve->ref();
        } else {
            _curve_before_lpe = new_curve->copy();
        }
    }
    sp_lpe_item_update_patheffect(this, true, write);
    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Return duplicate of _curve_before_lpe (if any exists) or NULL if there is no curve
 */
SPCurve * SPPath::get_original_curve () const
{
    if (_curve_before_lpe) {
        return _curve_before_lpe->copy();
    }
    return NULL;
}

/**
 * Return duplicate of edittable curve which is _curve_before_lpe if it exists or
 * shape->curve if not.
 */
SPCurve* SPPath::get_curve_for_edit () const
{
    if (_curve_before_lpe && sp_lpe_item_has_path_effect_recursive(SP_LPE_ITEM(this))) {
        return get_original_curve();
    } else {
        return getCurve();
    }
}

/**
 * Returns \c _curve_before_lpe if it is not NULL and a valid LPE is applied or
 * \c curve if not.
 */
const SPCurve* SPPath::get_curve_reference () const
{
    if (_curve_before_lpe && sp_lpe_item_has_path_effect_recursive(SP_LPE_ITEM(this))) {
        return _curve_before_lpe;
    } else {
        return _curve;
    }
}

/**
 * Returns \c _curve_before_lpe if it is not NULL and a valid LPE is applied or \c curve if not.
 * \todo should only be available to class friends!
 */
SPCurve* SPPath::get_curve ()
{
    if (_curve_before_lpe && sp_lpe_item_has_path_effect_recursive(SP_LPE_ITEM(this))) {
        return _curve_before_lpe;
    } else {
        return _curve;
    }
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
