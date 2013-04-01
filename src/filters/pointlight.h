#ifndef SP_FEPOINTLIGHT_H_SEEN
#define SP_FEPOINTLIGHT_H_SEEN

/** \file
 * SVG <filter> implementation, see sp-filter.cpp.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_FEPOINTLIGHT (sp_fepointlight_get_type())
#define SP_FEPOINTLIGHT(obj) ((SPFePointLight*)obj)
#define SP_IS_FEPOINTLIGHT(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFePointLight)))

class CFePointLight;

class SPFePointLight : public SPObject {
public:
	CFePointLight* cfepointlight;

    /** x coordinate of the light source */
    gfloat x; 
    guint x_set : 1;
    /** y coordinate of the light source */
    gfloat y; 
    guint y_set : 1;
    /** z coordinate of the light source */
    gfloat z; 
    guint z_set : 1;

    //other fields
};

class CFePointLight : public CObject {
public:
	CFePointLight(SPFePointLight* pointlight);
	virtual ~CFePointLight();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void release();

	virtual void set(unsigned int key, const gchar* value);

	virtual void update(SPCtx* ctx, unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

private:
	SPFePointLight* spfepointlight;
};

struct SPFePointLightClass {
    SPObjectClass parent_class;
};

GType
sp_fepointlight_get_type();
#endif /* !SP_FEPOINTLIGHT_H_SEEN */

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
