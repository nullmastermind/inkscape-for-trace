// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SHAPE_REFERENCE_H
#define SEEN_SP_SHAPE_REFERENCE_H

/*
 * Reference class for shapes (SVG 2 text).
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2010 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "uri-references.h"

#include "sp-shape.h"

class SPDocument;
class SPObject;

class SPShapeReference : public Inkscape::URIReference {
public:
    ~SPShapeReference() override;
    SPShapeReference(SPObject *obj);
    SPShape *getObject() const {
        return static_cast<SPShape *>(URIReference::getObject());
    }

protected:
    bool _acceptObject(SPObject *obj) const override {
        return SP_IS_SHAPE(obj) && URIReference::_acceptObject(obj);
    };

  private:
    void on_shape_modified(SPObject *, unsigned flags);

    sigc::connection _shape_modified_connection;
    sigc::connection _owner_release_connection;
};

#endif // SEEN_SP_SHAPE_REFERENCE_H
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
