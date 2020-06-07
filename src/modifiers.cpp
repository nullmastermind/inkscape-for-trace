// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Modifiers for inkscape
 *
 * The file provides a definition of all the ways shift/ctrl/alt modifiers
 * are used in Inkscape, and allows users to customise them in keys.xml
 * 
 *//*
 * Authors:
 * 2020 Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include "modifiers.h"

namespace Inkscape {

Modifier::ModifierLookup Modifier::_modifier_lookup;

// these must be in the same order as the SP_MODIFIER_* enum in "modifers.h"
Modifier *Modifier::_modifiers[] = {
    // Header
    new Modifier(SP_MODIFIER_INVALID, nullptr, nullptr, nullptr, 0),

    // Canvas modifiers
    new Modifier(SP_MODIFIER_CANVAS_SCROLL_Y, "canvas-scroll-y", "Vertical scroll", "Scroll up and down", 0),
    new Modifier(SP_MODIFIER_CANVAS_SCROLL_X, "canvas-scroll-x", "Horizontal scroll", "Scroll left and right", SHIFT),
    new Modifier(SP_MODIFIER_CANVAS_ZOOM, "canvas-zoom", "Canvas zoom", "Zoom in and out with scroll wheel", CTRL),
    new Modifier(SP_MODIFIER_CANVAS_ROTATE, "canvas-rotate", "Canavas rotate", "Rotate the canvas with scroll wheel", SHIFT & CTRL),
    
    // Select tool modifiers (minus transforms)
    new Modifier(SP_MODIFIER_SELECT_ADD_TO, "select-add-to", "Add to selection", "Add items to existing selection", SHIFT),
    new Modifier(SP_MODIFIER_SELECT_IN_GROUPS, "select-in-groups", "Select inside groups", "Ignore groups when selecting items", CTRL),
    new Modifier(SP_MODIFIER_SELECT_TOUCH_PATH, "select-touch-path", "Select with touch-path", "Draw a band around items to select them", ALT),
    new Modifier(SP_MODIFIER_SELECT_ALWAYS_BOX, "select-always-box", "Select with box", "Don't drag items, select more with a box", SHIFT),
    new Modifier(SP_MODIFIER_SELECT_FIRST_HIT, "select-first-hit", "Select the first", "Scroll up and down", CTRL),

    // Transform handle modifiers (applies to multiple tools)
    new Modifier(SP_MODIFIER_MOVE_AXIS_CONFINE, "move-confine", "Move confine", "When dragging items, confine to either x or y axis", CTRL),
    new Modifier(SP_MODIFIER_SCALE_RATIO_CONFINE, "scale-confine", "Scale confine", "When resizing objects, confine the aspect ratio", CTRL),
    new Modifier(SP_MODIFIER_SCALE_FROM_CENTER, "scale-from-center", "Scale from center", "When resizing obects, scale from the center", SHIFT),
    new Modifier(SP_MODIFIER_SCALE_FIXED_RATIO, "scale-fixed-ratio", "Scale fixed amounts", "When resizing objects, scale by fixed amounts", ALT),
    new Modifier(SP_MODIFIER_TRANS_FIXED_RATIO, "trans-fixed-ratio", "Transform in increments", "Rotate or skew by fixed amounts", CTRL),
    new Modifier(SP_MODIFIER_TRANS_OFF_CENTER, "trans-off-center", "Transform against center", "When rotating or skewing, use the far point as the anchor", SHIFT),

    // Footer
    new Modifier(SP_MODIFIER_LAST, " '\"invalid id", nullptr, nullptr, 0)
};

std::vector<Inkscape::Modifier *>
Modifier::getList () {

    std::vector<Modifier *> modifiers;
    // Go through the dynamic modifier table
    for (auto & _modifier : _modifiers) {
        Modifier * modifier = _modifier;
        if (modifier->get_index() == SP_MODIFIER_INVALID ||
            modifier->get_index() == SP_MODIFIER_LAST) {
            continue;
        }
        modifiers.push_back(modifier);
    }

    return modifiers;
};

}  // namespace Inkscape

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
