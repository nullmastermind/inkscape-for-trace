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
namespace Modifiers {

Modifier::Lookup Modifier::_modifier_lookup;

// these must be in the same order as the * enum in "modifers.h"
decltype(Modifier::_modifiers) Modifier::_modifiers {
    // Canvas modifiers
    {Type::CANVAS_SCROLL_Y, new Modifier("canvas-scroll-y", "Vertical scroll", "Scroll up and down", 0)},
    {Type::CANVAS_SCROLL_X, new Modifier("canvas-scroll-x", "Horizontal scroll", "Scroll left and right", SHIFT)},
    {Type::CANVAS_ZOOM, new Modifier("canvas-zoom", "Canvas zoom", "Zoom in and out with scroll wheel", CTRL)},
    {Type::CANVAS_ROTATE, new Modifier("canvs-rotate", "Canavas rotate", "Rotate the canvas with scroll wheel", SHIFT & CTRL)},
    
    // Select tool modifiers (minus transforms)
    {Type::SELECT_ADD_TO, new Modifier("select-add-to", "Add to selection", "Add items to existing selection", SHIFT)},
    {Type::SELECT_IN_GROUPS, new Modifier("select-in-groups", "Select inside groups", "Ignore groups when selecting items", CTRL)},
    {Type::SELECT_TOUCH_PATH, new Modifier("select-touch-path", "Select with touch-path", "Draw a band around items to select them", ALT)},
    {Type::SELECT_ALWAYS_BOX, new Modifier("select-always-box", "Select with box", "Don't drag items, select more with a box", SHIFT)},
    {Type::SELECT_FIRST_HIT, new Modifier("select-first-hit", "Select the first", "Drag the first item the mouse hits", CTRL)},
    {Type::SELECT_FORCE_DRAG, new Modifier("select-force-drag", "Forced Drag", "Drag objects even if the mouse isn't over them.", ALT)},

    // Transform handle modifiers (applies to multiple tools)
    {Type::MOVE_AXIS_CONFINE, new Modifier("move-confine", "Move confine", "When dragging items, confine to either x or y axis", CTRL)},
    {Type::SCALE_RATIO_CONFINE, new Modifier("scale-confine", "Scale confine", "When resizing objects, confine the aspect ratio", CTRL)},
    {Type::SCALE_FROM_CENTER, new Modifier("scale-from-center", "Scale from center", "When resizing obects, scale from the center", SHIFT)},
    {Type::SCALE_FIXED_RATIO, new Modifier("scale-fixed-ratio", "Scale fixed amounts", "When resizing objects, scale by fixed amounts", ALT)},
    {Type::TRANS_FIXED_RATIO, new Modifier("trans-fixed-ratio", "Transform in increments", "Rotate or skew by fixed amounts", CTRL)},
    {Type::TRANS_OFF_CENTER, new Modifier("trans-off-center", "Transform against center", "When rotating or skewing, use the far point as the anchor", SHIFT)},
};

/**
  * List all the modifiers available. Used in UI listing.
  *
  * @return a vector of Modifier objects.
  */
std::vector<Modifier *>
Modifier::getList () {

    std::vector<Modifier *> modifiers;
    // Go through the dynamic modifier table
    for( auto const& [key, val] : _modifiers ) {
        modifiers.push_back(val);
    }

    return modifiers;
};

/**
 * Test if this modifier is currently active.
 *
 * @param  button_state - The GDK button state from an event
 * @return a boolean, true if the modifiers for this action are active.
 */
bool Modifier::active(int button_state)
{
    // TODO:
    //  * ALT key is sometimes MOD1, MOD2 etc, if we find other ALT keys, set the ALT bit
    //  * SUPER key could be HYPER or META, these cases need to be considered.
    return get_and_mask() & button_state;
}

} // namespace Modifiers
} // namespace Inkscape

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
