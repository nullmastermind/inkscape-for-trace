// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MODIFIERS_H
#define SEEN_SP_MODIFIERS_H
/*
 *  Copyright (C) 2020 Martin Owens <doctormo@gmail.com>

 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include <vector>
#include <map>

#include <gdk/gdk.h>

namespace Inkscape {
namespace Modifiers {

using KeyMask = int;
using Trigger = int;

enum Key : KeyMask {
    NON_USER = -1,
    SHIFT = GDK_SHIFT_MASK,
    CTRL = GDK_CONTROL_MASK,
    ALT = GDK_MOD1_MASK,
    SUPER = GDK_SUPER_MASK,
};
// Triggers used for collision warnings, two tools are using the same trigger
enum Triggers : Trigger {CLICK, DRAG, SCROLL, HANDLE};
// TODO: We may want to further define the tool, from ANY, SELECT, NODE etc.

/**
 * This anonymous enum is used to provide a list of the Shifts
 */
enum class Type {
    // {TOOL_NAME}_{ACTION_NAME}

    // Canvas tools (applies to any tool selection)
    CANVAS_SCROLL_Y,      // Scroll up and down {NOTHING+SCROLL}
    CANVAS_SCROLL_X,      // Scroll left and right {SHIFT+SCROLL}
    CANVAS_ZOOM,          // Zoom in and out {CTRL+SCROLL}
    CANVAS_ROTATE,        // Rotate CW and CCW {CTRL+SHIFT+SCROLL}

    // Select tool (minus transform)
    SELECT_ADD_TO,        // Add selection {SHIFT+CLICK}
    SELECT_IN_GROUPS,     // Select within groups {CTRL+CLICK}
    SELECT_TOUCH_PATH,    // Draw band to select {ALT+DRAG+Nothing selected}
    SELECT_ALWAYS_BOX,    // Draw box to select {SHIFT+DRAG}
    SELECT_FIRST_HIT,     // Start dragging first item hit {CTRL+DRAG} (Is this an actual feature?)
    SELECT_FORCE_DRAG,    // Drag objects even if the mouse isn't over them {ALT+DRAG+Selected}
    SELECT_CYCLE,         // Cycle through objects under cursor {ALT+SCROLL}

    // Transform handles (applies to multiple tools)
    MOVE_AXIS_CONFINE,    // Limit dragging to X OR Y only {DRAG+CTRL}
    SCALE_RATIO_CONFINE,  // Confine resize aspect ratio {HANDLE+CTRL}
    SCALE_FROM_CENTER,    // Resize from center {HANDLE+SHIFT}
    SCALE_FIXED_RATIO,    // Resize by fixed ratio sizes {HANDLE+ALT}
    TRANS_FIXED_RATIO,    // Rotate/skew by fixed ratio angles {HANDLE+CTRL}
    TRANS_OFF_CENTER,     // Rotate/skew from oposite corner {HANDLE+SHIFT}
    // TODO: Alignment ommitted because it's UX is not completed
};


/**
 * A class to represent ways functionality is driven by shift modifiers
 */
class Modifier {
private:
    /** An easy to use definition of the table of modifiers by Type and ID. */
    typedef std::map<Type, Modifier *> Container;
    typedef std::map<std::string, Modifier *> Lookup;

    /** A table of all the created modifers and their ID lookups. */
    static Container _modifiers;
    static Lookup _modifier_lookup;

    char const * _id;    // A unique id used by keys.xml to identify it
    char const * _name;  // A descriptive name used in preferences UI
    char const * _desc;  // A more verbose description used in preferences UI

    Trigger _trigger; // The type of trigger used for collisions

    // Default values if nothing is set in keys.xml
    KeyMask _and_mask_default; // The pressed keys must have these bits set

    // User set data, set by keys.xml (or other included file)
    KeyMask _and_mask_user = NON_USER;

protected:

public:

    char const * get_id() const { return _id; }
    char const * get_name() const { return _name; }
    char const * get_description() const { return _desc; }
    const Trigger get_trigger() const { return _trigger; }

    // Set user value
    bool is_set() const { return _and_mask_user != NON_USER; }
    void set(KeyMask and_mask) {
        _and_mask_user = and_mask;
    }
    void unset() { set(NON_USER); }

    // Get value, either user defined value or default
    const KeyMask get_and_mask() {
        if(_and_mask_user != NON_USER) {
            return _and_mask_user;
        }
        return _and_mask_default;
    }

    /**
     * Inititalizes the Modifier with the parameters.
     *
     * @param id       Goes to \c _id.
     * @param name     Goes to \c _name.
     * @param desc     Goes to \c _desc.
     * @param default_ Goes to \c _default.
     */
    Modifier(char const * id,
             char const * name,
             char const * desc,
             const KeyMask and_mask,
             const Trigger trigger) :
        _id(id),
        _name(name),
        _desc(desc),
        _and_mask_default(and_mask),
        _trigger(trigger)
    {
        _modifier_lookup.emplace(_id, this);
    }
    // Delete the destructor, because we are eternal
    ~Modifier() = delete;

    static std::vector<Modifier *>getList ();
    bool active(int button_state);

    /**
     * A function to turn an enum index into a modifier object.
     *
     * @param  index  The enum index to be translated
     * @return A pointer to a modifier object or a NULL if not found.
     */
    static Modifier * get(Type index) {
        return _modifiers[index];
    }
    /**
     * A function to turn a string id into a modifier object.
     *
     * @param  id  The id string to be translated
     * @return A pointer to a modifier object or a NULL if not found.
     */
    static Modifier * get(char const * id) {
        Modifier *modifier = nullptr;
        Lookup::iterator mod_found = _modifier_lookup.find(id);

        if (mod_found != _modifier_lookup.end()) {
            modifier = mod_found->second;
        }
    
        return modifier;
    }

}; // Modifier class

} // namespace Modifiers
} // namespace Inkscape


#endif // SEEN_SP_MODIFIERS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
