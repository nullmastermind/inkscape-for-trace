/** @file
 * @gradient stop class.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999,2005 authors
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "sp-stop.h"
#include "style.h"

#include "attributes.h"
#include "streq.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "svg/css-ostringstream.h"
#include "xml/repr.h"

#include "sp-factory.h"

namespace {
	SPObject* createStop() {
		return new SPStop();
	}

	bool stopRegistered = SPFactory::instance().registerObject("svg:stop", createStop);
}

SPStop::SPStop() : SPObject(), CObject(this) {
	delete this->cobject;
	this->cobject = this;

	this->path_string = NULL;

    this->offset = 0.0;
    this->currentColor = false;
    this->specified_color.set( 0x000000ff );
    this->opacity = 1.0;
}

SPStop::~SPStop() {
}

void SPStop::build(SPDocument* doc, Inkscape::XML::Node* repr) {
	SPStop* object = this;

    CObject::build(doc, repr);

    object->readAttr( "offset" );
    object->readAttr( "stop-color" );
    object->readAttr( "stop-opacity" );
    object->readAttr( "style" );
    object->readAttr( "path" ); // For mesh
}

/**
 * Virtual build: set stop attributes from its associated XML node.
 */

void SPStop::set(unsigned int key, const gchar* value) {
	SPStop* object = this;

    SPStop *stop = SP_STOP(object);

    switch (key) {
        case SP_ATTR_STYLE: {
        /** \todo
         * fixme: We are reading simple values 3 times during build (Lauris).
         * \par
         * We need presentation attributes etc.
         * \par
         * remove the hackish "style reading" from here: see comments in
         * sp_object_get_style_property about the bugs in our current
         * approach.  However, note that SPStyle doesn't currently have
         * stop-color and stop-opacity properties.
         */
            {
                gchar const *p = object->getStyleProperty( "stop-color", "black");
                if (streq(p, "currentColor")) {
                    stop->currentColor = true;
                } else {
                    stop->specified_color = SPStop::readStopColor( p );
                }
            }
            {
                gchar const *p = object->getStyleProperty( "stop-opacity", "1");
                gdouble opacity = sp_svg_read_percentage(p, stop->opacity);
                stop->opacity = opacity;
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
        case SP_PROP_STOP_COLOR: {
            {
                gchar const *p = object->getStyleProperty( "stop-color", "black");
                if (streq(p, "currentColor")) {
                    stop->currentColor = true;
                } else {
                    stop->currentColor = false;
                    stop->specified_color = SPStop::readStopColor( p );
                }
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
        case SP_PROP_STOP_OPACITY: {
            {
                gchar const *p = object->getStyleProperty( "stop-opacity", "1");
                gdouble opacity = sp_svg_read_percentage(p, stop->opacity);
                stop->opacity = opacity;
            }
            object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
        case SP_ATTR_OFFSET: {
            stop->offset = sp_svg_read_percentage(value, 0.0);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
        case SP_PROP_STOP_PATH: {
            if (value) {
                stop->path_string = new Glib::ustring( value );
                //Geom::PathVector pv = sp_svg_read_pathv(value);
                //SPCurve *curve = new SPCurve(pv);
                //if( curve ) {
                    // std::cout << "Got Curve" << std::endl;
                    //curve->unref();
                //}
            }
            break;
        }
        default: {
            CObject::set(key, value);
            break;
        }
    }
}

/**
 * Virtual set: set attribute to value.
 */

Inkscape::XML::Node* SPStop::write(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
	SPStop* object = this;

    SPStop *stop = SP_STOP(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:stop");
    }

    Glib::ustring colorStr = stop->specified_color.toString();
    gfloat opacity = stop->opacity;

    CObject::write(xml_doc, repr, flags);

    // Since we do a hackish style setting here (because SPStyle does not support stop-color and
    // stop-opacity), we must do it AFTER calling the parent write method; otherwise
    // sp_object_write would clear our style= attribute (bug 1695287)

    Inkscape::CSSOStringStream os;
    os << "stop-color:";
    if (stop->currentColor) {
        os << "currentColor";
    } else {
        os << colorStr;
    }
    os << ";stop-opacity:" << opacity;
    repr->setAttribute("style", os.str().c_str());
    repr->setAttribute("stop-color", NULL);
    repr->setAttribute("stop-opacity", NULL);
    sp_repr_set_css_double(repr, "offset", stop->offset);
    /* strictly speaking, offset an SVG <number> rather than a CSS one, but exponents make no sense
     * for offset proportions. */

    return repr;
}

/**
 * Virtual write: write object attributes to repr.
 */

// A stop might have some non-stop siblings
SPStop* SPStop::getNextStop()
{
    SPStop *result = 0;

    for (SPObject* obj = getNext(); obj && !result; obj = obj->getNext()) {
        if (SP_IS_STOP(obj)) {
            result = SP_STOP(obj);
        }
    }

    return result;
}

SPStop* SPStop::getPrevStop()
{
    SPStop *result = 0;

    for (SPObject* obj = getPrev(); obj; obj = obj->getPrev()) {
        // The closest previous SPObject that is an SPStop *should* be ourself.
        if (SP_IS_STOP(obj)) {
            SPStop* stop = SP_STOP(obj);
            // Sanity check to ensure we have a proper sibling structure.
            if (stop->getNextStop() == this) {
                result = stop;
            } else {
                g_warning("SPStop previous/next relationship broken");
            }
            break;
        }
    }

    return result;
}

SPColor SPStop::readStopColor( Glib::ustring const &styleStr, guint32 dfl )
{
    SPColor color(dfl);
    SPStyle* style = sp_style_new(0);
    SPIPaint paint;
    paint.read( styleStr.c_str(), *style );
    if ( paint.isColor() ) {
        color = paint.value.color;
    }
    sp_style_unref(style);
    return color;
}

SPColor SPStop::getEffectiveColor() const
{
    SPColor ret;
    if (currentColor) {
        char const *str = getStyleProperty("color", NULL);
        /* Default value: arbitrarily black.  (SVG1.1 and CSS2 both say that the initial
         * value depends on user agent, and don't give any further restrictions that I can
         * see.) */
        ret = readStopColor( str, 0 );
    } else {
        ret = specified_color;
    }
    return ret;
}

/**
 * Return stop's color as 32bit value.
 */
guint32
sp_stop_get_rgba32(SPStop const *const stop)
{
    guint32 rgb0 = 0;
    /* Default value: arbitrarily black.  (SVG1.1 and CSS2 both say that the initial
     * value depends on user agent, and don't give any further restrictions that I can
     * see.) */
    if (stop->currentColor) {
        char const *str = stop->getStyleProperty( "color", NULL);
        if (str) {
            rgb0 = sp_svg_read_color(str, rgb0);
        }
        unsigned const alpha = static_cast<unsigned>(stop->opacity * 0xff + 0.5);
        g_return_val_if_fail((alpha & ~0xff) == 0,
                             rgb0 | 0xff);
        return rgb0 | alpha;
    } else {
        return stop->specified_color.toRGBA32( stop->opacity );
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
