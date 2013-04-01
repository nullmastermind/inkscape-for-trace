/** \file
 * SVG <feTile> implementation.
 *
 */
/*
 * Authors:
 *   hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "attributes.h"
#include "svg/svg.h"
#include "filters/tile.h"
#include "xml/repr.h"
#include "display/nr-filter.h"
#include "display/nr-filter-tile.h"

/* FeTile base class */
G_DEFINE_TYPE(SPFeTile, sp_feTile, G_TYPE_OBJECT);

static void
sp_feTile_class_init(SPFeTileClass *klass)
{
}

CFeTile::CFeTile(SPFeTile* tile) : CFilterPrimitive(tile) {
	this->spfetile = tile;
}

CFeTile::~CFeTile() {
}

SPFeTile::SPFeTile() : SPFilterPrimitive() {
	SPFeTile* feTile = this;

	feTile->cfetile = new CFeTile(feTile);
	feTile->typeHierarchy.insert(typeid(SPFeTile));

	delete feTile->cfilterprimitive;
	feTile->cfilterprimitive = feTile->cfetile;
	feTile->cobject = feTile->cfetile;
}

static void
sp_feTile_init(SPFeTile *feTile)
{
	new (feTile) SPFeTile();
}

/**
 * Reads the Inkscape::XML::Node, and initializes SPFeTile variables.  For this to get called,
 * our name must be associated with a repr via "sp_object_type_register".  Best done through
 * sp-object-repr.cpp's repr_name_entries array.
 */
void CFeTile::build(SPDocument *document, Inkscape::XML::Node *repr) {
	CFilterPrimitive::build(document, repr);
}

/**
 * Drops any allocated memory.
 */
void CFeTile::release() {
	CFilterPrimitive::release();
}

/**
 * Sets a specific value in the SPFeTile.
 */
void CFeTile::set(unsigned int key, gchar const *value) {
	SPFeTile* object = this->spfetile;

    SPFeTile *feTile = SP_FETILE(object);
    (void)feTile;

    switch(key) {
	/*DEAL WITH SETTING ATTRIBUTES HERE*/
        default:
        	CFilterPrimitive::set(key, value);
            break;
    }
}

/**
 * Receives update notifications.
 */
void CFeTile::update(SPCtx *ctx, guint flags) {
	SPFeTile* object = this->spfetile;

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG |
                 SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {

        /* do something to trigger redisplay, updates? */

    }

    CFilterPrimitive::update(ctx, flags);
}

/**
 * Writes its settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* CFeTile::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags) {
	SPFeTile* object = this->spfetile;

    /* TODO: Don't just clone, but create a new repr node and write all
     * relevant values into it */
    if (!repr) {
        repr = object->getRepr()->duplicate(doc);
    }

    CFilterPrimitive::write(doc, repr, flags);

    return repr;
}

void CFeTile::build_renderer(Inkscape::Filters::Filter* filter) {
	SPFeTile* primitive = this->spfetile;

    g_assert(primitive != NULL);
    g_assert(filter != NULL);

    SPFeTile *sp_tile = SP_FETILE(primitive);
    (void)sp_tile;

    int primitive_n = filter->add_primitive(Inkscape::Filters::NR_FILTER_TILE);
    Inkscape::Filters::FilterPrimitive *nr_primitive = filter->get_primitive(primitive_n);
    Inkscape::Filters::FilterTile *nr_tile = dynamic_cast<Inkscape::Filters::FilterTile*>(nr_primitive);
    g_assert(nr_tile != NULL);

    sp_filter_primitive_renderer_common(primitive, nr_primitive);
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
