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
    {Type::CANVAS_SCROLL_Y, new Modifier("canvas-scroll-y", "Vertical scroll", "Scroll up and down", 0, SCROLL)},
    {Type::CANVAS_SCROLL_X, new Modifier("canvas-scroll-x", "Horizontal scroll", "Scroll left and right", SHIFT, SCROLL)},
    {Type::CANVAS_ZOOM, new Modifier("canvas-zoom", "Canvas zoom", "Zoom in and out with scroll wheel", CTRL, SCROLL)},
    {Type::CANVAS_ROTATE, new Modifier("canvas-rotate", "Canavas rotate", "Rotate the canvas with scroll wheel", SHIFT & CTRL, SCROLL)},
    
    // Select tool modifiers (minus transforms)
    {Type::SELECT_ADD_TO, new Modifier("select-add-to", "Add to selection", "Add items to existing selection", SHIFT, CLICK)},
    {Type::SELECT_IN_GROUPS, new Modifier("select-in-groups", "Select inside groups", "Ignore groups when selecting items", CTRL, CLICK)},
    {Type::SELECT_TOUCH_PATH, new Modifier("select-touch-path", "Select with touch-path", "Draw a band around items to select them", ALT, DRAG)},
    {Type::SELECT_ALWAYS_BOX, new Modifier("select-always-box", "Select with box", "Don't drag items, select more with a box", SHIFT, DRAG)},
    {Type::SELECT_FIRST_HIT, new Modifier("select-first-hit", "Select the first", "Drag the first item the mouse hits", CTRL, DRAG)},
    {Type::SELECT_FORCE_DRAG, new Modifier("select-force-drag", "Forced Drag", "Drag objects even if the mouse isn't over them.", ALT, DRAG)},
    {Type::SELECT_CYCLE, new Modifier("select-cycle", "Cycle through objects", "Scroll through objects under the cursor.", ALT, SCROLL)},

    // Transform handle modifiers (applies to multiple tools)
    {Type::MOVE_CONFINE, new Modifier("move-confine", "Move one axis only", "When dragging items, confine to either x or y axis.", CTRL, DRAG)},   // MOVE by DRAG
    {Type::MOVE_FIXED_RATIO, new Modifier("move-fixed-ratio", "Move fixed amounts", "Move the objects by fixed amounts when dragging.", ALT, DRAG)}, // MOVE by DRAG
    {Type::SCALE_CONFINE, new Modifier("scale-confine", "Keep aspect ratio", "When resizing objects, confine the aspect ratio.", CTRL, HANDLE)}, // SCALE/STRETCH
    {Type::SCALE_FIXED_RATIO, new Modifier("scale-fixed-ratio", "Scale fixed amounts", "When moving or resizing objects, use fixed amounts.", ALT, HANDLE)}, // SCALE/STRETCH
    {Type::TRANS_FIXED_RATIO, new Modifier("trans-fixed-ratio", "Transform in increments", "Rotate or skew by fixed amounts", CTRL, HANDLE)}, // ROTATE/SKEW
    {Type::TRANS_OFF_CENTER, new Modifier("trans-off-center", "Transform against center", "Change the center point when transforming objects.", SHIFT, HANDLE)}, // SCALE/ROTATE/SKEW
    {Type::TRANS_SNAPPING, new Modifier("trans-snapping", "Disable Snapping", "Disable snapping when transforming objects.", SHIFT, DRAG)},   // MOVE/SCALE/STRETCH/ROTATE/SKEW
    // Center handle click: seltrans.cpp:734 SHIFT
    // Align handle click: seltrans.cpp:1365 SHIFT
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

/**
 * Generate a label for any modifier keys based on the mask
 *
 * @param  mask - The Modifier Mask such as {SHIFT & CTRL}
 * @return a string of the keys needed for this mask to be true.
 */
std::string generate_label(KeyMask mask)
{
    auto ret = std::string();
    if(mask & CTRL) ret.append("Ctrl");
    if(mask & SHIFT) {
        if(!ret.empty()) ret.append("+");
        ret.append("Shift");
    }
    if(mask & ALT) {
        if(!ret.empty()) ret.append("+");
        ret.append("Alt");
    }
    if(mask & SUPER) {
        if(!ret.empty()) ret.append("+");
        ret.append("Super");
    }
    if(mask & HYPER) {
        if(!ret.empty()) ret.append("+");
        ret.append("Hyper");
    }
    if(mask & META) {
        if(!ret.empty()) ret.append("+");
        ret.append("Meta");
    }
    return ret;
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
