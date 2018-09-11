// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <desc> implementation
 *
 * Authors:
 *   Jeff Schiller <codedread@gmail.com>
 *
 * Copyright (C) 2008 Jeff Schiller
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-desc.h"
#include "xml/repr.h"

SPDesc::SPDesc() : SPObject() {
}

SPDesc::~SPDesc() = default;

/**
 * Writes it's settings to an incoming repr object, if any.
 */
Inkscape::XML::Node* SPDesc::write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, guint flags) {
    if (!repr) {
        repr = this->getRepr()->duplicate(doc);
    }

    SPObject::write(doc, repr, flags);

    return repr;
}
