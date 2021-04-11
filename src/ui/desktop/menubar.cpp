// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Desktop main menu bar code.
 */
/*
 * Authors:
 *   Tavmjong Bah       <tavmjong@free.fr>
 *   Alex Valavanis     <valavanisalex@gmail.com>
 *   Patrick Storz      <eduard.braun2@gmx.de>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Kris De Gussem     <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <iostream>

#include "inkscape.h"
#include "inkscape-application.h" // Open recent

#include "message-context.h"

#include "menu-icon-shift.h"

#include "helper/action.h"
#include "helper/action-context.h"

#include "io/resource.h"    // UI File location

#include "object/sp-namedview.h"

#include "ui/icon-loader.h"
#include "ui/shortcuts.h"
#include "ui/view/view.h"
#include "ui/uxmanager.h"   // To Do: Convert to actions

#ifdef GDK_WINDOWING_QUARTZ
#include <gtkosxapplication.h>
#endif

// ================= Common ====================

std::vector<std::pair<std::pair<unsigned int, Gtk::MenuItem *>, Inkscape::UI::View::View *>> menuitems;
unsigned int lastverb = -1;


/**
 * Get menu item (if it was registered in `menuitems`)
 */
Gtk::MenuItem *get_menu_item_for_verb(unsigned int verb, Inkscape::UI::View::View *view)
{
    for (auto &item : menuitems) {
        if (item.first.first == verb && item.second == view) {
            return item.first.second;
        }
    }
    return nullptr;
}

#ifdef GDK_WINDOWING_QUARTZ
/**
 * Update menubar for macOS
 *
 * Can be used as a glib timeout callback.
 */
static gboolean sync_menubar(gpointer = nullptr)
{
    auto osxapp = gtkosx_application_get();
    if (osxapp) {
        gtkosx_application_sync_menubar(osxapp);
    }
    return false;
}
#endif

// Sets tip
static void
select_action(SPAction *action)
{
    sp_action_get_view(action)->tipsMessageContext()->set(Inkscape::NORMAL_MESSAGE, action->tip);
}

// Clears tip
static void
deselect_action(SPAction *action)
{
    sp_action_get_view(action)->tipsMessageContext()->clear();
}

// Trigger action
static void item_activate(Gtk::MenuItem *menuitem, SPAction *action)
{
    if (action->verb->get_code() == lastverb) {
        lastverb = -1;
        return;
    }
    lastverb = action->verb->get_code();
    sp_action_perform(action, nullptr);
    lastverb = -1;

#ifdef GDK_WINDOWING_QUARTZ
    sync_menubar();
#endif
}

static void set_menuitems(unsigned int emitting_verb, bool value)
{
    for (auto menu : menuitems) {
        if (menu.second == SP_ACTIVE_DESKTOP) {
            if (emitting_verb == menu.first.first) {
                if (emitting_verb == lastverb) {
                    lastverb = -1;
                    return;
                }
                lastverb = emitting_verb;
                Gtk::CheckMenuItem *check = dynamic_cast<Gtk::CheckMenuItem *>(menu.first.second);
                Gtk::RadioMenuItem *radio = dynamic_cast<Gtk::RadioMenuItem *>(menu.first.second);
                if (radio) {
                    radio->property_active() = value;
                } else if (check) {
                    check->property_active() = value;
                }
                lastverb = -1;
            }
        }
    }
}

// Change label name (used in the Undo/Redo menu items).
static void
set_name(Glib::ustring const &name, Gtk::MenuItem* menuitem)
{
    if (menuitem) {
        Gtk::Widget* widget = menuitem->get_child();

        // Label is either child of menuitem
        Gtk::Label* label = dynamic_cast<Gtk::Label*>(widget);

        // Or wrapped inside a box which is a child of menuitem (if with icon).
        if (!label) {
            Gtk::Box* box = dynamic_cast<Gtk::Box*>(widget);
            if (box) {
                std::vector<Gtk::Widget*> children = box->get_children();
                for (auto child: children) {
                    label = dynamic_cast<Gtk::Label*>(child);
                    if (label) break;
                }
            }
        }

        if (label) {
            label->set_markup_with_mnemonic(name);
        } else {
            std::cerr << "set_name: could not find label!" << std::endl;
        }
    }
}

// ================= MenuItem ====================

Gtk::MenuItem*
build_menu_item_from_verb(SPAction* action,
                          bool show_icon,
                          bool radio = false,
                          Gtk::RadioMenuItem::Group *group = nullptr)
{
    Gtk::MenuItem* menuitem = nullptr;

    if (radio) {
        menuitem = Gtk::manage(new Gtk::RadioMenuItem(*group));
    } else {
        menuitem = Gtk::manage(new Gtk::MenuItem());
    }

    Gtk::AccelLabel* label = Gtk::manage(new Gtk::AccelLabel(action->name, true));
    label->set_xalign(0.0);
    label->set_accel_widget(*menuitem);
    Inkscape::Shortcuts::getInstance().add_accelerator(menuitem, action->verb);

    // If there is an image associated with the action, we can add it as an icon for the menu item.
    if (show_icon && action->image) {
        menuitem->set_name("ImageMenuItem");  // custom name to identify our "ImageMenuItems"
        Gtk::Image* image = Gtk::manage(sp_get_icon_image(action->image, Gtk::ICON_SIZE_MENU));

        // Create a box to hold icon and label as Gtk::MenuItem derives from GtkBin and can
        // only hold one child
        Gtk::Box *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*image, false, false, 0);
        box->pack_start(*label, true,  true,  0);
        menuitem->add(*box);
    } else {
        menuitem->add(*label);
    }
    menuitem->signal_activate().connect(
        sigc::bind<Gtk::MenuItem*, SPAction*>(sigc::ptr_fun(&item_activate), menuitem, action));
    menuitem->signal_select().connect(  sigc::bind<SPAction*>(sigc::ptr_fun(&select_action),   action));
    menuitem->signal_deselect().connect(sigc::bind<SPAction*>(sigc::ptr_fun(&deselect_action), action));

    action->signal_set_sensitive.connect(
        sigc::bind<0>(sigc::ptr_fun(&gtk_widget_set_sensitive), (GtkWidget*)menuitem->gobj()));
    action->signal_set_name.connect(
        sigc::bind<Gtk::MenuItem*>(sigc::ptr_fun(&set_name), menuitem));

    // initialize sensitivity with verb default
    sp_action_set_sensitive(action, action->verb->get_default_sensitive());

    return menuitem;
}

// =============== CheckMenuItem ==================

bool getStateFromPref(SPDesktop *dt, Glib::ustring item)
{
    Glib::ustring pref_path;

    if (dt->is_focusMode()) {
        pref_path = "/focus/";
    } else if (dt->is_fullscreen()) {
        pref_path = "/fullscreen/";
    } else {
        pref_path = "/window/";
    }

    pref_path += item;
    pref_path += "/state";

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    return prefs->getBool(pref_path, false);
}

// I wonder if this can be done without hard coding names.
static void
checkitem_update(Gtk::CheckMenuItem* menuitem, SPAction* action)
{
    bool active = false;
    if (action && action->id) {
        Glib::ustring id = action->id;
        SPDesktop* dt = static_cast<SPDesktop*>(sp_action_get_view(action));

        if (id == "ToggleGrid") {
            active = dt->gridsEnabled();

        } else if (id == "EditGuidesToggleLock") {
            active = dt->namedview->lockguides;

        } else if (id == "ToggleGuides") {
            active = dt->namedview->getGuides();

        } else if (id == "ToggleRotationLock") {
            active = dt->get_rotation_lock();

        } else if (id == "ViewCmsToggle") {
            active = dt->colorProfAdjustEnabled();

        } else if (id == "ToggleCommandsToolbar") {
            active = getStateFromPref(dt, "commands");

        } else if (id == "ToggleSnapToolbar") {
            active = getStateFromPref(dt, "snaptoolbox");

        } else if (id == "ToggleToolToolbar") {
            active = getStateFromPref(dt, "toppanel");

        } else if (id == "ToggleToolbox") {
            active = getStateFromPref(dt, "toolbox");

        } else if (id == "ToggleRulers") {
            active = getStateFromPref(dt, "rulers");

        } else if (id == "ToggleScrollbars") {
            active = getStateFromPref(dt, "scrollbars");

        } else if (id == "TogglePalette") {
            active = getStateFromPref(dt, "panels");  // Rename?

        } else if (id == "ToggleStatusbar") {
            active = getStateFromPref(dt, "statusbar");

        } else if (id == "FlipHorizontal") {
            active = dt->is_flipped(SPDesktop::FLIP_HORIZONTAL);
        
        } else if (id == "FlipVertical") {
            active = dt->is_flipped(SPDesktop::FLIP_VERTICAL);

        } else {
            std::cerr << "checkitem_update: unhandled item: " << id << std::endl;
        }

        menuitem->set_active(active);
    } else {
        std::cerr << "checkitem_update: unknown action" << std::endl;
    }
}

static Gtk::CheckMenuItem*
build_menu_check_item_from_verb(SPAction* action)
{
    Gtk::CheckMenuItem* menuitem = Gtk::manage(new Gtk::CheckMenuItem(action->name, true));
    Inkscape::Shortcuts::getInstance().add_accelerator(menuitem, action->verb);

    // Set initial state before connecting signals.
    checkitem_update(menuitem, action);

    menuitem->signal_toggled().connect(
      sigc::bind<Gtk::CheckMenuItem*, SPAction*>(sigc::ptr_fun(&item_activate), menuitem, action));
    menuitem->signal_select().connect(  sigc::bind<SPAction*>(sigc::ptr_fun(&select_action),   action));
    menuitem->signal_deselect().connect(sigc::bind<SPAction*>(sigc::ptr_fun(&deselect_action), action));

    return menuitem;
}


// ================= Tasks Submenu ==============
static void
task_activated(SPDesktop* dt, int number)
{
    Inkscape::UI::UXManager::getInstance()->setTask(dt, number);

#ifdef GDK_WINDOWING_QUARTZ
    // call later, crashes during startup if called directly
    g_idle_add(sync_menubar, nullptr);
#endif
}

// Sets tip
static void
select_task(SPDesktop* dt, Glib::ustring tip)
{
    dt->tipsMessageContext()->set(Inkscape::NORMAL_MESSAGE, tip.c_str());
}

// Clears tip
static void
deselect_task(SPDesktop* dt)
{
    dt->tipsMessageContext()->clear();
}

static void
add_tasks(Gtk::MenuShell* menu, SPDesktop* dt)
{
    const Glib::ustring data[3][2] = {
        { C_("Interface setup", "Default"), _("Default interface setup")   },
        { C_("Interface setup", "Custom"),  _("Setup for custom task")     },
        { C_("Interface setup", "Wide"),    _("Setup for widescreen work") }
    };

    int active = Inkscape::UI::UXManager::getInstance()->getDefaultTask(dt);

    Gtk::RadioMenuItem::Group group;

    for (unsigned int i = 0; i < 3; ++i) {

        Gtk::RadioMenuItem* menuitem = Gtk::manage(new Gtk::RadioMenuItem(group, data[i][0]));
        if (menuitem) {
            if (active == i) {
                menuitem->set_active(true);
            }

            menuitem->signal_toggled().connect(
                sigc::bind<SPDesktop*, int>(sigc::ptr_fun(&task_activated), dt, i));
            menuitem->signal_select().connect(
                sigc::bind<SPDesktop*, Glib::ustring>(sigc::ptr_fun(&select_task),  dt, data[i][1]));
            menuitem->signal_deselect().connect(
                sigc::bind<SPDesktop*>(sigc::ptr_fun(&deselect_task),dt));

            menu->append(*menuitem);
        }
    }
}


static void
sp_recent_open(Gtk::RecentChooser* recentchooser)
{
    Glib::ustring uri = recentchooser->get_current_uri();

    Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(uri.raw());

    auto *app = InkscapeApplication::instance();

    app->create_window(file);
}

// =================== Main Menu ================
// Recursively build menu and submenus.
void
build_menu(Gtk::MenuShell* menu, Inkscape::XML::Node* xml, Inkscape::UI::View::View* view, bool show_icons = true)
{
    if (menu == nullptr) {
        std::cerr << "build_menu: menu is nullptr" << std::endl;
        return;
    }

    if (xml == nullptr) {
        std::cerr << "build_menu: xml is nullptr" << std::endl;
        return;
    }

    // user preference for icons in menus (1: show all, -1: hide all; 0: theme chooses per icon)
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int show_icons_pref = prefs->getInt("/theme/menuIcons", 0);

    Gtk::RadioMenuItem::Group group;

    for (auto menu_ptr = xml; menu_ptr != nullptr; menu_ptr = menu_ptr->next()) {

        if (menu_ptr->name()) {

            // show menu icons for current item?
            bool show_icons_curr = show_icons;
            if (show_icons_pref == 1) {          // show all icons per global pref
                show_icons_curr = true;
            } else if (show_icons_pref == -1) {  // hide all icons per global pref
                show_icons_curr = false;
            } else {  // set according to 'show-icons' attribute in theme's XML file; value is fully inherited
                const char *str = menu_ptr->attribute("show-icons");
                if (str) {
                    Glib::ustring ustr = str;
                    if (ustr == "true") {
                        show_icons_curr = true;
                    } else if (ustr == "false") {
                        show_icons_curr = false;
                    } else {
                        std::cerr << "build_menu: invalid value for 'show-icons' (use 'true' or 'false')."
                                  << ustr << std::endl;
                    }
                }
            }

            Glib::ustring name = menu_ptr->name();

            if (name == "inkscape") {
                build_menu(menu, menu_ptr->firstChild(), view, show_icons_curr);
                continue;
            }

            if (name == "submenu") {

                const char *name = menu_ptr->attribute("name");
                if (!name) {
                    g_warning("menus.xml: skipping submenu without name.");
                    continue;
                }

                Gtk::MenuItem* menuitem = Gtk::manage(new Gtk::MenuItem(_(name), true));
                menuitem->set_name(name);

                // TEMP
                if (strcmp(name, "_View") == 0) {
                    // Add from menu-view.ui first.

                    auto refBuilder = Gtk::Builder::create();
                    try
                    {
                        std::string filename =
                            Inkscape::IO::Resource::get_filename(Inkscape::IO::Resource::UIS, "menu-view.ui");
                        refBuilder->add_from_file(filename);
                    }
                    catch (const Glib::Error& err)
                    {
                        std::cerr << "build_menu: failed to load View menu from: "
                                  << "menu-view.ui: "
                                  << err.what() << std::endl;
                    }

                    auto object = refBuilder->get_object("view-menu");
                    auto gmenu = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
                    if (!gmenu) {
                        std::cerr << "build_menu: failed to build View menu!" << std::endl;
                    } else {
                        auto submenu = Gtk::manage(new Gtk::Menu(gmenu));
                        menuitem->set_submenu(*submenu);
                        menu->append(*menuitem);

                        // Rest of View menu from menus.xml
                        build_menu(submenu, menu_ptr->firstChild(), view, show_icons_curr);
                    }

                    continue;

                } else {

                    Gtk::Menu* submenu = Gtk::manage(new Gtk::Menu());
                    build_menu(submenu, menu_ptr->firstChild(), view, show_icons_curr);
                    menuitem->set_submenu(*submenu);
                    menu->append(*menuitem);

                    continue;
                }
            }

            if (name == "contextmenu") {
                if (menu_ptr->attribute("id")) {
                    Glib::ustring id = menu_ptr->attribute("id");
                    if (id == "canvas" || id == "layers" || id == "objects") {
                        Glib::ustring prefname = Glib::ustring::compose("/theme/menuIcons_%1", id);
                        prefs->setBool(prefname, show_icons_curr);
                    } else {
                        std::cerr << "build_menu: invalid contextmenu id: " << id << std::endl;
                    }
                } else {
                    std::cerr << "build_menu: contextmenu element needs a valid id" << std::endl;
                }
                continue;
            }

            if (name == "verb") {
                if (menu_ptr->attribute("verb-id") != nullptr) {
                    Glib::ustring verb_name = menu_ptr->attribute("verb-id");

                    Inkscape::Verb *verb = Inkscape::Verb::getbyid(verb_name.c_str());
                    if (verb != nullptr && verb->get_code() != SP_VERB_NONE) {

#ifdef GDK_WINDOWING_QUARTZ
                        if (verb->get_code() == SP_VERB_FILE_QUIT) {
                            continue;
                        }
#endif

                        SPAction* action = verb->get_action(Inkscape::ActionContext(view));
                        if (menu_ptr->attribute("check") != nullptr) {
                            Gtk::MenuItem *menuitem = build_menu_check_item_from_verb(action);
                            if (menuitem) {
                                std::pair<std::pair<unsigned int, Gtk::MenuItem *>, Inkscape::UI::View::View *>
                                    verbmenuitem = std::make_pair(std::make_pair(verb->get_code(), menuitem), view);
                                menuitems.push_back(verbmenuitem);
                                menu->append(*menuitem);
                            }
                        } else if (menu_ptr->attribute("radio") != nullptr) {
                            Gtk::MenuItem* menuitem = build_menu_item_from_verb(action, show_icons_curr, true, &group);
                            if (menuitem) {
                                if (menu_ptr->attribute("default") != nullptr) {
                                    auto radiomenuitem = dynamic_cast<Gtk::RadioMenuItem*>(menuitem);
                                    if (radiomenuitem) {
                                        radiomenuitem->set_active(true);
                                    }
                                }
                                std::pair<std::pair<unsigned int, Gtk::MenuItem *>, Inkscape::UI::View::View *>
                                    verbmenuitem = std::make_pair(std::make_pair(verb->get_code(), menuitem), view);
                                menuitems.push_back(verbmenuitem);
                                menu->append(*menuitem);
                            }
                        } else {
                            Gtk::MenuItem* menuitem = build_menu_item_from_verb(action, show_icons_curr);
                            if (menuitem) {
                                menu->append(*menuitem);
                            }

#ifdef GDK_WINDOWING_QUARTZ
                            // for moving menu items to "Inkscape" menu
                            switch (verb->get_code()) {
                                case SP_VERB_DIALOG_PREFERENCES:
                                case SP_VERB_DIALOG_INPUT:
                                case SP_VERB_HELP_ABOUT:
                                    menuitems.emplace_back(std::make_pair(verb->get_code(), menuitem), view);
                            }
#endif
                        }
                    } else if (true
#ifndef WITH_GSPELL
                        && strcmp(verb_name.c_str(), "DialogSpellcheck") != 0
#endif
                        ) {
                        std::cerr << "build_menu: no verb with id: " << verb_name << std::endl;
                    }
                }
                continue;
            }

            // This is used only for wide-screen vs non-wide-screen displays.
            // The code should be rewritten to use actions like everything else here.
            if (name == "task-checkboxes") {
                add_tasks(menu, static_cast<SPDesktop*>(view));
                continue;
            }

            if (name == "recent-file-list") {

                // Filter recent files to those already opened in Inkscape.
                Glib::RefPtr<Gtk::RecentFilter> recentfilter = Gtk::RecentFilter::create();
                recentfilter->add_application(g_get_prgname());
                recentfilter->add_application("org.inkscape.Inkscape");
                recentfilter->add_application("inkscape");
#ifdef _WIN32
                recentfilter->add_application("inkscape.exe");
#endif

                Gtk::RecentChooserMenu* recentchoosermenu = Gtk::manage(new Gtk::RecentChooserMenu());
                int max = Inkscape::Preferences::get()->getInt("/options/maxrecentdocuments/value");
                recentchoosermenu->set_limit(max);
                recentchoosermenu->set_sort_type(Gtk::RECENT_SORT_MRU); // Sort most recent first.
                recentchoosermenu->set_show_tips(true);
                recentchoosermenu->set_show_not_found(false);
                recentchoosermenu->add_filter(recentfilter);
                recentchoosermenu->signal_item_activated().connect(
                    sigc::bind<Gtk::RecentChooserMenu*>(sigc::ptr_fun(&sp_recent_open), recentchoosermenu));

                Gtk::MenuItem* menuitem = Gtk::manage(new Gtk::MenuItem(_("Open _Recent"), true));
                menuitem->set_submenu(*recentchoosermenu);
                menu->append(*menuitem);
                continue;
            }

            if (name == "separator") {
                Gtk::MenuItem* menuitem = Gtk::manage(new Gtk::SeparatorMenuItem());
                menu->append(*menuitem);
                continue;
            }

            // Comments and items handled elsewhere.
            if (name == "comment"      ||
                name == "filters-list" ||
                name == "effects-list" ) {
                continue;
            }

            std::cerr << "build_menu: unhandled option: " << name << std::endl;

        } else {
            std::cerr << "build_menu: xml node has no name!" << std::endl;
        }
    }
}

void
reload_menu(Inkscape::UI::View::View* view, Gtk::MenuBar* menubar)
{   
    menubar->hide();
    for (auto *widg : menubar->get_children()) {
        menubar->remove(*widg);
    }
    menuitems.clear();
    build_menu(menubar, INKSCAPE.get_menus()->parent(), view);

    shift_icons_recursive(menubar); // Find all submenus and add callback to each one.

    menubar->show_all();
#ifdef GDK_WINDOWING_QUARTZ
    sync_menubar();
    menubar->hide();
#endif
}

Gtk::MenuBar*
build_menubar(Inkscape::UI::View::View* view)
{
    Gtk::MenuBar* menubar = Gtk::manage(new Gtk::MenuBar());
    build_menu(menubar, INKSCAPE.get_menus()->parent(), view);
    SP_ACTIVE_DESKTOP->_menu_update.connect(sigc::ptr_fun(&set_menuitems));

    shift_icons_recursive(menubar); // Find all submenus and add callback to each one.

    return menubar;
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
