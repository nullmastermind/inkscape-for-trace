// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::ShapeEditor
 * This is a container class which contains a knotholder for shapes.
 * It is attached to a single item.
 *//*
 * Authors: see git history
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SHAPE_EDITOR_H
#define SEEN_SHAPE_EDITOR_H

#include <2geom/affine.h>

class KnotHolder;
class LivePathEffectObject;
class SPDesktop;
class SPItem;

namespace Inkscape { namespace XML { class Node; }
namespace UI {

class ShapeEditor {
public:

    ShapeEditor(SPDesktop *desktop, Geom::Affine edit_transform = Geom::identity());
    ~ShapeEditor();

    void set_item(SPItem *item);
    void unset_item(bool keep_knotholder = false);

    void update_knotholder(); //((deprecated))

    bool has_local_change();
    void decrement_local_change();

    bool knot_mouseover() const;
    KnotHolder *knotholder;
    KnotHolder *lpeknotholder;
    bool has_knotholder();
    static void blockSetItem(bool b) { _blockSetItem = b; } // kludge
    static void event_attr_changed(Inkscape::XML::Node * /*repr*/, char const *name, char const * /*old_value*/,
                                   char const * /*new_value*/, bool /*is_interactive*/, void *data);
private:
    void reset_item();
    static bool _blockSetItem;

    SPDesktop *desktop;
    Inkscape::XML::Node *knotholder_listener_attached_for;
    Inkscape::XML::Node *lpeknotholder_listener_attached_for;
    Geom::Affine _edit_transform;
};

} // namespace UI
} // namespace Inkscape

#endif // SEEN_SHAPE_EDITOR_H


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

