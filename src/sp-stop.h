#ifndef SEEN_SP_STOP_H
#define SEEN_SP_STOP_H

/** \file
 * SPStop: SVG <stop> implementation.
 */
/*
 * Authors?
 */

#include "sp-object.h"
#include "color.h"
#include <glib.h>

namespace Glib {
class ustring;
}

#define SP_TYPE_STOP (sp_stop_get_type())
#define SP_STOP(obj) ((SPStop*)obj)
#define SP_IS_STOP(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPStop)))

GType sp_stop_get_type();

class CStop;

/** Gradient stop. */
class SPStop : public SPObject {
public:
	SPStop();
	CStop* cstop;

    /// \todo fixme: Should be SPSVGPercentage
    gfloat offset;

    bool currentColor;

    /** \note
     * N.B.\ Meaningless if currentColor is true.  Use sp_stop_get_rgba32 or sp_stop_get_color
     * (currently static in sp-gradient.cpp) if you want the effective color.
     */
    SPColor specified_color;

    /// \todo fixme: Implement SPSVGNumber or something similar.
    gfloat opacity;

    Glib::ustring * path_string;
    //SPCurve path;

    static SPColor readStopColor( Glib::ustring const &styleStr, guint32 dfl = 0 );

    SPStop* getNextStop();
    SPStop* getPrevStop();

    SPColor getEffectiveColor() const;

};

/// The SPStop vtable.
struct SPStopClass {
    SPObjectClass parent_class;
};


class CStop : public CObject {
public:
	CStop(SPStop* stop);
	virtual ~CStop();

	virtual void build(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void set(unsigned int key, const gchar* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags);

protected:
	SPStop* spstop;
};


guint32 sp_stop_get_rgba32(SPStop const *);


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
