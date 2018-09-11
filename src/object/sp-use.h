// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_USE_H
#define SEEN_SP_USE_H

/*
 * SVG <use> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2014 Authors
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstddef>
#include <sigc++/sigc++.h>

#include "svg/svg-length.h"
#include "sp-dimensions.h"
#include "sp-item.h"
#include "enums.h"

class SPUseReference;

class SPUse : public SPItem, public SPDimensions {
public:
	SPUse();
	~SPUse() override;

    // item built from the original's repr (the visible clone)
    // relative to the SPUse itself, it is treated as a child, similar to a grouped item relative to its group
    SPItem *child;

    // SVG attrs
    char *href;

    // the reference to the original object
    SPUseReference *ref;

    // a sigc connection for delete notifications
    sigc::connection _delete_connection;
    sigc::connection _changed_connection;

    // a sigc connection for transformed signal, used to do move compensation
    sigc::connection _transformed_connection;

	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;
	void set(SPAttributeEnum key, char const *value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType bboxtype) const override;
    const char* displayName() const override;
	char* description() const override;
	void print(SPPrintContext *ctx) override;
	Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
	void hide(unsigned int key) override;
	void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const override;

	SPItem *root();
	SPItem const *root() const;
    int cloneDepth() const;

	SPItem *unlink();
	SPItem *get_original();
	Geom::Affine get_parent_transform();
	Geom::Affine get_root_transform();

private:
    void href_changed();
    void move_compensate(Geom::Affine const *mp);
    void delete_self();
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
