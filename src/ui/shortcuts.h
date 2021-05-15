// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Shortcuts
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_SHORTCUTS_H
#define INK_SHORTCUTS_H

#include <map>
#include <set>

#include <giomm.h>
#include <gtkmm.h>

namespace Inkscape {
class Verb;

namespace UI {
namespace View {
class View;
}
}

namespace XML {
class Document;
class Node;
}

struct accel_key_less
{
    bool operator()(const Gtk::AccelKey& key1, const Gtk::AccelKey& key2) const
    {
        if(key1.get_key() < key2.get_key()) return true;
        if(key1.get_key() > key2.get_key()) return false;
        return (key1.get_mod() < key2.get_mod());
    }
};

class Shortcuts {

public:

    enum What {
        All,
        System,
        User
    };
        
    static Shortcuts& getInstance()
    {
        static Shortcuts instance;

        // Uncomment after verbs are gone. Until then, initialization explicitly called in InkscapeApplication::on_startup2().
        // if (!instance.initialized) {
        //     instance.init();
        // }

        return instance;
    }
  
private:
    Shortcuts();
    ~Shortcuts() = default;

public:
    Shortcuts(Shortcuts const&)      = delete;
    void operator=(Shortcuts const&) = delete;

    void init();
    void clear();

    bool read( Glib::RefPtr<Gio::File> file, bool user_set = false);
    bool write(Glib::RefPtr<Gio::File> file, What what = User);
    bool write_user();

    // Verb stuff, to be removed.
    Gtk::AccelKey get_shortcut_from_verb(Verb* verb);
    Verb* get_verb_from_shortcut(const Gtk::AccelKey& shortcut);
    bool invoke_verb(GdkEventKey const *event, UI::View::View *view);

    bool is_user_set(Gtk::AccelKey verb_shortcut);
    bool is_user_set(Glib::ustring& action);

    // Add/remove shortcuts
    bool add_shortcut(Glib::ustring name, const Gtk::AccelKey& shortcut, bool user, bool is_primary = false);
    bool remove_shortcut(Glib::ustring name);
    Glib::ustring remove_shortcut(const Gtk::AccelKey& shortcut);

    // User shortcuts
    bool add_user_shortcut(Glib::ustring name, const Gtk::AccelKey& shortcut);
    bool remove_user_shortcut(Glib::ustring name);
    bool clear_user_shortcuts();

    // Utility
    static Glib::ustring get_label(const Gtk::AccelKey& shortcut);
    static Glib::ustring get_modifiers_verb(unsigned int mod_val);
    static Gtk::AccelKey get_from_event(GdkEventKey const *event, bool fix = false);
    std::vector<Glib::ustring> list_all_detailed_action_names();
    std::vector<Glib::ustring> list_all_actions();

    // Will disappear after verbs are gone. (Use Gtk::AccelGroup functions instead for actions.)
    Glib::ustring shortcut_to_accelerator(const Gtk::AccelKey& shortcut);
    Gtk::AccelKey accelerator_to_shortcut(const Glib::ustring& accelerator);

    // Will disappear after verbs are gone. 
    void add_accelerator(Gtk::Widget *widget, Verb* verb);

    static std::vector<std::pair<Glib::ustring, Glib::ustring>> get_file_names();

    void update_gui_text_recursive(Gtk::Widget* widget);

    // Dialogs
    bool import_shortcuts();
    bool export_shortcuts();

    // Debug
    void dump();

    void dump_all_recursive(Gtk::Widget* widget);

private:

    // Gio::Actions
    Glib::RefPtr<Gtk::Application> app;
    std::map<Glib::ustring, bool> action_user_set;

    // Legacy verbs
    std::map<Gtk::AccelKey, Inkscape::Verb*, accel_key_less> shortcut_to_verb_map;
    std::map<Inkscape::Verb *, Gtk::AccelKey> primary;  // Shown in menus, etc.
    std::set<Gtk::AccelKey, accel_key_less> user_set;

    void _read(XML::Node const &keysnode, bool user_set);

    bool initialized = false;
};

} // Namespace Inkscape

#endif // INK_SHORTCUTS_H

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
