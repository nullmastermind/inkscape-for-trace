// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_BOX3D_H
#define SEEN_SP_BOX3D_H

/*
 * SVG <box3d> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Maximilian Albert <Anhalter42@gmx.de>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org.
 *
 * Copyright (C) 2007      Authors
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-item-group.h"
#include "proj_pt.h"
#include "axis-manip.h"

#define SP_TYPE_BOX3D            (box3d_get_type ())

class Persp3D;
class Persp3DReference;

class SPBox3D : public SPGroup {
public:
	SPBox3D();
	~SPBox3D() override;

    int z_orders[6]; // z_orders[i] holds the ID of the face at position #i in the group (from top to bottom)

    char *persp_href;
    Persp3DReference *persp_ref;

    Proj::Pt3 orig_corner0;
    Proj::Pt3 orig_corner7;

    Proj::Pt3 save_corner0;
    Proj::Pt3 save_corner7;

    Box3D::Axis swapped; // to indicate which coordinates are swapped during dragging

    int my_counter; // for debugging only

    /**
     * Create a SPBox3D and append it to the parent.
     */
    static SPBox3D * createBox3D(SPItem * parent);

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void set(SPAttributeEnum key, char const* value) override;
	void update(SPCtx *ctx, unsigned int flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

        virtual const char* display_name();
	Geom::Affine set_transform(Geom::Affine const &transform) override;
    void convert_to_guides() const override;
    const char* displayName() const override;
    char *description() const override;

    void position_set ();
    Geom::Point get_corner_screen (unsigned int id, bool item_coords = true) const;
    Proj::Pt3 get_proj_center ();
    Geom::Point get_center_screen ();

    void set_corner (unsigned int id, Geom::Point const &new_pos, Box3D::Axis movement, bool constrained);
    void set_center (Geom::Point const &new_pos, Geom::Point const &old_pos, Box3D::Axis movement, bool constrained);
    void corners_for_PLs (Proj::Axis axis, Geom::Point &corner1, Geom::Point &corner2, Geom::Point &corner3, Geom::Point &corner4) const;
    bool recompute_z_orders ();
    void set_z_orders ();

    int pt_lies_in_PL_sector (Geom::Point const &pt, int id1, int id2, Box3D::Axis axis) const;
    int VP_lies_in_PL_sector (Proj::Axis vpdir, int id1, int id2, Box3D::Axis axis) const;

    void relabel_corners();
    void check_for_swapped_coords();

    static std::list<SPBox3D *> extract_boxes(SPObject *obj);

    Persp3D *get_perspective() const;
    void switch_perspectives(Persp3D *old_persp, Persp3D *new_persp, bool recompute_corners = false);

    SPGroup *convert_to_group();
};

static inline void box3d_position_set (SPBox3D *box) { box->position_set(); }
static inline Geom::Point box3d_get_corner_screen (SPBox3D const *box, unsigned int id, bool item_coords = true) { return box->get_corner_screen(id, item_coords); }
static inline Proj::Pt3 box3d_get_proj_center (SPBox3D *box) { return box->get_proj_center(); }
static inline Geom::Point box3d_get_center_screen (SPBox3D *box) { return box->get_center_screen(); }

static inline void box3d_set_corner (SPBox3D *box, unsigned int id, Geom::Point const &new_pos, Box3D::Axis movement, bool constrained) { box->set_corner(id, new_pos, movement, constrained); }
static inline void box3d_set_center (SPBox3D *box, Geom::Point const &new_pos, Geom::Point const &old_pos, Box3D::Axis movement, bool constrained) { box->set_center(new_pos, old_pos, movement, constrained); }
static inline void box3d_corners_for_PLs (const SPBox3D * box, Proj::Axis axis, Geom::Point &corner1, Geom::Point &corner2, Geom::Point &corner3, Geom::Point &corner4) { box->corners_for_PLs(axis, corner1, corner2, corner3, corner4); }
static inline bool box3d_recompute_z_orders (SPBox3D *box) { return box->recompute_z_orders(); }
static inline void box3d_set_z_orders (SPBox3D *box) { box->set_z_orders(); }

static inline int box3d_pt_lies_in_PL_sector (SPBox3D const *box, Geom::Point const &pt, int id1, int id2, Box3D::Axis axis) { return box->pt_lies_in_PL_sector(pt, id1, id2, axis); }
static inline int box3d_VP_lies_in_PL_sector (SPBox3D const *box, Proj::Axis vpdir, int id1, int id2, Box3D::Axis axis) { return box->VP_lies_in_PL_sector(vpdir, id1, id2, axis); }

static inline void box3d_relabel_corners(SPBox3D *box) { box->relabel_corners(); }
static inline void box3d_check_for_swapped_coords(SPBox3D *box) { box->check_for_swapped_coords(); }

static inline std::list<SPBox3D *> box3d_extract_boxes(SPObject *obj) { return SPBox3D::extract_boxes(obj); }

static inline Persp3D *box3d_get_perspective(SPBox3D const *box) { return box->get_perspective(); }
static inline void box3d_switch_perspectives(SPBox3D *box, Persp3D *old_persp, Persp3D *new_persp, bool recompute_corners = false) { box->switch_perspectives(old_persp, new_persp, recompute_corners); }

static inline SPGroup *box3d_convert_to_group(SPBox3D *box) { return box->convert_to_group(); }


#endif // SEEN_SP_BOX3D_H

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
