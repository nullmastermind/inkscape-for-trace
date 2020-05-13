// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Shortcuts
 *
 * Copyright (C) 2020 Tavmjong Bah
 * Rewrite of code (C) MenTalguY and others.
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */


/* Much of the complexity of this code is in dealing with both Inkscape verbs and Gio::Actions at
 * the same time. When we remove verbs we can avoid using 'unsigned long long int shortcut' to
 * track keys and rely directly on Glib::ustring as used by
 * Gtk::Application::get_accels_for_action(). This will then automatically handle the '<Primary>'
 * modifier value (which takes care of the differences between Linux and OSX) as well as allowing
 * us to set multiple accelerators for actions in InkscapePreferences. */

#include "shortcuts.h"

#include <iostream>
#include <iomanip>

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "preferences.h"
#include "verbs.h"
#include "helper/action.h"
#include "helper/action-context.h"

#include "io/resource.h"
#include "io/dir-util.h"

#include "ui/tools/tool-base.h"    // For latin keyval
#include "ui/dialog/filedialog.h"  // Importing/exporting files.

#include "xml/simple-document.h"
#include "xml/node.h"
#include "xml/node-iterators.h"

using namespace Inkscape::IO::Resource;

namespace Inkscape {


Shortcuts::Shortcuts()
{
    Glib::RefPtr<Gio::Application> gapp = Gio::Application::get_default();
    app = Glib::RefPtr<Gtk::Application>::cast_dynamic(gapp); // Save as we constantly use it.
}


void
Shortcuts::init() {

    // Clear arrays (we may be re-reading).
    clear();

    bool success = false; // We've read a shortcut file!
    std::string path;
  
    // ------------ Open Inkscape shortcut file ------------

    // Try filename from preferences first.
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Glib::ustring reason;
    path = prefs->getString("/options/kbshortcuts/shortcutfile");
    if (path.empty()) {
        reason = "No shortcut file set in preferences";
    } else {
        bool absolute = true;
        if (!Glib::path_is_absolute(path)) {
            path = get_path_string(SYSTEM, KEYS, path.c_str());
            absolute = false;
        }

        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
        success = read(file);

        // Save relative path to "share/keys" if possible to handle parallel installations of
        // Inskcape gracefully.
        if (success && absolute) {
            std::string relative_path = sp_relative_path_from_path(path, std::string(get_path(SYSTEM, KEYS)));
            prefs->setString("/options/kbshortcuts/shortcutfile", relative_path.c_str());
        }

        reason = "Unable to read shortcut file listed in preferences: " + path;
    }

    if (!success) {
        std::cerr << "Shortcut::Shortcut: " << reason << ", trying default.xml" << std::endl;

        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(SYSTEM, KEYS, "default.xml"));
        success = read(file);
    }
  
    if (!success) {
        std::cerr << "Shortcut::Shortcut: Failed to read file default.xml, trying inkscape.xml" << std::endl;

        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(SYSTEM, KEYS, "inkscape.xml"));
        success = read(file);
    }

    if (!success) {
        std::cerr << "Shortcut::Shortcut: Failed to read file inkscape.xml; giving up!" << std::endl;
    }

 
    // ------------ Open User shortcut file -------------
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
    read(file, true); 

    // dump();
}

// Clear all shortcuts
void
Shortcuts::clear()
{
    // Verbs: We track everything
    shortcut_to_verb_map.clear();
    primary.clear();
    user_set.clear();

    // NEED TO ADD WIN AND DOC!
    // Actions: We rely on Gtk for everything except user/system setting.
    action_user_set.clear();
    std::vector<Glib::ustring> actions = app->list_actions();
    for (auto action : actions) {
        app->unset_accels_for_action("app." + action);
    }
}


// Read a shortcut file. We do not check for conflicts between verbs and actions.
bool
Shortcuts::read(Glib::RefPtr<Gio::File> file, bool user_set)
{
    if (!file->query_exists()) {
        std::cerr << "Shortcut::read: file does not exist: " << file->get_path() << std::endl;
        return false;
    }

    XML::Document *document = sp_repr_read_file(file->get_path().c_str(), nullptr);
    if (!document) {
        std::cerr << "Shortcut::read: could not parse file: " << file->get_path() << std::endl;
        return false;
    }

    XML::NodeConstSiblingIterator iter = document->firstChild();
    for ( ; iter ; ++iter ) { // We iterate in case of comments.
        if (strcmp(iter->name(), "keys") == 0) {
            break;
        }
    }

    if (!iter) {
        std::cerr << "Shortcuts::read: File in wrong format: " << file->get_path() << std::endl;
        return false;
    }

    // Loop through children of <keys>
    iter = iter->firstChild();
    for ( ; iter ; ++iter ) {

        if (strcmp(iter->name(), "bind") != 0) {
            // Unknown element, do not complain.
            continue;
        }

        // Gio::Action's
        gchar const *gaction = iter->attribute("gaction");
        gchar const *keys    = iter->attribute("keys");
        if (gaction && keys) {

            std::vector<Glib::ustring> key_vector = Glib::Regex::split_simple("\\s*,\\s*", keys);
            app->set_accels_for_action(gaction, key_vector);

            action_user_set[gaction] = user_set;

            // Uncomment to see what the cat dragged in.
            // if (!key_vector.empty()) {
            //     std::cout << "Shortcut::read: gaction: "<< gaction
            //               << ", user set: " << std::boolalpha << user_set << ", ";
            //     for (auto key : key_vector) {
            //         std::cout << key << ", ";
            //     }
            //     std::cout << std::endl;
            // }

            continue;
        }

        // Legacy verbs
        bool is_primary =
            iter->attribute("display")                        &&
            strcmp(iter->attribute("display"), "false") != 0  &&
            strcmp(iter->attribute("display"), "0")     != 0;

        gchar const *verb_name = iter->attribute("action");
        if (!verb_name) {
            std::cerr << "Shortcut::read: Missing verb name!" << std::endl;
            continue;
        }

        Inkscape::Verb *verb = Inkscape::Verb::getbyid(verb_name);
        if (!verb) {
            std::cerr << "Shortcut::read: invalid verb: " << verb_name << std::endl;
            continue;
        }

        gchar const *keyval_name = iter->attribute("key");
        if (!keyval_name  ||!*keyval_name) {
            // OK. Verb without shortcut (for reference).
            continue;
        }

        guint keyval = gdk_keyval_from_name(keyval_name);
        if (keyval == GDK_KEY_VoidSymbol || keyval == 0) {
            std::cerr << "Shortcut::read: Unknown keyval " << keyval_name << " for " << verb_name << std::endl;
            continue;
        }

        unsigned long long int modifiers = 0;
        gchar const *modifiers_string=iter->attribute("modifiers");
        if (modifiers_string) {

            Glib::ustring str(modifiers_string);
            std::vector<Glib::ustring> mod_vector = Glib::Regex::split_simple("\\s*,\\s*", modifiers_string);

            for (auto mod : mod_vector) {
                if (mod == "Control" || mod == "Ctrl") {
                    modifiers |= GDK_CONTROL_MASK;
                } else if (mod == "Shift") {
                    modifiers |= GDK_SHIFT_MASK;
                } else if (mod == "Alt") {
                    modifiers |= GDK_MOD1_MASK;
                } else if (mod == "Super") {
                    modifiers |= GDK_SUPER_MASK; // Not used
                } else if (mod == "Hyper") {
                    modifiers |= GDK_HYPER_MASK; // Not used
                } else if (mod == "Meta") {
                    modifiers |= GDK_META_MASK;
                } else if (mod == "Primary") {

                    // System dependent key to invoke menus. (Needed for OSX in particular.)
                    // We only read "Primary" and never write it for verbs.
                    auto display = Gdk::Display::get_default();
                    if (display) {
                        GdkKeymap* keymap = display->get_keymap();
                        GdkModifierType type =
                            gdk_keymap_get_modifier_mask (keymap, GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
                        gdk_keymap_add_virtual_modifiers(keymap, &type);
                        if (type & GDK_CONTROL_MASK)
                            modifiers |= GDK_CONTROL_MASK;
                        else if (type & GDK_META_MASK)
                            modifiers |= GDK_META_MASK;
                        else {
                            std::cerr << "Shortcut::read: Unknown primary accelerator!" << std::endl;
                            modifiers |= GDK_CONTROL_MASK;
                        }
                    } else {
                        modifiers |= GDK_CONTROL_MASK;
                    }

                } else {
                    std::cerr << "Shortcut::read: Unknown GDK modifier: " << mod << std::endl;
                }
            }
        }

        set_verb_shortcut (keyval | modifiers << 32, verb, is_primary, user_set);
    }

    return true;
}

// In principle, we only write User shortcuts. But for debugging, we might want to write something else.
bool
Shortcuts::write(Glib::RefPtr<Gio::File> file, What what) {

    auto *document = new XML::SimpleDocument();
    XML::Node * node = document->createElement("keys");
    switch (what) {
        case User:
            node->setAttribute("name", "User Shortcuts");
            break;
        case System:
            node->setAttribute("name", "System Shortcuts");
            break;
        default:
            node->setAttribute("name", "Inkscape Shortcuts");
    }

    document->appendChild(node);

    // Legacy verbs
    for (auto entry : shortcut_to_verb_map) {
        Verb *verb = entry.second;
        if ( what == All                        ||
            (what == System && !user_set[verb]) ||
            (what == User   &&  user_set[verb]) )  
        {
            int key_val = entry.first & 0xffffffff;
            int mod_val = entry.first >> 32;

            gchar *key = gdk_keyval_name (key_val);
            Glib::ustring mod = get_modifiers_verb (mod_val);
            Glib::ustring id  = verb->get_id();
            
            XML::Node * node = document->createElement("bind");
            node->setAttribute("key", key);
            node->setAttributeOrRemoveIfEmpty("modifiers", mod);
            node->setAttribute("action", id);
            if (primary[verb] == entry.first) {
                node->setAttribute("display", "true");
            }
            document->root()->appendChild(node);
        }
    }

    // Actions
    // NEED TO ADD WIN AND DOC (turn into function?).
    std::vector<Glib::ustring> actions = app->list_actions();
    std::sort(actions.begin(), actions.end());
    for (auto action : actions) {
        Glib::ustring fullname("app." + action);
        if ( what == All                                 ||
            (what == System && !action_user_set[fullname]) ||
            (what == User   &&  action_user_set[fullname]) )  
        {
            std::vector<Glib::ustring> accels = app->get_accels_for_action(fullname);
            if (!accels.empty()) {

                XML::Node * node = document->createElement("bind");

                node->setAttribute("gaction", fullname);

                Glib::ustring keys;
                for (auto accel : accels) {
                    keys += accel;
                    keys += ",";
                }
                keys.resize(keys.size() - 1);
                node->setAttribute("keys", keys);

                document->root()->appendChild(node);
            }
        }
    }

    sp_repr_save_file(document, file->get_path().c_str(), nullptr);
    GC::release(document);

    return true;
};

// Set a verb shortcut.
void
Shortcuts::set_verb_shortcut(unsigned long long int const &val, Inkscape::Verb *verb, bool is_primary, bool is_user_set)
{
    // val is key value in lower 32 bits and modifier in upper 32 bits.
    Inkscape::Verb *old_verb = shortcut_to_verb_map[val];
    shortcut_to_verb_map[val] = verb;

    // Clear old verb data (if key reassigned to different verb).
    if (old_verb && old_verb != verb) {
        unsigned long long int old_primary = primary[old_verb];

        if (old_primary == val) {
            primary[old_verb] = 0;
            user_set[old_verb] = false;
        }
    }

    if (is_primary) {
        primary[verb] = val;
        user_set[verb] = is_user_set;
    }
}


// Return the primary shortcut for a verb or GDK_KEY_VoidSymbol if not found.
unsigned long long int
Shortcuts::get_shortcut_from_verb(Verb *verb)
{
    for (auto const& it : shortcut_to_verb_map) {
        if (it.second == verb) {
            return primary[verb];
        }
    }

    return (unsigned long long int) GDK_KEY_VoidSymbol;
}


// Return verb corresponding to shortcut or nullptr if no verb.
Verb*
Shortcuts::get_verb_from_shortcut(unsigned long long int shortcut)
{
    auto it = shortcut_to_verb_map.find(shortcut);
    if (it != shortcut_to_verb_map.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

// Return if user set shortcut for verb.
bool
Shortcuts::is_user_set(Verb *verb)
{
    auto it = user_set.find(verb);
    if (it != user_set.end()) {
        return user_set[verb];
    } else {
        return false;
    }
}

// Return if user set shortcut for Gio::Action.
bool
Shortcuts::is_user_set(Glib::ustring& action)
{
    auto it = action_user_set.find(action);
    if (it != action_user_set.end()) {
        return action_user_set[action];
    } else {
        return false;
    }
}

// Invoke verb corresponding to shortcut.
bool
Shortcuts::invoke_verb(GdkEventKey const *event, UI::View::View *view)
{
    // std::cout << "Shortcuts::invoke_verb: "
    //           << std::hex << event->keyval << " "
    //           << std::hex << event->state << std::endl;
    unsigned long long int shortcut = get_from_event(event);

    Verb* verb = get_verb_from_shortcut(shortcut);
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(view));
        if (action) {
            sp_action_perform(action, nullptr);
            return true;
        }
    }

    return false;
}


// Adds to user's default.xml file.
bool
Shortcuts::add_user_shortcut(Glib::ustring name, unsigned long long int shortcut) {

    std::cout << "Shortcuts::add_user_shortcut: " << name << ": " << std::hex << shortcut << std::endl;

    // We must first remove any existing shortcuts that use the same shortcut value.
    Verb* old_verb = shortcut_to_verb_map[shortcut];
    if (old_verb) {
        primary[old_verb] = 0;
        user_set[old_verb] = false;
        shortcut_to_verb_map.erase(shortcut);
    }

    // Need to add win and doc.
    Glib::ustring accel = shortcut_to_accelerator(shortcut);
    std::vector<Glib::ustring> actions = app->get_actions_for_accel(accel);
    for (auto action : actions) {
        app->unset_accels_for_action(action);
        action_user_set.erase(action);
    }

    // Now we add new verb/action
    
    // Try verb first
    Verb* verb = Verb::getbyid(name.c_str(), false); // false => no error message
    if (verb) {
        shortcut_to_verb_map[shortcut] = verb;
        primary[verb] = shortcut;
        user_set[verb] = true;
    } else {
        // If not verb, it must be an action!
        bool found = false;

        // NEED TO ADD WINDOW AND DOCUMENT ACTIONS!
        std::vector<Glib::ustring> app_actions = app->list_actions();
        for (auto action : app_actions) {
            if (("app." + action) == name) {
                // Action exists
                app->set_accel_for_action(name, accel);
                action_user_set[name] = true;
                found = true;
                break;
            }
        }

        if (!found) {
            // Oops, not an action either!
            std::cerr << "Shortcuts::add_shortcut: No Verb or Action for " << name << std::endl;
            return false;
        }
    }

    // Save
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
    write(file, User);

    return true;
};

// Remove from user's default.xml file.
bool
Shortcuts::remove_user_shortcut(Glib::ustring name, unsigned long long int shortcut)
{
    bool removed = false;

    // Try verb first
    Verb* verb = Verb::getbyid(name.c_str(), false); // Not verbose!
    if (verb) {
        shortcut_to_verb_map.erase(shortcut);
        primary[verb] = 0;
        user_set[verb] = false;
        removed = true;
    } else {
        // If not verb, it must be an action!
        std::vector<Glib::ustring> actions = app->list_actions();
        for (auto action : actions) {
            Glib::ustring fullname("app." + action);
            if (fullname == name) {
                // Action exists
                app->unset_accels_for_action(fullname);
                action_user_set.erase(fullname);
                removed = true;
                break;
            }
        }
    }

    if (removed) {
        // Save
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
        write(file, User);
    } else {
        std::cerr << "Shortcuts::remove_user_shortcut: Failed to remove: " << name << std::endl;
    }
        
    return removed;
};

// Remove all user's shortcuts (simply overwrites existing file).
bool
Shortcuts::clear_user_shortcuts()
{
    // Create new empty document and save
    auto *document = new XML::SimpleDocument();
    XML::Node * node = document->createElement("keys");
    node->setAttribute("name", "User Shortcuts");
    document->appendChild(node);
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
    sp_repr_save_file(document, file->get_path().c_str(), nullptr);
    GC::release(document);
    
    // Re-read everything!
    init();
    return true;
}

Glib::ustring
Shortcuts::get_label(unsigned long long int shortcut)
{
    Glib::ustring label;

    int key_val = shortcut & 0xffffffff;
    int mod_val = shortcut >> 32;

    if (key_val != GDK_KEY_VoidSymbol) {
        label = Gtk::AccelGroup::get_label(key_val, (Gdk::ModifierType)mod_val);
    }

    return label;
}

Glib::ustring
Shortcuts::get_modifiers_verb(unsigned int mod_val)
{
    Glib::ustring modifiers;
    if (mod_val & GDK_CONTROL_MASK) modifiers += "Ctrl,";
    if (mod_val & GDK_SHIFT_MASK)   modifiers += "Shift,";
    if (mod_val & GDK_MOD1_MASK)    modifiers += "Alt,";
    if (mod_val & GDK_SUPER_MASK)   modifiers += "Super,";
    if (mod_val & GDK_HYPER_MASK)   modifiers += "Hyper,";
    if (mod_val & GDK_META_MASK)    modifiers += "Meta,";

    if (modifiers.length() > 0) {
        modifiers.resize(modifiers.size() -1);
    }

    return modifiers;
}

Glib::ustring
Shortcuts::shortcut_to_accelerator(unsigned long long int shortcut)
{
    unsigned int keyval = shortcut & 0xffffffff;
    unsigned int modval = shortcut >> 32;

    Glib::ustring accelerator;
    if (modval & GDK_CONTROL_MASK) accelerator += "<Ctrl>";
    if (modval & GDK_SHIFT_MASK)   accelerator += "<Shift>";
    if (modval & GDK_MOD1_MASK)    accelerator += "<Alt>";
    if (modval & GDK_SUPER_MASK)   accelerator += "<Super>";
    if (modval & GDK_HYPER_MASK)   accelerator += "<Hyper>";
    if (modval & GDK_META_MASK)    accelerator += "<Meta>";

    gchar* key = gdk_keyval_name(keyval);
    if (key) {
        accelerator += key;
    }

    Glib::ustring accelerator2 = Gtk::AccelGroup::name(keyval, Gdk::ModifierType(modval));
    Glib::ustring accelerator3 = Gtk::AccelGroup::get_label(keyval, Gdk::ModifierType(modval));

    std::cout << "accelerator: " << accelerator << " " << accelerator2 << " " << accelerator3 << std::endl;
    return accelerator;
}

unsigned long long int
Shortcuts::accelerator_to_shortcut(Glib::ustring& accelerator)
{
    unsigned int modval = 0;
    std::vector<Glib::ustring> parts = Glib::Regex::split_simple("<(<.*?>)", accelerator);
    for (auto part : parts) {
        if (part == "<Ctrl>")  modval |= GDK_CONTROL_MASK;
        if (part == "<Shift>") modval |= GDK_SHIFT_MASK;
        if (part == "<Alt>")   modval |= GDK_MOD1_MASK;
        if (part == "<Super>") modval |= GDK_SUPER_MASK;
        if (part == "<Hyper>") modval |= GDK_HYPER_MASK;
        if (part == "<Meta>")  modval |= GDK_META_MASK;
        if (part == "<Primary>") std::cerr << "Shortcuts::accelerator_to_shortcut: need to handle 'Primary'!" << std::endl;
    }

    int keyval = gdk_keyval_from_name(parts[parts.size()-1].c_str());

    unsigned long long int retval = keyval | (unsigned long long int)modval << 32;

    return retval;
}

/*
 * Return: keyval translated to group 0 in lower 32 bits, modifier encoded in upper 32 bits.
 *
 * Usuage of group 0 (i.e. the main, typically English layout) instead of simply event->keyval
 * ensures that shortcuts work regardless of the active keyboard layout (e.g. Cyrillic).
 *
 * The returned modifiers are the modifiers that were not "consumed" by the translation and
 * can be used by the application to define a shortcut, e.g.
 *  - when pressing "Shift+9" the resulting character is "(";
 *    the shift key was "consumed" to make this character and should not be part of the shortcut
 *  - when pressing "Ctrl+9" the resulting character is "9";
 *    the ctrl key was *not* consumed to make this character and must be included in the shortcut
 *  - Exception: letter keys like [A-Z] always need the shift modifier,
 *               otherwise lower case and uper case keys are treated as equivalent.
 */
unsigned long long int
Shortcuts::get_from_event(GdkEventKey const *event)
{
    unsigned long long int initial_modifiers = event->state;
    unsigned int consumed_modifiers = 0;

    unsigned int keyval = Inkscape::UI::Tools::get_latin_keyval(event, &consumed_modifiers);

    // If a key value is "convertible", i.e. it has different lower case and upper case versions,
    // convert to lower case and don't consume the "shift" modifier.
    bool is_case_convertible = !(gdk_keyval_is_upper(keyval) && gdk_keyval_is_lower(keyval));
    if (is_case_convertible) {
        keyval = gdk_keyval_to_lower(keyval);
        consumed_modifiers &= ~ GDK_SHIFT_MASK;
    }

    return (keyval | (initial_modifiers &~ consumed_modifiers) << 32);
}


// Add an accelerator to the group.
void
Shortcuts::add_accelerator (Gtk::Widget *widget, Verb *verb)
{
    unsigned long long int shortcut = get_shortcut_from_verb(verb);
    
    if (shortcut == GDK_KEY_VoidSymbol || shortcut == 0) {
        return;
    }

    static Glib::RefPtr<Gtk::AccelGroup> accel_group = Gtk::AccelGroup::create();

    widget->add_accelerator ("activate", accel_group, shortcut & 0xffffffff,
                             Gdk::ModifierType(shortcut >> 32), Gtk::ACCEL_VISIBLE);
}


// Get a list of filenames to populate menu
std::vector<std::pair<Glib::ustring, Glib::ustring>>
Shortcuts::get_file_names()
{
    // TODO  Filenames should be std::string but that means changing the whole stack.
    using namespace Inkscape::IO::Resource;

    // Make a list of all key files from System and User.  Glib::ustring should be std::string!
    std::vector<Glib::ustring> filenames = get_filenames(SYSTEM, KEYS, {".xml"});
    // Exclude default.xml as it only contains user modifications.
    std::vector<Glib::ustring> filenames_user = get_filenames(USER, KEYS, {".xml"}, {"default.xml"});
    filenames.insert(filenames.end(), filenames_user.begin(), filenames_user.end());

    // Check file exists and extract out label if it does.
    std::vector<std::pair<Glib::ustring, Glib::ustring>> names_and_paths;
    for (auto &filename : filenames) {
        std::string label = Glib::path_get_basename(filename);
        Glib::ustring filename_relative = sp_relative_path_from_path(filename, std::string(get_path(SYSTEM, KEYS)));

        XML::Document *document = sp_repr_read_file(filename.c_str(), nullptr);
        if (!document) {
            std::cerr << "Shortcut::get_file_names: could not parse file: " << filename << std::endl;
            continue;
        }

        XML::NodeConstSiblingIterator iter = document->firstChild();
        for ( ; iter ; ++iter ) { // We iterate in case of comments.
            if (strcmp(iter->name(), "keys") == 0) {
                gchar const *name = iter->attribute("name");
                if (name) {
                    label = Glib::ustring(name) + " (" + label + ")";
                }
                std::pair<Glib::ustring, Glib::ustring> name_and_path = std::make_pair(label, filename_relative);
                names_and_paths.emplace_back(name_and_path);
                break;
            }
        }
        if (!iter) {
            std::cerr << "Shortcuts::get_File_names: not a shortcut keys file: " << filename << std::endl;
        }

        Inkscape::GC::release(document);
    }

    // Sort by name
    std::sort(names_and_paths.begin(), names_and_paths.end(),
            [](std::pair<Glib::ustring, Glib::ustring> pair1, std::pair<Glib::ustring, Glib::ustring> pair2) {
                return Glib::path_get_basename(pair1.first).compare(Glib::path_get_basename(pair2.first)) < 0;
            });

    // But default.xml at top
    auto it_default = std::find_if(names_and_paths.begin(), names_and_paths.end(),
            [](std::pair<Glib::ustring, Glib::ustring>& pair) {
                return !Glib::path_get_basename(pair.second).compare("default.xml");
            });
    if (it_default != names_and_paths.end()) {
        std::rotate(names_and_paths.begin(), it_default, it_default+1);
    }

    return names_and_paths;
}


// Dialogs

// Import user shortcuts from a file.
bool
Shortcuts::import_shortcuts() {

    // Users key directory.
    Glib::ustring directory = get_path_string(USER, KEYS, "");

    // Create and show the dialog
    Gtk::Window* window = app->get_active_window();
    Inkscape::UI::Dialog::FileOpenDialog *importFileDialog =
        Inkscape::UI::Dialog::FileOpenDialog::create(*window, directory, Inkscape::UI::Dialog::CUSTOM_TYPE, _("Select a file to import"));
    importFileDialog->addFilterMenu(_("Inkscape shortcuts (*.xml)"), "*.xml");
    bool const success = importFileDialog->show();

    if (!success) {
        delete importFileDialog;
        return false;
    }

    // Get file name and read.
    Glib::ustring path = importFileDialog->getFilename(); // It's a full path, not just a filename!
    delete importFileDialog;

    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
    return read(file, true);
};

bool
Shortcuts::export_shortcuts() {

    // Users key directory.
    Glib::ustring directory = get_path_string(USER, KEYS, "");

    // Create and show the dialog
    Gtk::Window* window = app->get_active_window();
    Inkscape::UI::Dialog::FileSaveDialog *saveFileDialog =
        Inkscape::UI::Dialog::FileSaveDialog::create(*window, directory, Inkscape::UI::Dialog::CUSTOM_TYPE, _("Select a filename for export"),
                                                     "", "", Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS);
    saveFileDialog->addFileType(_("Inkscape shortcuts (*.xml)"), "*.xml");
    bool success = saveFileDialog->show();

    // Get file name and write.
    if (success) {
        Glib::ustring path = saveFileDialog->getFilename(); // It's a full path, not just a filename!
        if (path.size() > 0) {
            Glib::ustring newFileName = Glib::filename_to_utf8(path);  // Is this really correct? (Paths should be std::string.)
            Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
            success = write(file, User);
        } else {
            // Can this ever happen?
            success = false;
        }
    }

    delete saveFileDialog;

    return success;
};


// For debugging.
void
Shortcuts::dump() {

    // What shortcuts are being used?
    std::vector<Gdk::ModifierType> modifiers {
        Gdk::ModifierType(0),
        Gdk::SHIFT_MASK,
        Gdk::CONTROL_MASK,
        Gdk::MOD1_MASK,
        Gdk::SHIFT_MASK   |  Gdk::CONTROL_MASK,
        Gdk::SHIFT_MASK   |  Gdk::MOD1_MASK,
        Gdk::CONTROL_MASK |  Gdk::MOD1_MASK,
        Gdk::SHIFT_MASK   |  Gdk::CONTROL_MASK   | Gdk::MOD1_MASK
    };
    for (auto mod : modifiers) {
        for (gchar key = '!'; key <= '~'; ++key) {

            Glib::ustring action;
            Glib::ustring accel = Gtk::AccelGroup::name(key, mod);
            std::vector<Glib::ustring> actions = app->get_actions_for_accel(accel);
            if (!actions.empty()) {
                action = actions[0];
            }

            unsigned long long int shortcut = key + ((unsigned long long int)mod << 32);
            Inkscape::Verb *verb = get_verb_from_shortcut(shortcut);
            if (verb) {
                action = verb->get_name();
            }

            std::cout << "  shortcut:  " << std::setw(10) << std::hex << shortcut
                      << "  " << std::setw(30) << std::left << accel
                      << "  " << action
                      << std::endl;
        }
    }
}

} // Namespace

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
