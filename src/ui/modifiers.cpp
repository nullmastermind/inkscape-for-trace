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
#include <glibmm/i18n.h>
#include "modifiers.h"
#include "ui/tools/tool-base.h"

namespace Inkscape {
namespace Modifiers {

Modifier::Lookup Modifier::_modifier_lookup;

// these must be in the same order as the * enum in "modifers.h"
decltype(Modifier::_modifiers) Modifier::_modifiers {
    // Canvas modifiers
    {Type::CANVAS_SCROLL_Y, new Modifier("canvas-scroll-y", _("Vertical scroll"), _("Scroll up and down"), 0, SCROLL)},
    {Type::CANVAS_SCROLL_X, new Modifier("canvas-scroll-x", _("Horizontal scroll"), _("Scroll left and right"), SHIFT, SCROLL)},
    {Type::CANVAS_ZOOM, new Modifier("canvas-zoom", _("Canvas zoom"), _("Zoom in and out with scroll wheel"), CTRL, SCROLL)},
    {Type::CANVAS_ROTATE, new Modifier("canvas-rotate", _("Canavas rotate"), _("Rotate the canvas with scroll wheel"), SHIFT & CTRL, SCROLL)},
    
    // Select tool modifiers (minus transforms)
    {Type::SELECT_ADD_TO, new Modifier("select-add-to", _("Add to selection"), _("Add items to existing selection"), SHIFT, CLICK)},
    {Type::SELECT_IN_GROUPS, new Modifier("select-in-groups", _("Select inside groups"), _("Ignore groups when selecting items"), CTRL, CLICK)},
    {Type::SELECT_TOUCH_PATH, new Modifier("select-touch-path", _("Select with touch-path"), _("Draw a band around items to select them"), ALT, DRAG)},
    {Type::SELECT_ALWAYS_BOX, new Modifier("select-always-box", _("Select with box"), _("Don't drag items, select more with a box"), SHIFT, DRAG)},
    {Type::SELECT_FIRST_HIT, new Modifier("select-first-hit", _("Select the first"), _("Drag the first item the mouse hits"), CTRL, DRAG)},
    {Type::SELECT_FORCE_DRAG, new Modifier("select-force-drag", _("Forced Drag"), _("Drag objects even if the mouse isn't over them."), ALT, DRAG)},
    {Type::SELECT_CYCLE, new Modifier("select-cycle", _("Cycle through objects"), _("Scroll through objects under the cursor."), ALT, SCROLL)},

    // Transform handle modifiers (applies to multiple tools)
    {Type::MOVE_CONFINE, new Modifier("move-confine", _("Move one axis only"), _("When dragging items, confine to either x or y axis."), CTRL, DRAG)},   // MOVE by DRAG
    {Type::MOVE_FIXED_RATIO, new Modifier("move-fixed-ratio", _("Move fixed amounts"), _("Move the objects by fixed amounts when dragging."), ALT, DRAG)}, // MOVE by DRAG
    {Type::SCALE_CONFINE, new Modifier("scale-confine", _("Keep aspect ratio"), _("When resizing objects, confine the aspect ratio."), CTRL, HANDLE)}, // SCALE/STRETCH
    {Type::SCALE_FIXED_RATIO, new Modifier("scale-fixed-ratio", _("Scale fixed amounts"), _("When moving or resizing objects, use fixed amounts."), ALT, HANDLE)}, // SCALE/STRETCH
    {Type::TRANS_FIXED_RATIO, new Modifier("trans-fixed-ratio", _("Transform in increments"), _("Rotate or skew by fixed amounts"), CTRL, HANDLE)}, // ROTATE/SKEW
    {Type::TRANS_OFF_CENTER, new Modifier("trans-off-center", _("Transform against center"), _("Change the center point when transforming objects."), SHIFT, HANDLE)}, // SCALE/ROTATE/SKEW
    {Type::TRANS_SNAPPING, new Modifier("trans-snapping", _("Disable Snapping"), _("Disable snapping when transforming objects."), SHIFT, DRAG)},   // MOVE/SCALE/STRETCH/ROTATE/SKEW
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
    return (Key::ALL_MODS & button_state) == get_and_mask();
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

/**
 * Set the responsive tooltip for this tool, given the selected types.
 *
 * @param message_context - The desktop's message context for showing tooltips
 * @param event - The current event status (which keys are pressed)
 * @param num_args - Number of Modifier::Type arguments to follow.
 * @param ... - One or more Modifier::Type arguments.
 */
void responsive_tooltip(Inkscape::MessageContext *message_context, GdkEvent *event, int num_args, ...)
{
    std::string ctrl_msg = "<b>Ctrl</b>: ";
    std::string shift_msg = "<b>Shift</b>: ";
    std::string alt_msg = "<b>Alt</b>: ";

    // NOTE: This will hide any keys changed to SUPER or multiple keys such as CTRL+SHIFT
    va_list args;
    va_start(args, num_args);
    for(int i = 0; i < num_args; i++) {
        auto modifier = Modifier::get(va_arg(args, Type));
        auto name = std::string(modifier->get_name());
        switch (modifier->get_and_mask()) {
            case CTRL:
                ctrl_msg += name + ", ";
                break;
            case SHIFT:
                shift_msg += name + ", ";
                break;
            case ALT:
                alt_msg += name + ", ";
                break;
            default:
                g_warning("Unhandled responsivle tooltip: %s", name.c_str());
        }
    }
    va_end(args);
    ctrl_msg.erase(ctrl_msg.size() - 2);
    shift_msg.erase(shift_msg.size() - 2);
    alt_msg.erase(alt_msg.size() - 2);

    Inkscape::UI::Tools::sp_event_show_modifier_tip(message_context, event,
        ctrl_msg.c_str(), shift_msg.c_str(), alt_msg.c_str());
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
