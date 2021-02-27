// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_PERSP3D_H
#define SEEN_PERSP3D_H

/*
 * Implementation of 3D perspectives as SPObjects
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007  Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#define SP_PERSP3D(obj) (dynamic_cast<Persp3D*>((SPObject*)obj))
#define SP_IS_PERSP3D(obj) (dynamic_cast<const Persp3D*>((SPObject*)obj) != NULL)

#include <list>
#include <map>
#include <vector>

#include "transf_mat_3x4.h"
#include "document.h"
#include "inkscape.h" // for SP_ACTIVE_DOCUMENT

#include "sp-object.h"

class SPBox3D;

namespace Inkscape {
namespace UI {
namespace Tools {

class Box3dTool;

}
}
}


class Persp3DImpl {
public:
    Persp3DImpl();

//private:
    Proj::TransfMat3x4 tmat;

    // Also write the list of boxes into the xml repr and vice versa link boxes to their persp3d?
    std::vector<SPBox3D *> boxes;
    SPDocument *document;

    // for debugging only
    int my_counter;

//    friend class Persp3D;
};

class Persp3D : public SPObject {
public:
	Persp3D();
	~Persp3D() override;

    Persp3DImpl *perspective_impl;

protected:
	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;

	void set(SPAttr key, char const* value) override;

	void update(SPCtx* ctx, unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;


public:
    // FIXME: Make more of these inline!
    static Persp3D * get_from_repr (Inkscape::XML::Node *repr) {
        return SP_PERSP3D(SP_ACTIVE_DOCUMENT->getObjectByRepr(repr));
    }
    Proj::Pt2 get_VP (Proj::Axis axis) const {
        return perspective_impl->tmat.column(axis);
    }
    Geom::Point get_PL_dir_from_pt (Geom::Point const &pt, Proj::Axis axis) const; // convenience wrapper around the following two
    Geom::Point get_finite_dir (Geom::Point const &pt, Proj::Axis axis) const;
    Geom::Point get_infinite_dir (Proj::Axis axis) const;
    double get_infinite_angle (Proj::Axis axis) const;
    static bool VP_is_finite (Persp3DImpl *persp_impl, Proj::Axis axis);
    void toggle_VP (Proj::Axis axis, bool set_undo = true);
    static void toggle_VPs (std::list<Persp3D *>, Proj::Axis axis);
    void set_VP_state (Proj::Axis axis, Proj::VPState state);
    void rotate_VP (Proj::Axis axis, double angle, bool alt_pressed); // angle is in degrees
    void apply_affine_transformation (Geom::Affine const &xform);

    void add_box (SPBox3D *box);
    void remove_box (SPBox3D *box);
    bool has_box (SPBox3D *box) const;

    void update_box_displays ();
    void update_box_reprs ();
    void update_z_orders ();
    unsigned int num_boxes () const { return perspective_impl->boxes.size(); }
    std::list<SPBox3D *> list_of_boxes() const;

    bool perspectives_coincide(Persp3D const *rhs) const;
    void absorb(Persp3D *persp2);

    static Persp3D * create_xml_element (SPDocument *document);
    static Persp3D * document_first_persp (SPDocument *document);

    bool has_all_boxes_in_selection (Inkscape::ObjectSet *set) const;

    void print_debugging_info () const;
    static void print_debugging_info_all(SPDocument *doc);
    static void print_all_selected();
};

#endif /* __PERSP3D_H__ */

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
