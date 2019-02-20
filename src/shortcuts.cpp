// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * Lauris Kaplinski <lauris@kaplinski.com>
 * MenTaLguY <mental@rydia.net>
 * bulia byak <buliabyak@users.sf.net>
 * Peter Moulder <pmoulder@mail.csse.monash.edu.au>
 * 2005 Monash University
 * 2005  MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/** \file
 * Keyboard shortcut processing.
 */
/*
 * Authors:
 *
 *
 * You may redistribute and/or modify this file under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <vector>
#include <cstring>
#include <string>
#include <map>

#include "shortcuts.h"
#include <gdk/gdkkeysyms.h>
#include <gdkmm/display.h>
#include <gtk/gtk.h>

#include <glibmm/i18n.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>

#include "helper/action.h"
#include "io/sys.h"
#include "io/resource.h"
#include "verbs.h"
#include "xml/node-iterators.h"
#include "xml/repr.h"
#include "document.h"
#include "preferences.h"
#include "ui/tools/tool-base.h"
#include "inkscape.h"
#include "desktop.h"
#include "path-prefix.h"
#include "ui/dialog/filedialog.h"

using namespace Inkscape;
using namespace Inkscape::IO::Resource;

static bool try_shortcuts_file(char const *filename, bool const is_user_set=false);
static void read_shortcuts_file(char const *filename, bool const is_user_set=false);

unsigned int sp_shortcut_get_key(unsigned int const shortcut);
GdkModifierType sp_shortcut_get_modifiers(unsigned int const shortcut);

/* Returns true if action was performed */

bool
sp_shortcut_invoke(unsigned int shortcut, Inkscape::UI::View::View *view)
{
    Inkscape::Verb *verb = sp_shortcut_get_verb(shortcut);
    if (verb) {
        SPAction *action = verb->get_action(Inkscape::ActionContext(view));
        if (action) {
            sp_action_perform(action, nullptr);
            return true;
        }
    }
    return false;
}

static std::map<unsigned int, Inkscape::Verb * > *verbs = nullptr;
static std::map<Inkscape::Verb *, unsigned int> *primary_shortcuts = nullptr;
static std::map<Inkscape::Verb *, unsigned int> *user_shortcuts = nullptr;

void sp_shortcut_init()
{
    verbs = new std::map<unsigned int, Inkscape::Verb * >();
    primary_shortcuts = new std::map<Inkscape::Verb *, unsigned int>();
    user_shortcuts = new std::map<Inkscape::Verb *, unsigned int>();

    // try to load shortcut file as set in preferences
    // if preference is unset or loading fails fallback to share/keys/default.xml and finally share/keys/inkscape.xml
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring shortcutfile = prefs->getString("/options/kbshortcuts/shortcutfile");
    bool success = false;
    gchar const *reason;
    if (shortcutfile.empty()) {
        reason = "No key file set in preferences";
    } else {
        success = try_shortcuts_file(shortcutfile.c_str());
        reason = "Unable to read key file set in preferences";
    }
#ifdef WITH_CARBON_INTEGRATION
    if (!success) {
        g_info("%s. Falling back to 'carbon.xml' for MacOSX keyboards.", reason);
        success = try_shortcuts_file(get_path(SYSTEM, KEYS, "carbon.xml"));
    }
#endif
    if (!success) {
        g_info("%s. Falling back to 'default.xml'.", reason);
        success = try_shortcuts_file(get_path(SYSTEM, KEYS, "default.xml"));
    }
    if (!success) {
        g_info("Could not load 'default.xml' either. Falling back to 'inkscape.xml'.");
        success = try_shortcuts_file(get_path(SYSTEM, KEYS, "inkscape.xml"));
    }
    if (!success) {
        g_warning("Could not load any keyboard shortcut file (including fallbacks to 'default.xml' and 'inkscape.xml').");
    }

    // load shortcuts adjusted by user
    try_shortcuts_file(get_path(USER, KEYS, "default.xml"), true);
}

static bool try_shortcuts_file(char const *filename, bool const is_user_set) {
    using Inkscape::IO::file_test;

    /* ah, if only we had an exception to catch... (permission, forgiveness) */
    if (file_test(filename, G_FILE_TEST_EXISTS)) {
        read_shortcuts_file(filename, is_user_set);
        return true;
    }

    g_info("Unable to read keyboard shortcuts from %s (does not exist)", filename);
    return false;
}

/*
 * Return the keyval corresponding to the key event in group 0 and the effective modifiers.
 *
 * Usage of group 0 (i.e. the main, typically English layout) instead of simply event->keyval
 * ensures that shortcuts work regardless of the active keyboard layouts (e.g. Cyrillic).
 *
 * The effective modifiers are the modifers that were not "consumed" by the translation and
 * can be used by the application to define a shortcut, e.g.
 *  - when pressing "Shift+9" the resulting character is "("
 *    the shift key was "consumed" to make this character and should not be part of the shortcut
 *  - when pressing "Ctrl+9" the resulting character is also "9"
 *    the ctrl key was *not* consumed to make this character and must be included in the shortcut
 *  - Exception: letter keys like [A-Z] always need the shift modifier,
 *               otherwise lower case and uper case keys are treated as equivalent
 * The modifier values are already transformed from the default GDK_*_MASK into the equivalent high-bit masks
 * defined by SP_SHORTCUT_*_MASK to allow for subsequent packing of the whole shortcut into a single int
 *
 * Note: Don't call this function directly but use the wrappers
 *        - sp_shortcut_get_from_event() - create a new shortcut from a key event
 *        - sp_shortcut_get_for_event()  - get an existing shortcut for a key event
 *       (they correctly handle the packing of modifier keys into the keyval)
 */
guint sp_shortcut_translate_event(GdkEventKey const *event, guint *effective_modifiers) {
    guint keyval = 0;

    guint initial_modifiers = event->state;
    guint consumed_modifiers = 0;
    guint remaining_modifiers = 0;
    guint resulting_modifiers = 0;  // remaining modifiers encoded in high-bit mask

    keyval = Inkscape::UI::Tools::get_latin_keyval(event, &consumed_modifiers);

    remaining_modifiers = initial_modifiers & ~consumed_modifiers;
    resulting_modifiers = ( remaining_modifiers & GDK_SHIFT_MASK ? SP_SHORTCUT_SHIFT_MASK : 0 ) |
	                      ( remaining_modifiers & GDK_CONTROL_MASK ? SP_SHORTCUT_CONTROL_MASK : 0 ) |
	                      ( remaining_modifiers & GDK_SUPER_MASK ? SP_SHORTCUT_SUPER_MASK : 0 ) |
	                      ( remaining_modifiers & GDK_HYPER_MASK ? SP_SHORTCUT_HYPER_MASK : 0 ) |
	                      ( remaining_modifiers & GDK_META_MASK ? SP_SHORTCUT_META_MASK : 0 ) |
	                      ( remaining_modifiers & GDK_MOD1_MASK ? SP_SHORTCUT_ALT_MASK : 0 );              

    // enforce the Shift modifier for uppercase letters (otherwise plain A and Shift+A are equivalent)
    // for characters that are not letters both (is_upper and is_lower) return TRUE, so the condition is false
    if (gdk_keyval_is_upper(keyval) && !gdk_keyval_is_lower(keyval)) {
        resulting_modifiers |= SP_SHORTCUT_SHIFT_MASK;
    }

    *effective_modifiers = resulting_modifiers;
    return keyval;
}

/*
 * Returns a new Inkscape shortcut parsed from a key event.
 */
unsigned int sp_shortcut_get_from_event(GdkEventKey const *event) {
    guint effective_modifiers;

    sp_shortcut_translate_event(event, &effective_modifiers);

    // return the actual keyval and the corresponding modifiers for creating the shortcut
    // we must not return the translated keyval, otherwise we end up with illegal shortcuts like "Shift+9" instead of "("
    return (event->keyval) | effective_modifiers;
}

/*
 * Returns a new Inkscape shortcut parsed from a key event.
 * (equivalent to sp_shortcut_get_from_event() but accepts the arguments of Gtk::CellRendererAccel::signal_accel_edited)
 */
unsigned int sp_shortcut_get_from_gdk_event(guint accel_key, Gdk::ModifierType accel_mods, guint hardware_keycode) {
    GdkEventKey event;
    event.keyval = accel_key;
    event.state = accel_mods;
    event.hardware_keycode = hardware_keycode;

    return sp_shortcut_get_from_event(&event);
}

/*
 * Returns the Inkscape-internal integral shortcut representation for a key event.
 * Use this to compare the received key event to known shortcuts.
 */
unsigned int sp_shortcut_get_for_event(GdkEventKey const *event) {
    guint keyval;
    guint effective_modifiers;

    keyval = sp_shortcut_translate_event(event, &effective_modifiers);

    // return the keyval translated to group 0 (English keyboard layout) and corresponding modifiers
    return keyval | effective_modifiers;
}

Glib::ustring sp_shortcut_to_label(unsigned int const shortcut) {

    Glib::ustring modifiers = "";

    if (shortcut & SP_SHORTCUT_CONTROL_MASK)
        modifiers += "Ctrl,";
    if (shortcut & SP_SHORTCUT_SHIFT_MASK)
        modifiers += "Shift,";
    if (shortcut & SP_SHORTCUT_ALT_MASK)
        modifiers += "Alt,";
    if (shortcut & SP_SHORTCUT_SUPER_MASK)
        modifiers += "Super,";
    if (shortcut & SP_SHORTCUT_HYPER_MASK)
        modifiers += "Hyper,";
    if (shortcut & SP_SHORTCUT_META_MASK)
        modifiers += "Meta,";

    if(modifiers.length() > 0 &&
            modifiers.find(',',modifiers.length()-1)!=modifiers.npos) {
        modifiers.erase(modifiers.length()-1, 1);
    }

    return modifiers;
}

/*
 * REmove all shortucts from the users file
 */

void sp_shortcuts_delete_all_from_file() {


    char const *filename = get_path(USER, KEYS, "default.xml");

    XML::Document *doc=sp_repr_read_file(filename, nullptr);
    if (!doc) {
        g_warning("Unable to read keys file %s", filename);
        return;
    }

    XML::Node *root=doc->root();
    g_return_if_fail(!strcmp(root->name(), "keys"));

    XML::Node *iter=root->firstChild();
    while (iter) {

        if (strcmp(iter->name(), "bind")) {
            // some unknown element, do not complain
            iter = iter->next();
            continue;
        }

        // Delete node
        sp_repr_unparent(iter);
        iter=root->firstChild();
    }


    sp_repr_save_file(doc, filename, nullptr);

    GC::release(doc);
}

Inkscape::XML::Document *sp_shortcut_create_template_file(char const *filename) {

    gchar const *buffer =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?> "
            "<keys name=\"My custom shortcuts\">"
            "</keys>";

    Inkscape::XML::Document *doc = sp_repr_read_mem(buffer, strlen(buffer), nullptr);
    sp_repr_save_file(doc, filename, nullptr);

    return sp_repr_read_file(filename, nullptr);
}

/*
 * Get a list of keyboard shortcut files names and paths from the system and users paths
 * Don't add the users custom keyboards file
 */
void sp_shortcut_get_file_names(std::vector<Glib::ustring> *names, std::vector<Glib::ustring> *paths) {
    using namespace Inkscape::IO::Resource;

    std::vector<Glib::ustring> filenames = get_filenames(SYSTEM, KEYS, {".xml"});
    std::vector<Glib::ustring> filenames_user = get_filenames(USER, KEYS, {".xml"}, {"default.xml"}); // exclude default.xml as it only includes the user's modifications
    filenames.insert(filenames.end(), filenames_user.begin(), filenames_user.end());

    std::vector<std::pair<Glib::ustring, Glib::ustring>> names_and_paths;
    for(auto &filename: filenames) {
        Glib::ustring label = Glib::path_get_basename(filename);

        XML::Document *doc = sp_repr_read_file(filename.c_str(), nullptr);
        if (!doc) {
            g_warning("Unable to read keyboard shortcut file %s", filename.c_str());
            continue;
        }

        // Get the "key name" from the root element of each file
        XML::Node *root=doc->root();
        if (!strcmp(root->name(), "keys")) {
            gchar const *name=root->attribute("name");
            if (name) {
                label = Glib::ustring(name) + " (" + label + ")";
            }
            std::pair<Glib::ustring, Glib::ustring> name_and_path;
            name_and_path = std::make_pair(label, filename);
            names_and_paths.push_back(name_and_path);
        } else {
            g_warning("Not a shortcut keys file %s", filename.c_str());
        }
        Inkscape::GC::release(doc);
    }
    
    // sort by name 
    std::sort(names_and_paths.begin(), names_and_paths.end(),
            [](std::pair<Glib::ustring, Glib::ustring> pair1, std::pair<Glib::ustring, Glib::ustring> pair2) {
                return Glib::path_get_basename(pair1.first).compare(Glib::path_get_basename(pair2.first)) < 0;
            });
    auto it_default = std::find_if(names_and_paths.begin(), names_and_paths.end(),
            [](std::pair<Glib::ustring, Glib::ustring>& pair) {
                return !Glib::path_get_basename(pair.second).compare("default.xml");
            });
    if (it_default != names_and_paths.end()) {
        std::rotate(names_and_paths.begin(), it_default, it_default+1);
    }

    // transform pairs to output vectors
    std::transform(names_and_paths.begin(),names_and_paths.end(), std::back_inserter(*names), 
            [](const std::pair<Glib::ustring, Glib::ustring>& pair) { return pair.first; });
    std::transform(names_and_paths.begin(),names_and_paths.end(), std::back_inserter(*paths), 
            [](const std::pair<Glib::ustring, Glib::ustring>& pair) { return pair.second; });
}

Glib::ustring sp_shortcut_get_file_path()
{
    //# Get the current directory for finding files
    Glib::ustring open_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Glib::ustring attr = prefs->getString("/dialogs/save_export/path");
    if (!attr.empty()) open_path = attr;

    //# Test if the open_path directory exists
    if (!Inkscape::IO::file_test(open_path.c_str(),
              (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        open_path = "";

    if (open_path.empty()) {
        /* Grab document directory */
        const gchar* docURI = SP_ACTIVE_DOCUMENT->getDocumentURI();
        if (docURI) {
            open_path = Glib::path_get_dirname(docURI);
            open_path.append(G_DIR_SEPARATOR_S);
        }
    }

    //# If no open path, default to our home directory
    if (open_path.empty())
    {
        open_path = g_get_home_dir();
        open_path.append(G_DIR_SEPARATOR_S);
    }

    return open_path;
}

//static Inkscape::UI::Dialog::FileSaveDialog * saveDialog = NULL;

void sp_shortcut_file_export()
{
    Glib::ustring open_path = sp_shortcut_get_file_path();
    open_path.append("shortcut_keys.xml");

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Glib::ustring filename;

    Inkscape::UI::Dialog::FileSaveDialog *saveDialog =
       Inkscape::UI::Dialog::FileSaveDialog::create(
            *(desktop->getToplevel()),
            open_path,
            Inkscape::UI::Dialog::CUSTOM_TYPE,
            _("Select a filename for exporting"),
            "",
            "",
            Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS
        );
    saveDialog->addFileType(_("Inkscape shortcuts (*.xml)"), ".xml");
    

    bool success = saveDialog->show();
    if (!success) {
        delete saveDialog;
        return;
    }

    Glib::ustring fileName = saveDialog->getFilename();
    if (fileName.size() > 0) {
        Glib::ustring newFileName = Glib::filename_to_utf8(fileName);
        sp_shortcut_file_export_do(newFileName.c_str());
    }

    delete saveDialog;
}

bool sp_shortcut_file_import() {

    Glib::ustring open_path = sp_shortcut_get_file_path();

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    Inkscape::UI::Dialog::FileOpenDialog *importFileDialog =
              Inkscape::UI::Dialog::FileOpenDialog::create(
                 *desktop->getToplevel(),
                 open_path,
                 Inkscape::UI::Dialog::CUSTOM_TYPE,
                 _("Select a file to import"));
        importFileDialog->addFilterMenu("All Files", "*");

    //# Show the dialog
    bool const success = importFileDialog->show();

    if (!success) {
        delete importFileDialog;
        return false;
    }

    Glib::ustring fileName = importFileDialog->getFilename();
    sp_shortcut_file_import_do(fileName.c_str());

    delete importFileDialog;

    return true;
}

void sp_shortcut_file_import_do(char const *importname) {

    XML::Document *doc=sp_repr_read_file(importname, nullptr);
    if (!doc) {
        g_warning("Unable to read keyboard shortcut file %s", importname);
        return;
    }

    char const *filename = get_path(USER, KEYS, "default.xml");
    sp_repr_save_file(doc, filename, nullptr);

    GC::release(doc);

    sp_shortcut_init();
}

void sp_shortcut_file_export_do(char const *exportname) {

    char const *filename = get_path(USER, KEYS, "default.xml");

    XML::Document *doc=sp_repr_read_file(filename, nullptr);
    if (!doc) {
        g_warning("Unable to read keyboard shortcut file %s", filename);
        return;
    }

    sp_repr_save_file(doc, exportname, nullptr);

    GC::release(doc);
}
/*
 * Add or delete a shortcut to the users default.xml keys file
 * @param addremove - when true add/override a shortcut, when false remove shortcut
 * @param addshift - when true addthe Shifg modifier to the non-display element
 *
 * Shortcut file consists of pairs of bind elements :
 * Element (a) is used for shortcut display in menus (display="True") and contains the gdk_keyval_name of the shortcut key
 * Element (b) is used in shortcut lookup and contains an uppercase version of the gdk_keyval_name,
 *   or a gdk_keyval_name name and the "Shift" modifier for Shift altered hardware code keys (see get_latin_keyval() for explanation)
 */
void sp_shortcut_delete_from_file(char const * /*action*/, unsigned int const shortcut) {

    char const *filename = get_path(USER, KEYS, "default.xml");

    XML::Document *doc=sp_repr_read_file(filename, nullptr);
    if (!doc) {
        g_warning("Unable to read keyboard shortcut file %s", filename);
        return;
    }

    gchar *key = gdk_keyval_name (sp_shortcut_get_key(shortcut));
    std::string modifiers = sp_shortcut_to_label(shortcut & (SP_SHORTCUT_MODIFIER_MASK));

    if (!key) {
        g_warning("Unknown key for shortcut %u", shortcut);
        return;
    }

    //g_message("Removing key %s, mods %s action %s", key, modifiers.c_str(), action);

    XML::Node *root=doc->root();
    g_return_if_fail(!strcmp(root->name(), "keys"));
    XML::Node *iter=root->firstChild();
    while (iter) {

        if (strcmp(iter->name(), "bind")) {
            // some unknown element, do not complain
            iter = iter->next();
            continue;
        }

        gchar const *verb_name=iter->attribute("action");
        if (!verb_name) {
            iter = iter->next();
            continue;
        }

        gchar const *keyval_name = iter->attribute("key");
        if (!keyval_name || !*keyval_name) {
            // that's ok, it's just listed for reference without assignment, skip it
            iter = iter->next();
            continue;
        }

        if (Glib::ustring(key).lowercase() != Glib::ustring(keyval_name).lowercase()) {
            // If deleting, then delete both the upper and lower case versions
            iter = iter->next();
            continue;
        }

        gchar const *modifiers_string = iter->attribute("modifiers");
        if ((modifiers_string && !strcmp(modifiers.c_str(), modifiers_string)) ||
                (!modifiers_string && modifiers.empty())) {
            //Looks like a match
            // Delete node
            sp_repr_unparent(iter);
            iter = root->firstChild();
            continue;
        }
        iter = iter->next();
    }

    sp_repr_save_file(doc, filename, nullptr);

    GC::release(doc);

}

void sp_shortcut_add_to_file(char const *action, unsigned int const shortcut) {

    char const *filename = get_path(USER, KEYS, "default.xml");

    XML::Document *doc=sp_repr_read_file(filename, nullptr);
    if (!doc) {
        g_warning("Unable to read keyboard shortcut file %s, creating ....", filename);
        doc = sp_shortcut_create_template_file(filename);
        if (!doc) {
            g_warning("Unable to create keyboard shortcut file %s", filename);
            return;
        }
    }

    gchar *key = gdk_keyval_name (sp_shortcut_get_key(shortcut));
    std::string modifiers = sp_shortcut_to_label(shortcut & (SP_SHORTCUT_MODIFIER_MASK));

    if (!key) {
        g_warning("Unknown key for shortcut %u", shortcut);
        return;
    }

    //g_message("Adding key %s, mods %s action %s", key, modifiers.c_str(), action);

    // Add node
    Inkscape::XML::Node *newnode;
    newnode = doc->createElement("bind");
    newnode->setAttribute("key", key);
    if (!modifiers.empty()) {
        newnode->setAttribute("modifiers", modifiers.c_str());
    }
    newnode->setAttribute("action", action);
    newnode->setAttribute("display", "true");

    doc->root()->appendChild(newnode);

    if (strlen(key) == 1) {
        // Add another uppercase version if a character
        Inkscape::XML::Node *newnode;
        newnode = doc->createElement("bind");
        newnode->setAttribute("key", Glib::ustring(key).uppercase().c_str());
        if (!modifiers.empty()) {
            newnode->setAttribute("modifiers", modifiers.c_str());
        }

        newnode->setAttribute("action", action);
        doc->root()->appendChild(newnode);
    }

    sp_repr_save_file(doc, filename, nullptr);

    GC::release(doc);

}
static void read_shortcuts_file(char const *filename, bool const is_user_set) {
    XML::Document *doc=sp_repr_read_file(filename, nullptr);
    if (!doc) {
        g_warning("Unable to read keys file %s", filename);
        return;
    }

    XML::Node const *root=doc->root();
    g_return_if_fail(!strcmp(root->name(), "keys"));
    XML::NodeConstSiblingIterator iter=root->firstChild();
    for ( ; iter ; ++iter ) {
        bool is_primary;

        if (!strcmp(iter->name(), "bind")) {
            is_primary = iter->attribute("display") && strcmp(iter->attribute("display"), "false") && strcmp(iter->attribute("display"), "0");
        } else {
            // some unknown element, do not complain
            continue;
        }

        gchar const *verb_name=iter->attribute("action");
        if (!verb_name) {
            g_warning("Missing verb name (action= attribute) for shortcut");
            continue;
        }

        Inkscape::Verb *verb=Inkscape::Verb::getbyid(verb_name);
        if (!verb
#if !HAVE_POTRACE
                // Squash warning about disabled features
                && strcmp(verb_name, "ToolPaintBucket")  != 0
                && strcmp(verb_name, "SelectionTrace")   != 0
                && strcmp(verb_name, "PaintBucketPrefs") != 0
#endif
           ) {
            g_warning("Unknown verb name: %s", verb_name);
            continue;
        }

        gchar const *keyval_name=iter->attribute("key");
        if (!keyval_name || !*keyval_name) {
            // that's ok, it's just listed for reference without assignment, skip it
            continue;
        }

        guint keyval=gdk_keyval_from_name(keyval_name);
        if (keyval == GDK_KEY_VoidSymbol || keyval == 0) {
            g_warning("Unknown keyval %s for %s", keyval_name, verb_name);
            continue;
        }

        guint modifiers=0;

        gchar const *modifiers_string=iter->attribute("modifiers");
        if (modifiers_string) {
            gchar const *iter=modifiers_string;
            while (*iter) {
                size_t length=strcspn(iter, ",");
                gchar *mod=g_strndup(iter, length);
                if (!strcmp(mod, "Control") || !strcmp(mod, "Ctrl")) {
                    modifiers |= SP_SHORTCUT_CONTROL_MASK;
                } else if (!strcmp(mod, "Shift")) {
                    modifiers |= SP_SHORTCUT_SHIFT_MASK;
                } else if (!strcmp(mod, "Alt")) {
                    modifiers |= SP_SHORTCUT_ALT_MASK;
                } else if (!strcmp(mod, "Super")) {
                    modifiers |= SP_SHORTCUT_SUPER_MASK;
                } else if (!strcmp(mod, "Hyper") || !strcmp(mod, "Cmd")) {
                    modifiers |= SP_SHORTCUT_HYPER_MASK;
                } else if (!strcmp(mod, "Meta")) {
                    modifiers |= SP_SHORTCUT_META_MASK;
                } else if (!strcmp(mod, "Primary")) {
                    GdkKeymap* keymap = Gdk::Display::get_default()->get_keymap();
                    GdkModifierType mod =
                        gdk_keymap_get_modifier_mask (keymap, GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
                    gdk_keymap_add_virtual_modifiers(keymap, &mod);
                    if (mod & GDK_CONTROL_MASK)
                        modifiers |= SP_SHORTCUT_CONTROL_MASK;
                    else if (mod & GDK_META_MASK)
                        modifiers |= SP_SHORTCUT_META_MASK;
                     else {
                         g_warning("unsupported primary accelerator ");
                         modifiers |= SP_SHORTCUT_CONTROL_MASK;
                    }
                } else {
                    g_warning("Unknown modifier %s for %s", mod, verb_name);
                }
                g_free(mod);
                iter += length;
                if (*iter) iter++;
            }
        }

        sp_shortcut_set(keyval | modifiers, verb, is_primary, is_user_set);
    }

    GC::release(doc);
}

/**
 * Removes a keyboard shortcut for the given verb.
 * (Removes any existing binding for the given shortcut, including appropriately
 * adjusting sp_shortcut_get_primary if necessary.)*
 */
void
sp_shortcut_unset(unsigned int const shortcut)
{
    if (!verbs) sp_shortcut_init();

    Inkscape::Verb *verb = (*verbs)[shortcut];

    /* Maintain the invariant that sp_shortcut_get_primary(v) returns either 0 or a valid shortcut for v. */
    if (verb) {

        (*verbs)[shortcut] = nullptr;

        unsigned int const old_primary = (*primary_shortcuts)[verb];
        if (old_primary == shortcut) {
            (*primary_shortcuts)[verb] = 0;
        }

    }
}

GtkAccelGroup *
sp_shortcut_get_accel_group()
{
    static GtkAccelGroup *accel_group = nullptr;

    if (!accel_group) {
        accel_group = gtk_accel_group_new ();
    }

    return accel_group;
}

/**
 * Adds a gtk accelerator to a widget
 * Used to display the keyboard shortcuts in the main menu items
 */
void
sp_shortcut_add_accelerator(GtkWidget *item, unsigned int const shortcut)
{
    if (shortcut == GDK_KEY_VoidSymbol) {
        return;
    }

    unsigned int accel_key = sp_shortcut_get_key(shortcut);
    if (accel_key > 0) {
        gtk_widget_add_accelerator (item,
                "activate",
                sp_shortcut_get_accel_group(),
                accel_key,
                sp_shortcut_get_modifiers(shortcut),
                GTK_ACCEL_VISIBLE);
    }
}


unsigned int
sp_shortcut_get_key(unsigned int const shortcut)
{
    return (shortcut & (~SP_SHORTCUT_MODIFIER_MASK));
}

GdkModifierType
sp_shortcut_get_modifiers(unsigned int const shortcut)
{
    return static_cast<GdkModifierType>(
            ((shortcut & SP_SHORTCUT_SHIFT_MASK) ? GDK_SHIFT_MASK : 0) |
            ((shortcut & SP_SHORTCUT_CONTROL_MASK) ? GDK_CONTROL_MASK : 0) |
            ((shortcut & SP_SHORTCUT_SUPER_MASK) ? GDK_SUPER_MASK : 0) |
            ((shortcut & SP_SHORTCUT_HYPER_MASK) ? GDK_HYPER_MASK : 0) |
            ((shortcut & SP_SHORTCUT_META_MASK) ? GDK_META_MASK : 0) |
            ((shortcut & SP_SHORTCUT_ALT_MASK) ? GDK_MOD1_MASK : 0)
            );
}

/**
 * Adds a keyboard shortcut for the given verb.
 * (Removes any existing binding for the given shortcut, including appropriately
 * adjusting sp_shortcut_get_primary if necessary.)
 *
 * \param is_primary True iff this is the shortcut to be written in menu items or buttons.
 *
 * \post sp_shortcut_get_verb(shortcut) == verb.
 * \post !is_primary or sp_shortcut_get_primary(verb) == shortcut.
 */
void
sp_shortcut_set(unsigned int const shortcut, Inkscape::Verb *const verb, bool const is_primary, bool const is_user_set)
{
    if (!verbs) sp_shortcut_init();

    Inkscape::Verb *old_verb = (*verbs)[shortcut];
    (*verbs)[shortcut] = verb;

    /* Maintain the invariant that sp_shortcut_get_primary(v) returns either 0 or a valid shortcut for v. */
    if (old_verb && old_verb != verb) {
        unsigned int const old_primary = (*primary_shortcuts)[old_verb];

        if (old_primary == shortcut) {
            (*primary_shortcuts)[old_verb] = 0;
            (*user_shortcuts)[old_verb] = 0;
        }
    }

    if (is_primary) {
        (*primary_shortcuts)[verb] = shortcut;
        (*user_shortcuts)[verb] = is_user_set;
    }
}

Inkscape::Verb *
sp_shortcut_get_verb(unsigned int shortcut)
{
    if (!verbs) sp_shortcut_init();
    return (*verbs)[shortcut];
}

unsigned int sp_shortcut_get_primary(Inkscape::Verb *verb)
{
    unsigned int result = GDK_KEY_VoidSymbol;
    if (!primary_shortcuts) {
        sp_shortcut_init();
    }
    
    if (primary_shortcuts->count(verb)) {
        result = (*primary_shortcuts)[verb];
    }
    return result;
}

bool sp_shortcut_is_user_set(Inkscape::Verb *verb)
{
    unsigned int result = false;
    if (!primary_shortcuts) {
        sp_shortcut_init();
    }

    if (primary_shortcuts->count(verb)) {
        result = (*user_shortcuts)[verb];
    }
    return result;
}

gchar *sp_shortcut_get_label(unsigned int shortcut)
{
    // The comment below was copied from the function sp_ui_shortcut_string in interface.cpp (which was subsequently removed)
    /* TODO: This function shouldn't exist.  Our callers should use GtkAccelLabel instead of
     * a generic GtkLabel containing this string, and should call gtk_widget_add_accelerator.
     * Will probably need to change sp_shortcut_invoke callers.
     *
     * The existing gtk_label_new_with_mnemonic call can be replaced with
     * g_object_new(GTK_TYPE_ACCEL_LABEL, NULL) followed by
     * gtk_label_set_text_with_mnemonic(lbl, str).
     */
    gchar *result = nullptr;
    if (shortcut != GDK_KEY_VoidSymbol) {
        result = gtk_accelerator_get_label(
                sp_shortcut_get_key(shortcut),
                sp_shortcut_get_modifiers(shortcut));
    }
    return result;
}

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
