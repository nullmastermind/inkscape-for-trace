// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "sp-stop.h"
#include "style.h"

#include "attributes.h"
#include "streq.h"
#include "svg/svg.h"
#include "svg/svg-color.h"
#include "svg/css-ostringstream.h"

SPStop::SPStop() : SPObject() {
    this->path_string = nullptr;
    this->offset = 0.0;
}

SPStop::~SPStop() = default;

void SPStop::build(SPDocument* doc, Inkscape::XML::Node* repr) {
    SPObject::build(doc, repr);

    this->readAttr( "style" );
    this->readAttr( "offset" );
    this->readAttr( "path" ); // For mesh
    SPObject::build(doc, repr);
}

/**
 * Virtual build: set stop attributes from its associated XML node.
 */

void SPStop::set(SPAttributeEnum key, const gchar* value) {
    switch (key) {
        case SP_ATTR_OFFSET: {
            this->offset = sp_svg_read_percentage(value, 0.0);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
        case SP_PROP_STOP_PATH: {
            if (value) {
                this->path_string = new Glib::ustring( value );
                //Geom::PathVector pv = sp_svg_read_pathv(value);
                //SPCurve *curve = new SPCurve(pv);
                //if( curve ) {
                    // std::cout << "Got Curve" << std::endl;
                    //curve->unref();
                //}
                this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
            break;
        }
        default: {
            if (SP_ATTRIBUTE_IS_CSS(key)) {
                this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            } else {
                SPObject::set(key, value);
            }
            // This makes sure that the parent sp-gradient is updated.
            this->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
            break;
        }
    }
}

void SPStop::modified(guint flags)
{
    if (parent && !(flags & SP_OBJECT_PARENT_MODIFIED_FLAG)) {
        parent->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
    }
}


/**
 * Virtual set: set attribute to value.
 */

Inkscape::XML::Node* SPStop::write(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:stop");
    }

    SPObject::write(xml_doc, repr, flags);
    sp_repr_set_css_double(repr, "offset", this->offset);
    /* strictly speaking, offset an SVG <number> rather than a CSS one, but exponents make no sense
     * for offset proportions. */

    return repr;
}

/**
 * Virtual write: write object attributes to repr.
 */

// A stop might have some non-stop siblings
SPStop* SPStop::getNextStop() {
    SPStop *result = nullptr;

    for (SPObject* obj = getNext(); obj && !result; obj = obj->getNext()) {
        if (SP_IS_STOP(obj)) {
            result = SP_STOP(obj);
        }
    }

    return result;
}

SPStop* SPStop::getPrevStop() {
    SPStop *result = nullptr;

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

SPColor SPStop::getColor() const
{
    if (style->stop_color.currentcolor) {
        return style->color.value.color;
    }
    Glib::ustring color = style->stop_color.value.color.toString();
    return style->stop_color.value.color;
}

gfloat SPStop::getOpacity() const
{
    return SP_SCALE24_TO_FLOAT(style->stop_opacity.value);
}

/**
 * Return stop's color as 32bit value.
 */
guint32 SPStop::get_rgba32() const
{
    return getColor().toRGBA32(getOpacity());
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
