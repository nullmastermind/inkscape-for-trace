// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_USE_REFERENCE_H
#define SEEN_SP_USE_REFERENCE_H

/*
 * The reference corresponding to href of <use> element.
 *
 * Copyright (C) 2004 Bulia Byak
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>

#include "sp-item.h"
#include "uri-references.h"

class SPCurve;

namespace Inkscape {
namespace XML {
class Node;
}
}


class SPUseReference : public Inkscape::URIReference {
public:
    SPUseReference(SPObject *owner) : URIReference(owner) {}

    SPItem *getObject() const {
        return static_cast<SPItem *>(URIReference::getObject());
    }

protected:
    bool _acceptObject(SPObject * const obj) const override;

};


class SPUsePath : public SPUseReference {
public:
    SPCurve *originalPath;
    bool sourceDirty;

    SPObject            *owner;
    char                *sourceHref;
    Inkscape::XML::Node *sourceRepr;
    SPObject            *sourceObject;

    sigc::connection _modified_connection;
    sigc::connection _delete_connection;
    sigc::connection _changed_connection;
    sigc::connection _transformed_connection;

    SPUsePath(SPObject* i_owner);
    ~SPUsePath() override;

    void link(char* to);
    void unlink();
    void start_listening(SPObject* to);
    void quit_listening();
    void refresh_source();

    void (*user_unlink) (SPObject *user);
};

#endif /* !SEEN_SP_USE_REFERENCE_H */

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
