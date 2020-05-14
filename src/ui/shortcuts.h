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

    // Will disappear after verbs are gone.
    void set_verb_shortcut(unsigned long long int const &val, Inkscape::Verb *const verb, bool is_primary, bool is_user_set);
    unsigned long long int get_shortcut_from_verb(Verb* verb);
    Verb* get_verb_from_shortcut(unsigned long long int shortcut);
    bool invoke_verb(GdkEventKey const *event, UI::View::View *view);

    bool is_user_set(Verb* verb);
    bool is_user_set(Glib::ustring& action);

    // User shortcuts
    bool add_user_shortcut(Glib::ustring name, unsigned long long int shortcut);
    bool remove_user_shortcut(Glib::ustring name, unsigned long long int shortcut);
    bool clear_user_shortcuts();

    // Utility
    static Glib::ustring get_label(unsigned long long int shortcut);
    static Glib::ustring get_modifiers_verb(unsigned int mod_val);
    static unsigned long long int get_from_event(GdkEventKey const *event);
    std::vector<Glib::ustring> list_all_actions();

    // Will disappear after verbs are gone. (Use Gtk::AccelGroup functions instead for actions.)
    Glib::ustring shortcut_to_accelerator(unsigned long long int shortcut);
    unsigned long long int accelerator_to_shortcut(Glib::ustring& accelerator);

    // Will disappear after verbs are gone. 
    void add_accelerator(Gtk::Widget *widget, Verb* verb);

    static std::vector<std::pair<Glib::ustring, Glib::ustring>> get_file_names();

    // Dialogs
    bool import_shortcuts();
    bool export_shortcuts();

    // Debug
    void dump();

private:

    // Gio::Actions
    Glib::RefPtr<Gtk::Application> app;
    std::map<Glib::ustring, bool> action_user_set;

    //Legacy verbs
    std::map<unsigned long long int, Inkscape::Verb*> shortcut_to_verb_map;
    std::map<Inkscape::Verb *, unsigned long long int> primary;  // Shown in menus, etc.
    std::map<Inkscape::Verb *, bool> user_set;
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
