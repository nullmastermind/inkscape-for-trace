// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Reference class for shapes (SVG 2 text).
 *
 * Copyright (C) 2020 Authors
 */

#include "sp-shape-reference.h"
#include "object/sp-text.h"

SPShapeReference::SPShapeReference(SPObject *obj)
    : URIReference(obj)
{
    // The text object can be detached from the document but still be
    // referenced, and its style (who's the owner of the SPShapeReference)
    // can also still be referenced even after the object got destroyed.
    _owner_release_connection = obj->connectRelease([this](SPObject *text_object) {
        assert(text_object == this->getOwner());

        // Fully detach to prevent reconnecting with a shape's modified signal
        this->detach();

        this->_owner_release_connection.disconnect();
    });

    // https://www.w3.org/TR/SVG/text.html#TextShapeInside
    // Applies to: 'text' elements
    // Inherited: no
    if (!dynamic_cast<SPText *>(obj)) {
        g_warning("shape reference on non-text object: %s", typeid(*obj).name());
        return;
    }

    // Listen to the shape's modified event to keep the text layout updated
    changedSignal().connect([this](SPObject *, SPObject *shape_object) {
        this->_shape_modified_connection.disconnect();

        if (shape_object) {
            this->_shape_modified_connection =
                shape_object->connectModified(sigc::mem_fun(*this, &SPShapeReference::on_shape_modified));
        }
    });
}

SPShapeReference::~SPShapeReference()
{ //
    _shape_modified_connection.disconnect();
    _owner_release_connection.disconnect();
}

/**
 * Slot to connect to the shape's modified signal. Requests display update of the text object.
 */
void SPShapeReference::on_shape_modified(SPObject *shape_object, unsigned flags)
{
    auto *text_object = getOwner();

    assert(text_object);
    assert(shape_object == getObject());

    if ((flags & SP_OBJECT_MODIFIED_FLAG)) {
        text_object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
    }
}

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :
