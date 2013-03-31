/*
 * SVG <polyline> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "config.h"

#include "attributes.h"
#include "sp-polyline.h"
#include "display/curve.h"
#include <glibmm/i18n.h>
#include "xml/repr.h"
#include "document.h"


G_DEFINE_TYPE(SPPolyLine, sp_polyline, SP_TYPE_SHAPE);

static void
sp_polyline_class_init(SPPolyLineClass *klass)
{
}

CPolyLine::CPolyLine(SPPolyLine* polyline) : CShape(polyline) {
	this->sppolyline = polyline;
}

CPolyLine::~CPolyLine() {
}

void sp_polyline_init(SPPolyLine * polyline)
{
	polyline->cpolyline = new CPolyLine(polyline);
	polyline->typeHierarchy.insert(typeid(SPPolyLine));

	delete polyline->cshape;
	polyline->cshape = polyline->cpolyline;
	polyline->clpeitem = polyline->cpolyline;
	polyline->citem = polyline->cpolyline;
	polyline->cobject = polyline->cpolyline;
}

void CPolyLine::build(SPDocument * document, Inkscape::XML::Node * repr) {
	SPPolyLine* object = this->sppolyline;

    CShape::build(document, repr);

    object->readAttr( "points" );
}

void CPolyLine::set(unsigned int key, const gchar* value) {
	SPPolyLine* object = this->sppolyline;
    SPPolyLine *polyline = object;

    switch (key) {
	case SP_ATTR_POINTS: {
            SPCurve * curve;
            const gchar * cptr;
            char * eptr;
            gboolean hascpt;

            if (!value) break;
            curve = new SPCurve ();
            hascpt = FALSE;

            cptr = value;
            eptr = NULL;

            while (TRUE) {
                gdouble x, y;

                while (*cptr != '\0' && (*cptr == ',' || *cptr == '\x20' || *cptr == '\x9' || *cptr == '\xD' || *cptr == '\xA')) {
                    cptr++;
                }
                if (!*cptr) break;

                x = g_ascii_strtod (cptr, &eptr);
                if (eptr == cptr) break;
                cptr = eptr;

                while (*cptr != '\0' && (*cptr == ',' || *cptr == '\x20' || *cptr == '\x9' || *cptr == '\xD' || *cptr == '\xA')) {
                    cptr++;
                }
                if (!*cptr) break;

                y = g_ascii_strtod (cptr, &eptr);
                if (eptr == cptr) break;
                cptr = eptr;
                if (hascpt) {
                    curve->lineto(x, y);
                } else {
                    curve->moveto(x, y);
                    hascpt = TRUE;
                }
            }
		
            (SP_SHAPE (polyline))->setCurve (curve, TRUE);
            curve->unref();
            break;
	}
	default:
            CShape::set(key, value);
            break;
    }
}

Inkscape::XML::Node* CPolyLine::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPPolyLine* object = this->sppolyline;

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:polyline");
    }

    if (repr != object->getRepr()) {
        repr->mergeFrom(object->getRepr(), "id");
    }

    CShape::write(xml_doc, repr, flags);

    return repr;
}

gchar* CPolyLine::description() {
	return g_strdup(_("<b>Polyline</b>"));
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
