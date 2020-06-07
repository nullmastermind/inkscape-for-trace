// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MODIFIERS_H
#define SEEN_SP_MODIFIERS_H
/*
 *  Copyright (C) 2020 Martin Owens <doctormo@gmail.com>

 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include <map>
#include <vector>
//#include <gdk/gdkkeysyms.h>

int SHIFT = 1; // GDK_SHIFT_MASK;
int CTRL = 4; // GDK_CONTROL_MASK;
int ALT = 8; // GDK_MOD1_MASK;

/**
 * This anonymous enum is used to provide a list of the Shifts
 */
enum {
    SP_MODIFIER_INVALID,
    // SP_MODIFIER_{TOOL_NAME}_{ACTION_NAME}

    // Canvas tools (applies to any tool selection)
    SP_MODIFIER_CANVAS_SCROLL_Y,      // Scroll up and down {NOTHING+SCROLL}
    SP_MODIFIER_CANVAS_SCROLL_X,      // Scroll left and right {SHIFT+SCROLL}
    SP_MODIFIER_CANVAS_ZOOM,          // Zoom in and out {CTRL+SCROLL}
    SP_MODIFIER_CANVAS_ROTATE,        // Rotate CW and CCW {CTRL+SHIFT+SCROLL}

    // Select tool (minus transform)
    SP_MODIFIER_SELECT_ADD_TO,        // Add selection {SHIFT+CLICK}
    SP_MODIFIER_SELECT_IN_GROUPS,     // Select within groups {CTRL+CLICK}
    SP_MODIFIER_SELECT_TOUCH_PATH,    // Draw band to select {ALT+DRAG}
    SP_MODIFIER_SELECT_ALWAYS_BOX,    // Draw box to select {SHIFT+DRAG}
    SP_MODIFIER_SELECT_FIRST_HIT,     // Start dragging first item hit {CTRL+DRAG} (Is this an actual feature?)

    // Transform handles (applies to multiple tools)
    SP_MODIFIER_MOVE_AXIS_CONFINE,    // Limit dragging to X OR Y only {DRAG+CTRL}
    SP_MODIFIER_SCALE_RATIO_CONFINE,  // Confine resize aspect ratio {HANDLE+CTRL}
    SP_MODIFIER_SCALE_FROM_CENTER,    // Resize from center {HANDLE+SHIFT}
    SP_MODIFIER_SCALE_FIXED_RATIO,    // Resize by fixed ratio sizes {HANDLE+ALT}
    SP_MODIFIER_TRANS_FIXED_RATIO,    // Rotate/skew by fixed ratio angles {HANDLE+CTRL}
    SP_MODIFIER_TRANS_OFF_CENTER,     // Rotate/skew from oposite corner {HANDLE+SHIFT}
    // TODO: Alignment ommitted because it's UX is not completed

    /* Footer */
    SP_MODIFIER_LAST
};

namespace Inkscape {

/**
 * A class to represent ways functionality is driven by shift modifiers
 */
class Modifier {
private:
    /** An easy to use definition of the table of modifiers by ID. */
    typedef std::map<char const *, Modifier *> ModifierLookup;

    /** A table of all the created modifers. */
    static ModifierLookup _modifier_lookup;

    static Modifier * _modifiers[SP_MODIFIER_LAST + 1];
    /* Plus one because there is an entry for SP_MODIFIER_LAST */

    // Fixed data defined in code and created at run time

    unsigned int _index; // Index of the modifier, based on the modifier enum
    char const * _id;    // A unique id used by keys.xml to identify it
    char const * _name;  // A descriptive name used in preferences UI
    char const * _desc;  // A more verbose description used in preferences UI
    //char const * _group; // Optional group for preferences UI
    unsigned int _default; // The default value if nothing set in keys.xml

    // User set data
    signed int _value; // The value set by keys.xml, set to -1 if unset.

protected:

public:

    unsigned int get_index () { return _index; }
    char const * get_id () { return _id; }
    char const * get_name () { return _name; }
    char const * get_description () { return _desc; }
    //char const * get_group () { return _group; }

    // Set user value
    bool is_user_set() { return _value != -1; }
    void set_value (unsigned int value) { _value = value; }
    void unset_value() { _value = -1; }

    // Get value, either user defined value or default
    unsigned int get_value() {
        if(is_user_set()) {
            return _value;
        }
        return _default;
    }

    /**
     * Inititalizes the Modifier with the parameters.
     *
     * @param index    Goes to \c _index.
     * @param id       Goes to \c _id.
     * @param name     Goes to \c _name.
     * @param desc     Goes to \c _desc.
     * @param default_ Goes to \c _default.
     */
    Modifier(const unsigned int index,
             char const * id,
             char const * name,
             char const * desc,
             const unsigned int default_) :
        _index(index),
        _id(id),
        _name(name),
        _desc(desc),
        _default(default_),
        _value(-1)
    {
        _modifier_lookup.insert(ModifierLookup::value_type(_id, this));
    }
    static void list ();
    static std::vector<Inkscape::Modifier *>getList ();

    /**
     * A function to turn an enum index into a modifier object.
     *
     * @param  index  The enum index to be translated
     * @return A pointer to a modifier object or a NULL if not found.
     */
    static Modifier * get(unsigned int index) {
        if (index <= SP_MODIFIER_LAST) {
            return _modifiers[index];
        }
    }
    /**
     * A function to turn a string id into a modifier object.
     *
     * @param  id  The id string to be translated
     * @return A pointer to a modifier object or a NULL if not found.
     */
    static Modifier * get(char const * id) {
        Modifier *modifier = nullptr;
        ModifierLookup::iterator mod_found = _modifier_lookup.find(id);

        if (mod_found != _modifier_lookup.end()) {
            modifier = mod_found->second;
        }
    
        return modifier;
    }

}; /* Modifier class */


}  /* Inkscape namespace */

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
