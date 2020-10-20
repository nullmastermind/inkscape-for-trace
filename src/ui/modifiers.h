// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MODIFIERS_H
#define SEEN_SP_MODIFIERS_H
/*
 *  Copyright (C) 2020 Martin Owens <doctormo@gmail.com>

 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <map>

#include <gdk/gdk.h>

#include "message-context.h"

namespace Inkscape {
namespace Modifiers {

using KeyMask = int;
using Trigger = int;

enum Key : KeyMask {
    NEVER = -2, // Never happen, switch off
    NOT_SET = -1, // Not set (user or keys)
    ALWAYS = 0, // Always happens, no modifier needed
    SHIFT = GDK_SHIFT_MASK,
    CTRL = GDK_CONTROL_MASK,
    ALT = GDK_MOD1_MASK,
    SUPER = GDK_SUPER_MASK,
    HYPER = GDK_HYPER_MASK,
    META = GDK_META_MASK,
    ALL_MODS = SHIFT | CTRL | ALT | SUPER | HYPER | META,
};

// Triggers used for collision warnings, two tools are using the same trigger
enum Triggers : Trigger {
    NO_CATEGORY, CANVAS, SELECT, MOVE, TRANSFORM,
    // Action taken to trigger this modifier, starts at
    // bit 6 so categories and triggers can be combined.
    CLICK = 32,
    DRAG = 64,
    SCROLL = 128,
};

/**
 * This anonymous enum is used to provide a list of the Shifts
 */
enum class Type {
    // {TOOL_NAME}_{ACTION_NAME}

    // Canvas tools (applies to any tool selection)
    CANVAS_PAN_Y,         // Pan up and down {NOTHING+SCROLL}
    CANVAS_PAN_X,         // Pan left and right {SHIFT+SCROLL}
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
    MOVE_CONFINE,         // Limit dragging to X OR Y only {DRAG+CTRL}
    MOVE_INCREMENT,       // Move objects by fixed amounts {DRAG+ALT}
    MOVE_SNAPPING,        // Disable snapping while moving {DRAG+SHIFT}
    TRANS_CONFINE,        // Confine resize aspect ratio {HANDLE+CTRL}
    TRANS_INCREMENT,      // Scale/Rotate/skew by fixed ratio angles {HANDLE+ALT}
    TRANS_OFF_CENTER,     // Scale/Rotate/skew from oposite corner {HANDLE+SHIFT}
    TRANS_SNAPPING,       // Disable snapping while transforming {HANDLE+SHIFT}
    // TODO: Alignment ommitted because it's UX is not completed
};


// Generate a label such as Shift+Ctrl from any KeyMask
std::string   generate_label(KeyMask mask);
unsigned long calculate_weight(KeyMask mask);

// Generate a responsivle tooltip set
void responsive_tooltip(Inkscape::MessageContext *message_context, GdkEvent *event, int num_args, ...);

/**
 * A class to represent ways functionality is driven by shift modifiers
 */
class Modifier {
private:
    /** An easy to use definition of the table of modifiers by Type and ID. */
    typedef std::map<Type, Modifier *> Container;
    typedef std::map<std::string, Modifier *> Lookup;
    typedef std::map<Trigger, std::string> CategoryNames;

    /** A table of all the created modifers and their ID lookups. */
    static Container _modifiers;
    static Lookup _modifier_lookup;
    static CategoryNames _category_names;

    char const * _id;    // A unique id used by keys.xml to identify it
    char const * _name;  // A descriptive name used in preferences UI
    char const * _desc;  // A more verbose description used in preferences UI

    Trigger _category; // The category of tool, what it might conflict with
    Trigger _trigger; // The type of trigger/action

    // Default values if nothing is set in keys.xml
    KeyMask _and_mask_default; // The pressed keys must have these bits set
    unsigned long _weight_default = 0;

    // User set data, set by keys.xml (or other included file)
    KeyMask _and_mask_keys = NOT_SET;
    KeyMask _not_mask_keys = NOT_SET;
    unsigned long _weight_keys = 0;
    KeyMask _and_mask_user = NOT_SET;
    KeyMask _not_mask_user = NOT_SET;
    unsigned long _weight_user = 0;

protected:

public:

    char const * get_id() const { return _id; }
    char const * get_name() const { return _name; }
    char const * get_description() const { return _desc; }
    const Trigger get_trigger() const { return _category | _trigger; }

    // Set user value
    void set_keys(KeyMask and_mask, KeyMask not_mask) {
        _and_mask_keys = and_mask;
        _not_mask_keys = not_mask;
        _weight_keys = calculate_weight(and_mask) + calculate_weight(not_mask);
    }
    void set_user(KeyMask and_mask, KeyMask not_mask) {
        _and_mask_user = and_mask;
        _not_mask_user = not_mask;
        _weight_user = calculate_weight(and_mask) + calculate_weight(not_mask);
    }
    void unset_keys() { set_keys(NOT_SET, NOT_SET); }
    void unset_user() { set_user(NOT_SET, NOT_SET); }
    bool is_set_user() const { return _and_mask_user != NOT_SET; }

    // Get value, either user defined value or default
    const KeyMask get_and_mask() {
        if(_and_mask_user != NOT_SET) return _and_mask_user;
        if(_and_mask_keys != NOT_SET) return _and_mask_keys;
        return _and_mask_default;
    }
    const KeyMask get_not_mask() {
        if(_not_mask_user != NOT_SET) return _not_mask_user;
        if(_not_mask_keys != NOT_SET) return _not_mask_keys;
        return NOT_SET;
    }
    // Return number of bits set for the keys
    unsigned long get_weight() {
        if(_and_mask_user != NOT_SET) return _weight_user;
        if(_and_mask_keys != NOT_SET) return _weight_keys;
        return _weight_default;
    }

    // Generate labels such as "Shift+Ctrl" for the active modifier
    std::string get_label() { return generate_label(get_and_mask()); }
    std::string get_category() { return _category_names[_category]; }

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
             const Trigger category,
             const Trigger trigger) :
        _id(id),
        _name(name),
        _desc(desc),
        _and_mask_default(and_mask),
        _category(category),
        _trigger(trigger)
    {
        _modifier_lookup.emplace(_id, this);
        _weight_default = calculate_weight(and_mask);
    }
    // Delete the destructor, because we are eternal
    ~Modifier() = delete;

    static Type which(Trigger trigger, int button_state);
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
