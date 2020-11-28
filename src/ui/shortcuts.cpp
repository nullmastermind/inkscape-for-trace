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
#include "inkscape-application.h"
#include "inkscape-window.h"

#include "verbs.h"
#include "helper/action.h"
#include "helper/action-context.h"

#include "io/resource.h"
#include "io/dir-util.h"

#include "ui/modifiers.h"
#include "ui/tools/tool-base.h"    // For latin keyval
#include "ui/dialog/filedialog.h"  // Importing/exporting files.

#include "xml/simple-document.h"
#include "xml/node.h"
#include "xml/node-iterators.h"

using namespace Inkscape::IO::Resource;
using namespace Inkscape::Modifiers;

namespace Inkscape {

Shortcuts::Shortcuts()
{
    Glib::RefPtr<Gio::Application> gapp = Gio::Application::get_default();
    app = Glib::RefPtr<Gtk::Application>::cast_dynamic(gapp); // Save as we constantly use it.
    if (!app) {
        std::cerr << "Shortcuts::Shortcuts: No app! Shortcuts cannot be used without a Gtk::Application!" << std::endl;
    }
}


void
Shortcuts::init() {

    initialized = true;

    // Clear arrays (we may be re-reading).
    clear();

    bool success = false; // We've read a shortcut file!
    std::string path;
  
    // ------------ Open Inkscape shortcut file ------------

    // Try filename from preferences first.
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    path = prefs->getString("/options/kbshortcuts/shortcutfile");
    if (!path.empty()) {
        bool absolute = true;
        if (!Glib::path_is_absolute(path)) {
            path = get_path_string(SYSTEM, KEYS, path.c_str());
            absolute = false;
        }

        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
        success = read(file);
        if (!success) {
            std::cerr << "Shortcut::Shortcut: Unable to read shortcut file listed in preferences: " + path << std::endl;;
        }

        // Save relative path to "share/keys" if possible to handle parallel installations of
        // Inskcape gracefully.
        if (success && absolute) {
            std::string relative_path = sp_relative_path_from_path(path, std::string(get_path(SYSTEM, KEYS)));
            prefs->setString("/options/kbshortcuts/shortcutfile", relative_path.c_str());
        }
    }

    if (!success) {
        // std::cerr << "Shortcut::Shortcut: " << reason << ", trying default.xml" << std::endl;

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
    // Test if file exists before attempting to read to avoid generating warning message.
    if (file->query_exists()) {
        read(file, true);
    }

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

    // Actions: We rely on Gtk for everything except user/system setting.
    for (auto action_description : app->list_action_descriptions()) {
        app->unset_accels_for_action(action_description);
    }
    action_user_set.clear();
}


Gdk::ModifierType
parse_modifier_string(gchar const *modifiers_string, gchar const *verb_name)
{
    Gdk::ModifierType modifiers(Gdk::ModifierType(0));
    if (modifiers_string) {
  
        Glib::ustring str(modifiers_string);
        std::vector<Glib::ustring> mod_vector = Glib::Regex::split_simple("\\s*,\\s*", modifiers_string);
  
        for (auto mod : mod_vector) {
            if (mod == "Control" || mod == "Ctrl") {
                modifiers |= Gdk::CONTROL_MASK;
            } else if (mod == "Shift") {
                modifiers |= Gdk::SHIFT_MASK;
            } else if (mod == "Alt") {
                modifiers |= Gdk::MOD1_MASK;
            } else if (mod == "Super") {
                modifiers |= Gdk::SUPER_MASK; // Not used
            } else if (mod == "Hyper") {
                modifiers |= Gdk::HYPER_MASK; // Not used
            } else if (mod == "Meta") {
                modifiers |= Gdk::META_MASK;
            } else if (mod == "Primary") {
  
                // System dependent key to invoke menus. (Needed for OSX in particular.)
                // We only read "Primary" and never write it for verbs.
                auto display = Gdk::Display::get_default();
                if (display) {
                    GdkKeymap* keymap = display->get_keymap();
                    GdkModifierType type = 
                        gdk_keymap_get_modifier_mask (keymap, GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
                    gdk_keymap_add_virtual_modifiers(keymap, &type);
                    if (type & Gdk::CONTROL_MASK)
                        modifiers |= Gdk::CONTROL_MASK;
                    else if (type & Gdk::META_MASK)
                        modifiers |= Gdk::META_MASK;
                    else {
                        std::cerr << "Shortcut::read: Unknown primary accelerator!" << std::endl;
                        modifiers |= Gdk::CONTROL_MASK;
                    }
                } else {
                    modifiers |= Gdk::CONTROL_MASK;
                }
            } else {
                std::cerr << "Shortcut::read: Unknown GDK modifier: " << mod.c_str() << std::endl;
            }
        }
    }
    return modifiers;
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

        if (strcmp(iter->name(), "modifier") == 0) {

            gchar const *mod_name = iter->attribute("action");
            if (!mod_name) {
                std::cerr << "Shortcuts::read: Missing modifier for action!" << std::endl;;
                continue;
            }

            Modifier *mod = Modifier::get(mod_name);
            if (mod == nullptr) {
                std::cerr << "Shortcuts::read: Can't find modifer: " << mod_name << std::endl; 
                continue;
            }
 
            // If mods isn't specified then it should use default, if it's an empty string
            // then the modifier is None (i.e. happens all the time without a modifier)
            gchar const *mod_attr = iter->attribute("modifiers");
            if (!mod_attr) {
                continue; // Default, do nothing and no warning.
            }

            KeyMask and_modifier = (KeyMask) parse_modifier_string(mod_attr, mod_name);

            // Parse not (cold key) modifier
            KeyMask not_modifier = NOT_SET;
            gchar const *not_attr = iter->attribute("not_modifiers");
            if (not_attr) {
                not_modifier = (KeyMask) parse_modifier_string(not_attr, mod_name);
            }

            if(user_set) {
                mod->set_user(and_modifier, not_modifier);
            } else {
                mod->set_keys(and_modifier, not_modifier);
            }
            continue;

        } else if (strcmp(iter->name(), "bind") != 0) {
            // Unknown element, do not complain.
            continue;
        }

        // Gio::Action's
        gchar const *gaction = iter->attribute("gaction");
        gchar const *keys    = iter->attribute("keys");
        if (gaction && keys) {

            std::vector<Glib::ustring> key_vector = Glib::Regex::split_simple("\\s*,\\s*", keys);
            // Set one shortcut at a time so we can check if it has been previously used.
            for (auto key : key_vector) {
                add_shortcut(gaction, key, user_set);
            }

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

        Gdk::ModifierType modifiers = parse_modifier_string(iter->attribute("modifiers"), verb_name);

        add_shortcut (verb_name, Gtk::AccelKey(keyval, modifiers), user_set, is_primary);
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
            unsigned int      key_val = entry.first.get_key();
            Gdk::ModifierType mod_val = entry.first.get_mod();

            gchar *key = gdk_keyval_name (key_val);
            Glib::ustring mod = get_modifiers_verb (mod_val);
            Glib::ustring id  = verb->get_id();
            
            XML::Node * node = document->createElement("bind");
            node->setAttribute("key", key);
            node->setAttributeOrRemoveIfEmpty("modifiers", mod);
            node->setAttribute("action", id);
            if (primary[verb].get_key() == entry.first.get_key() && primary[verb].get_mod() == entry.first.get_mod()) {
                node->setAttribute("display", "true");
            }
            document->root()->appendChild(node);
        }
    }

    // Actions: write out all actions with accelerators.
    for (auto action_description : app->list_action_descriptions()) {
        if ( what == All                                 ||
            (what == System && !action_user_set[action_description]) ||
            (what == User   &&  action_user_set[action_description]) )
        {
            std::vector<Glib::ustring> accels = app->get_accels_for_action(action_description);
            if (!accels.empty()) {

                XML::Node * node = document->createElement("bind");

                node->setAttribute("gaction", action_description);

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

// Return the primary shortcut for a verb or GDK_KEY_VoidSymbol if not found.
Gtk::AccelKey
Shortcuts::get_shortcut_from_verb(Verb *verb)
{
    for (auto const& it : shortcut_to_verb_map) {
        if (it.second == verb) {
            return primary[verb];
        }
    }

    return (Gtk::AccelKey());
}


// Return verb corresponding to shortcut or nullptr if no verb.
Verb*
Shortcuts::get_verb_from_shortcut(const Gtk::AccelKey& shortcut)
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
    Gtk::AccelKey shortcut = get_from_event(event);

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

// Get a list of detailed action names (as defined in action extra data).
// This is more useful for shortcuts than a list of all actions.
std::vector<Glib::ustring>
Shortcuts::list_all_detailed_action_names()
{
    auto *iapp = InkscapeApplication::instance();
    InkActionExtraData& action_data = iapp->get_action_extra_data();
    return action_data.get_actions();
}

// Get a list of all actions (application, window, and document), properly prefixed.
// We need to do this ourselves as Gtk::Application does not have a function for this.
std::vector<Glib::ustring>
Shortcuts::list_all_actions()
{
    std::vector<Glib::ustring> all_actions;

    std::vector<Glib::ustring> actions = app->list_actions();
    std::sort(actions.begin(), actions.end());
    for (auto action : actions) {
        all_actions.emplace_back("app." + action);
    }

    auto gwindow = app->get_active_window();
    auto window = dynamic_cast<InkscapeWindow *>(gwindow);
    if (window) {
        std::vector<Glib::ustring> actions = window->list_actions();
        std::sort(actions.begin(), actions.end());
        for (auto action : actions) {
            all_actions.emplace_back("win." + action);
        }

        auto document = window->get_document();
        if (document) {
            auto map = document->getActionGroup();
            if (map) {
                std::vector<Glib::ustring> actions = map->list_actions();
                for (auto action : actions) {
                    all_actions.emplace_back("doc." + action);
                }
            } else {
                std::cerr << "Shortcuts::list_all_actions: No document map!" << std::endl;
            }
        }
    }

    return all_actions;
}


// Add a shortcut, removing any previous use of shortcut.
// is_primary is for use with verbs and can be removed after verbs are gone.
bool
Shortcuts::add_shortcut(Glib::ustring name, const Gtk::AccelKey& shortcut, bool user, bool is_primary)
{
    // Remove previous use of shortcut (already removed if new user shortcut).
    if (Glib::ustring old_name = remove_shortcut(shortcut); old_name != "") {
        std::cerr << "Shortcut::add_shortcut: duplicate shortcut found for: " << shortcut.get_abbrev()
                  << "  Old: " << old_name << "  New: " << name << " !" << std::endl;
    }

    // Add shortcut

    // Try verb first
    Verb* verb = Verb::getbyid(name.c_str(), false); // false => no error message
    if (verb) {
        shortcut_to_verb_map[shortcut] = verb;
        if (is_primary) {
            primary[verb] = shortcut;
            user_set[verb] = user;
        }
        return true;
    }

    // To be removed after verbs are gone and initialization happens in InkscapeWindow constructor.
    // We can then check if action exists before assigning shortcut to it.
    // If not verb, must be action!
    std::vector<Glib::ustring> accels = app->get_accels_for_action(name);
    accels.push_back(shortcut.get_abbrev());
    app->set_accels_for_action(name, accels);
    action_user_set[name] = user;
    return true;

    // To be uncommented after verbs are gone.
    // for (auto action : list_all_detailed_action_names()) {
    //     if (action == name) {
    //         // Action exists
    //         app->set_accel_for_action(action, shortcut.get_abbrev());
    //         action_user_set[action] = user;
    //         return true;
    //     }
    // }

    // // Oops, not an action either!
    // std::cerr << "Shortcuts::add_shortcut: No Verb or Action for " << name << std::endl;
    // return false;
}


// Add a user shortcut, updating user's shortcut file if successful.
bool
Shortcuts::add_user_shortcut(Glib::ustring name, const Gtk::AccelKey& shortcut)
{
    // Remove previous shortcut(s) for verb/action.
    remove_shortcut(name);

    // Remove previous use of shortcut from other verbs/actions.
    remove_shortcut(shortcut);

    // Add shortcut, if successful, save to file.
    if (add_shortcut(name, shortcut, true, true)) {  // Always user, always primary (verbs only).
        // Save
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
        write(file, User);
        return true;
    }

    std::cerr << "Shortcut::add_user_shortcut: Failed to add: " << name << " with shortcut " << shortcut.get_abbrev() << std::endl;
    return false;
};


// Remove a shortcut via key. Return name of removed verb or action.
Glib::ustring
Shortcuts::remove_shortcut(const Gtk::AccelKey& shortcut)
{
    // Try verb first
    Verb *verb = shortcut_to_verb_map[shortcut];
    if (verb) {
        shortcut_to_verb_map.erase(shortcut);
        primary[verb] = Gtk::AccelKey();
        user_set[verb] = false;
        return verb->get_id();
    }

    // Try action second
    std::vector<Glib::ustring> actions = app->get_actions_for_accel(shortcut.get_abbrev());
    if (actions.empty()) {
        return Glib::ustring(); // No verb, no action, no pie.
    }

    Glib::ustring action_name;
    for (auto action : actions) {
        // Remove just the one shortcut, leaving the others intact.
        std::vector<Glib::ustring> accels = app->get_accels_for_action(action);
        auto it = std::find(accels.begin(), accels.end(), shortcut.get_abbrev());
        if (it != accels.end()) {
            action_name = action;
            accels.erase(it);
        }
        app->set_accels_for_action(action, accels);
    }

    return action_name;
}


// Remove a shortcut via verb/action name.
bool
Shortcuts::remove_shortcut(Glib::ustring name)
{
    // Try verb first
    Verb* verb = Verb::getbyid(name.c_str(), false); // Not verbose!
    if (verb) {
        Gtk::AccelKey shortcut = get_shortcut_from_verb(verb);
        shortcut_to_verb_map.erase(shortcut);
        primary[verb] = Gtk::AccelKey();
        user_set[verb] = false;
        return true;
    }

    // Try action second
    for (auto action : list_all_detailed_action_names()) {
        if (action == name) {
            // Action exists
            app->unset_accels_for_action(action);
            action_user_set.erase(action);
            return true;
        }
    }

    return false;
}

// Remove a user shortcut, updating user's shortcut file.
bool
Shortcuts::remove_user_shortcut(Glib::ustring name)
{
    // Check if really user shortcut.
    bool user_shortcut = false;
    Verb *verb = Verb::getbyid(name.c_str(), false); // Not verbose
    if (verb) {
        user_shortcut = is_user_set(verb);
    } else {
        user_shortcut = is_user_set(name);
    }

    if (!user_shortcut) {
        // We don't allow removing non-user shortcuts.
        return false;
    }

    if (remove_shortcut(name)) {
        // Save
        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
        write(file, User);

        // Reread to get original shortcut (if any).
        init();
        return true;
    }

    std::cerr << "Shortcuts::remove_user_shortcut: Failed to remove shortcut for: " << name << std::endl;
    return false;
}


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
Shortcuts::get_label(const Gtk::AccelKey& shortcut)
{
    Glib::ustring label;

    if (!shortcut.is_null()) {
        label = Gtk::AccelGroup::get_label(shortcut.get_key(), shortcut.get_mod());
    }

    return label;
}

Glib::ustring
Shortcuts::get_modifiers_verb(unsigned int mod_val)
{
    Glib::ustring modifiers;
    if (mod_val & Gdk::CONTROL_MASK) modifiers += "Ctrl,";
    if (mod_val & Gdk::SHIFT_MASK)   modifiers += "Shift,";
    if (mod_val & Gdk::MOD1_MASK)    modifiers += "Alt,";
    if (mod_val & Gdk::SUPER_MASK)   modifiers += "Super,";
    if (mod_val & Gdk::HYPER_MASK)   modifiers += "Hyper,";
    if (mod_val & Gdk::META_MASK)    modifiers += "Meta,";

    if (modifiers.length() > 0) {
        modifiers.resize(modifiers.size() -1);
    }

    return modifiers;
}

Glib::ustring
Shortcuts::shortcut_to_accelerator(const Gtk::AccelKey& shortcut)
{
    unsigned int keyval = shortcut.get_key();
    unsigned int modval = shortcut.get_mod();

    Glib::ustring accelerator;
    if (modval & Gdk::CONTROL_MASK) accelerator += "<Ctrl>";
    if (modval & Gdk::SHIFT_MASK)   accelerator += "<Shift>";
    if (modval & Gdk::MOD1_MASK)    accelerator += "<Alt>";
    if (modval & Gdk::SUPER_MASK)   accelerator += "<Super>";
    if (modval & Gdk::HYPER_MASK)   accelerator += "<Hyper>";
    if (modval & Gdk::META_MASK)    accelerator += "<Meta>";

    gchar* key = gdk_keyval_name(keyval);
    if (key) {
        accelerator += key;
    }

    // Glib::ustring accelerator2 = Gtk::AccelGroup::name(keyval, Gdk::ModifierType(modval));
    // Glib::ustring accelerator3 = Gtk::AccelGroup::get_label(keyval, Gdk::ModifierType(modval));

    // std::cout << "accelerator: " << accelerator << " " << accelerator2 << " " << accelerator3 << std::endl;
    return accelerator;
}

Gtk::AccelKey
Shortcuts::accelerator_to_shortcut(const Glib::ustring& accelerator)
{
    Gdk::ModifierType modval = Gdk::ModifierType(0);
    std::vector<Glib::ustring> parts = Glib::Regex::split_simple("<(<.*?>)", accelerator);
    for (auto part : parts) {
        if (part == "<Ctrl>")  modval |= Gdk::CONTROL_MASK;
        if (part == "<Shift>") modval |= Gdk::SHIFT_MASK;
        if (part == "<Alt>")   modval |= Gdk::MOD1_MASK;
        if (part == "<Super>") modval |= Gdk::SUPER_MASK;
        if (part == "<Hyper>") modval |= Gdk::HYPER_MASK;
        if (part == "<Meta>")  modval |= Gdk::META_MASK;
        if (part == "<Primary>") std::cerr << "Shortcuts::accelerator_to_shortcut: need to handle 'Primary'!" << std::endl;
    }

    unsigned int keyval = gdk_keyval_from_name(parts[parts.size()-1].c_str());

    return Gtk::AccelKey(keyval, modval);
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
Gtk::AccelKey
Shortcuts::get_from_event(GdkEventKey const *event, bool fix)
{
    // MOD2 corresponds to the NumLock key. Masking it out allows
    // shortcuts to work regardless of its state.
    Gdk::ModifierType initial_modifiers  = Gdk::ModifierType(event->state & ~Gdk::MOD2_MASK);
    unsigned int consumed_modifiers = 0;
    //Gdk::ModifierType consumed_modifiers = Gdk::ModifierType(0);

    unsigned int keyval = Inkscape::UI::Tools::get_latin_keyval(event, &consumed_modifiers);

    // If a key value is "convertible", i.e. it has different lower case and upper case versions,
    // convert to lower case and don't consume the "shift" modifier.
    bool is_case_convertible = !(gdk_keyval_is_upper(keyval) && gdk_keyval_is_lower(keyval));
    if (is_case_convertible) {
        keyval = gdk_keyval_to_lower(keyval);
        consumed_modifiers &= ~ Gdk::SHIFT_MASK;
    }

    // The InkscapePreferences dialog returns an event structure where the Shift modifier is not
    // set for keys like '('. This causes '(' to be converted to '9' by get_latin_keyval. It also
    // returns 'Shift-k' for 'K' (instead of 'Shift-K') but this is not a problem.
    // We fix this by restoring keyval to its original value.
    if (fix) {
        keyval = event->keyval;
    }

    // std::cout << "Shortcuts::get_from_event: End:   "
    //           << " Key: " << std::hex << keyval << " (" << (char)keyval << ")"
    //           << " Mod: " << std::hex << (initial_modifiers &~ consumed_modifiers) << std::endl;
    return (Gtk::AccelKey(keyval, Gdk::ModifierType(initial_modifiers &~ consumed_modifiers)));
}


// Add an accelerator to the group.
void
Shortcuts::add_accelerator (Gtk::Widget *widget, Verb *verb)
{
    Gtk::AccelKey shortcut = get_shortcut_from_verb(verb);
    
    if (shortcut.is_null()) {
        return;
    }

    static Glib::RefPtr<Gtk::AccelGroup> accel_group = Gtk::AccelGroup::create();

    widget->add_accelerator ("activate", accel_group, shortcut.get_key(), shortcut.get_mod(), Gtk::ACCEL_VISIBLE);
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

// void on_foreach(Gtk::Widget& widget) {
//     std::cout <<  "on_foreach: " << widget.get_name() << std::endl;;
// }

/*
 * Update text with shortcuts.
 * Inkscape includes shortcuts in tooltips and in dialog titles. They need to be updated
 * anytime a tooltip is changed.
 */
void
Shortcuts::update_gui_text_recursive(Gtk::Widget* widget)
{

    // NOT what we want
    // auto activatable = dynamic_cast<Gtk::Activatable *>(widget);

    // We must do this until GTK4
    GtkWidget* gwidget = widget->gobj();
    bool is_actionable = GTK_IS_ACTIONABLE(gwidget);

    if (is_actionable) {
        const gchar* gaction = gtk_actionable_get_action_name(GTK_ACTIONABLE(gwidget));
        if (gaction) {

            Glib::ustring action = gaction;
            std::vector<Glib::ustring> accels = app->get_accels_for_action(action);

            Glib::ustring tooltip;
            auto *iapp = InkscapeApplication::instance();
            if (iapp) {
                tooltip = iapp->get_action_extra_data().get_tooltip_for_action(action);
            }

            // Add new primary accelerator.
            if (accels.size() > 0) {

                // Add space between tooltip and accel if there is a tooltip
                if (!tooltip.empty()) {
                    tooltip += " ";
                }

                // Convert to more user friendly notation.
                unsigned int key = 0;
                Gdk::ModifierType mod = Gdk::ModifierType(0);
                Gtk::AccelGroup::parse(accels[0], key, mod);
                tooltip += "(" + Gtk::AccelGroup::get_label(key, mod) + ")";
            }

            // Update tooltip.
            widget->set_tooltip_text(tooltip);
        }
    }

    auto container = dynamic_cast<Gtk::Container *>(widget);
    if (container) {
        auto children = container->get_children();
        for (auto child : children) {
            update_gui_text_recursive(child);
        }
    }

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

    Glib::RefPtr<Gio::File> file_read = Gio::File::create_for_path(path);
    if (!read(file_read, true)) {
        std::cerr << "Shortcuts::import_shortcuts: Failed to read file!" << std::endl;
        return false;
    }

    // Save
    Glib::RefPtr<Gio::File> file_save = Gio::File::create_for_path(get_path_string(USER, KEYS, "default.xml"));
    return write(file_save, User);
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

            Gtk::AccelKey shortcut(key, mod);
            Inkscape::Verb *verb = get_verb_from_shortcut(shortcut);
            if (verb) {
                action = verb->get_name();
            }

            std::cout << "  shortcut:"
                      << "  " << std::setw(8) << std::hex << shortcut.get_mod()
                      << "  " << std::setw(8) << std::hex << shortcut.get_key()
                      << "  " << std::setw(30) << std::left << accel
                      << "  " << action
                      << std::endl;
        }
    }
}

void
Shortcuts::dump_all_recursive(Gtk::Widget* widget)
{
    static unsigned int indent = 0;
    ++indent;
    for (int i = 0; i < indent; ++i) std::cout << "  ";

    // NOT what we want
    // auto activatable = dynamic_cast<Gtk::Activatable *>(widget);

    // We must do this until GTK4
    GtkWidget* gwidget = widget->gobj();
    bool is_actionable = GTK_IS_ACTIONABLE(gwidget);
    Glib::ustring action;
    if (is_actionable) {
        const gchar* gaction = gtk_actionable_get_action_name( GTK_ACTIONABLE(gwidget) );
        if (gaction) {
            action = gaction;
        }
    }

    std::cout << widget->get_name()
              << ":   actionable: " << std::boolalpha << is_actionable
              << ":   " << widget->get_tooltip_text()
              << ":   " << action
              << std::endl;
    auto container = dynamic_cast<Gtk::Container *>(widget);
    if (container) {
        auto children = container->get_children();
        for (auto child : children) {
            dump_all_recursive(child);
        }
    }
    --indent;
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
