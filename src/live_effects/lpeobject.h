// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_OBJECT_H
#define INKSCAPE_LIVEPATHEFFECT_OBJECT_H

/*
 * Inkscape::LivePathEffect
 *
 * Copyright (C) Johan Engelen 2007-2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "effect-enum.h"

#include "object/sp-object.h"

namespace Inkscape {
    namespace XML {
        class Node;
        struct Document;
    }
    namespace LivePathEffect {
        class Effect;
    }
}

#define LIVEPATHEFFECT(obj) ((LivePathEffectObject*)obj)
#define IS_LIVEPATHEFFECT(obj) (dynamic_cast<const LivePathEffectObject*>((SPObject*)obj))

class LivePathEffectObject : public SPObject {
public:
	LivePathEffectObject();
	~LivePathEffectObject() override;

    Inkscape::LivePathEffect::EffectType effecttype;

    bool effecttype_set;
    // dont check values only structure and ID
    bool is_similar(LivePathEffectObject *that);

    LivePathEffectObject * fork_private_if_necessary(unsigned int nr_of_allowed_users = 1);

    /* Note that the returned pointer can be NULL in a valid LivePathEffectObject contained in a valid list of lpeobjects in an lpeitem!
     * So one should always check whether the returned value is NULL or not */
    Inkscape::LivePathEffect::Effect * get_lpe() {
        return lpe;
    }
    Inkscape::LivePathEffect::Effect const * get_lpe() const {
        return lpe;
    };

    Inkscape::LivePathEffect::Effect *lpe; // this can be NULL in a valid LivePathEffectObject

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttr key, char const* value) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

#endif

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
