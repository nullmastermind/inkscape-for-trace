// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Main UI stuff.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2012 Kris De Gussem
 * Copyright (C) 2010 authors
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/radiomenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <glibmm/miscutils.h>

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "enums.h"
#include "file.h"
#include "gradient-drag.h"
#include "inkscape.h"
#include "inkscape-window.h"
#include "message-context.h"
#include "message-stack.h"
#include "preferences.h"
#include "selection-chemistry.h"
#include "shortcuts.h"

#include "display/sp-canvas.h"

#include "extension/db.h"
#include "extension/effect.h"
#include "extension/find_extension_by_mime.h"
#include "extension/input.h"

#include "helper/action.h"

#include "ui/icon-loader.h"

#include "io/sys.h"

#include "object/sp-anchor.h"
#include "object/sp-clippath.h"
#include "object/sp-flowtext.h"
#include "object/sp-image.h"
#include "object/sp-item-group.h"
#include "object/sp-mask.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "style.h"

#include "svg/svg-color.h"

#include "ui/clipboard.h"
#include "ui/contextmenu.h"
#include "ui/dialog-events.h"
#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/inkscape-preferences.h"
#include "ui/dialog/layer-properties.h"
#include "ui/interface.h"
#include "ui/monitor.h"
#include "ui/tools/tool-base.h"
#include "ui/uxmanager.h"
#include "ui/view/svg-view-widget.h"

#include "widgets/desktop-widget.h"
#include "widgets/ege-paint-def.h"

using Inkscape::DocumentUndo;

static bool temporarily_block_actions = false;

static void sp_ui_import_one_file(char const *filename);
static void sp_ui_import_one_file_with_check(gpointer filename, gpointer unused);
static void sp_ui_menu_item_set_name(GtkWidget *data,
                                     Glib::ustring const &name);
static void sp_recent_open(GtkRecentChooser *, gpointer);

void
sp_create_window(SPViewWidget *vw, bool editable)
{
    g_return_if_fail(vw != nullptr);
    g_return_if_fail(SP_IS_VIEW_WIDGET(vw));

    SPDesktopWidget *desktop_widget = reinterpret_cast<SPDesktopWidget*>(vw);
    SPDesktop* desktop = desktop_widget->desktop;
    SPDocument* document = desktop->getDocument();

    InkscapeWindow* win = new InkscapeWindow(document);
    win->set_desktop_widget(desktop_widget);

    if (editable) {
        g_object_set_data(G_OBJECT(vw), "window", win);


        desktop_widget->window = win;

        win->set_data("desktop", desktop);
        win->set_data("desktopwidget", desktop_widget);

        win->signal_delete_event().connect(sigc::mem_fun(*(SPDesktop*)vw->view, &SPDesktop::onDeleteUI));
        win->signal_window_state_event().connect(sigc::mem_fun(*desktop, &SPDesktop::onWindowStateEvent));
        win->signal_focus_in_event().connect(sigc::mem_fun(*desktop_widget, &SPDesktopWidget::onFocusInEvent));

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        int window_geometry = prefs->getInt("/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_NONE);
        if (window_geometry == PREFS_WINDOW_GEOMETRY_LAST) {
            gint pw = prefs->getInt("/desktop/geometry/width", -1);
            gint ph = prefs->getInt("/desktop/geometry/height", -1);
            gint px = prefs->getInt("/desktop/geometry/x", -1);
            gint py = prefs->getInt("/desktop/geometry/y", -1);
            gint full = prefs->getBool("/desktop/geometry/fullscreen");
            gint maxed = prefs->getBool("/desktop/geometry/maximized");
            if (pw>0 && ph>0) {
                Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_at_point(px, py);
                pw = std::min(pw, monitor_geometry.get_width());
                ph = std::min(ph, monitor_geometry.get_height());
                desktop->setWindowSize(pw, ph);
                desktop->setWindowPosition(Geom::Point(px, py));
            }
            if (maxed) {
                win->maximize();
            }
            if (full) {
                win->fullscreen();
            }
        }
    }

    win->show();

    // needed because the first ACTIVATE_DESKTOP was sent when there was no window yet
    if ( SP_IS_DESKTOP_WIDGET(vw) ) {
        INKSCAPE.reactivate_desktop(SP_DESKTOP_WIDGET(vw)->desktop);
    }
}

void
sp_ui_new_view()
{
    SPDocument *document;
    SPViewWidget *dtw;

    document = SP_ACTIVE_DOCUMENT;
    if (!document) return;

    dtw = sp_desktop_widget_new(sp_document_namedview(document, nullptr));
    g_return_if_fail(dtw != nullptr);

    sp_create_window(dtw, TRUE);
    sp_namedview_window_from_document(static_cast<SPDesktop*>(dtw->view));
    sp_namedview_update_layers_from_document(static_cast<SPDesktop*>(dtw->view));
}

void sp_ui_reload()
{

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/dialogs/preferences/page", PREFS_PAGE_UI_THEME);
    Inkscape::UI::Dialog::Dialog *prefs_dialog = SP_ACTIVE_DESKTOP->_dlg_mgr->getDialog("InkscapePreferences");
    if (prefs_dialog) {
        prefs_dialog->hide();
    }
    int window_geometry = prefs->getInt("/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_NONE);
    if (GtkSettings *settings = gtk_settings_get_default()) {
        Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
        if (themeiconname != "") {
            g_object_set(settings, "gtk-icon-theme-name", themeiconname.c_str(), NULL);
        }
    }
    prefs->setInt("/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_LAST);
    prefs->save();
    std::list<SPDesktop *> desktops;
    INKSCAPE.get_all_desktops(desktops);
    std::list<SPDesktop *>::iterator i = desktops.begin();
    while (i != desktops.end()) {
        SPDesktop *dt = *i;
        if (dt == nullptr) {
            ++i;
            continue;
        }
        dt->storeDesktopPosition();
        SPDocument *document;
        SPViewWidget *dtw;

        document = dt->getDocument();
        if (!document) {
            ++i;
            continue;
        }

        dtw = sp_desktop_widget_new(sp_document_namedview(document, nullptr));
        if (dtw == nullptr) {
            ++i;
            continue;
        }
        sp_create_window(dtw, TRUE);
        SPDesktop *desktop = static_cast<SPDesktop *>(dtw->view);
        if (desktop) {
            sp_namedview_window_from_document(desktop);
            sp_namedview_update_layers_from_document(desktop);
        }
        dt->destroyWidget();
        i++;
    }
    SP_ACTIVE_DESKTOP->_dlg_mgr->showDialog("InkscapePreferences");
    INKSCAPE.add_gtk_css();
    prefs->setInt("/options/savewindowgeometry/value", window_geometry);
}

void
sp_ui_close_view(GtkWidget */*widget*/)
{
    SPDesktop *dt = SP_ACTIVE_DESKTOP;

    if (dt == nullptr) {
        return;
    }

    if (dt->shutdown()) {
        return; // Shutdown operation has been canceled, so do nothing
    }

    // If closing the last document, open a new document so Inkscape doesn't quit.
    std::list<SPDesktop *> desktops;
    INKSCAPE.get_all_desktops(desktops);
    if (desktops.size() == 1) {
        Glib::ustring templateUri = sp_file_default_template_uri();
        SPDocument *doc = SPDocument::createNewDoc( templateUri.empty() ? nullptr : templateUri.c_str(), TRUE, true );
        // Set viewBox if it doesn't exist
        if (!doc->getRoot()->viewBox_set) {
            doc->setViewBox(Geom::Rect::from_xywh(0, 0, doc->getWidth().value(doc->getDisplayUnit()), doc->getHeight().value(doc->getDisplayUnit())));
        }
        dt->change_document(doc);
        sp_namedview_window_from_document(dt);
        sp_namedview_update_layers_from_document(dt);
        return;
    }

    // Shutdown can proceed; use the stored reference to the desktop here instead of the current SP_ACTIVE_DESKTOP,
    // because the user might have changed the focus in the meantime (see bug #381357 on Launchpad)
    dt->destroyWidget();
}


unsigned int
sp_ui_close_all()
{
    /* Iterate through all the windows, destroying each in the order they
       become active */
    while (SP_ACTIVE_DESKTOP) {
        SPDesktop *dt = SP_ACTIVE_DESKTOP;
        if (dt->shutdown()) {
            /* The user canceled the operation, so end doing the close */
            return FALSE;
        }
        // Shutdown can proceed; use the stored reference to the desktop here instead of the current SP_ACTIVE_DESKTOP,
        // because the user might have changed the focus in the meantime (see bug #381357 on Launchpad)
        dt->destroyWidget();
    }

    return TRUE;
}

/*
 * Some day when the right-click menus are ready to start working
 * smarter with the verbs, we'll need to change this NULL being
 * sent to sp_action_perform to something useful, or set some kind
 * of global "right-clicked position" variable for actions to
 * investigate when they're called.
 */
static void
sp_ui_menu_activate(void */*object*/, SPAction *action)
{
    if (!temporarily_block_actions) {
        sp_action_perform(action, nullptr);
    }
}

static void
sp_ui_menu_select_action(void */*object*/, SPAction *action)
{
    sp_action_get_view(action)->tipsMessageContext()->set(Inkscape::NORMAL_MESSAGE, action->tip);
}

static void
sp_ui_menu_deselect_action(void */*object*/, SPAction *action)
{
    sp_action_get_view(action)->tipsMessageContext()->clear();
}

static void
sp_ui_menu_select(gpointer object, gpointer tip)
{
    Inkscape::UI::View::View *view = static_cast<Inkscape::UI::View::View*> (g_object_get_data(G_OBJECT(object), "view"));
    view->tipsMessageContext()->set(Inkscape::NORMAL_MESSAGE, (gchar *)tip);
}

static void
sp_ui_menu_deselect(gpointer object)
{
    Inkscape::UI::View::View *view = static_cast<Inkscape::UI::View::View*>  (g_object_get_data(G_OBJECT(object), "view"));
    view->tipsMessageContext()->clear();
}


void
sp_ui_dialog_title_string(Inkscape::Verb *verb, gchar *c)
{
    SPAction     *action;
    unsigned int shortcut;
    gchar        *s;
    gchar        *atitle;

    action = verb->get_action(Inkscape::ActionContext());
    if (!action)
        return;

    atitle = sp_action_get_title(action);

    s = g_stpcpy(c, atitle);

    g_free(atitle);

    shortcut = sp_shortcut_get_primary(verb);
    if (shortcut!=GDK_KEY_VoidSymbol) {
        gchar* key = sp_shortcut_get_label(shortcut);
        s = g_stpcpy(s, " (");
        s = g_stpcpy(s, key);
        g_stpcpy(s, ")");
        g_free(key);
    }
}

/* install CSS to shift icons into the space reserved for toggles (i.e. check and radio items)
 *
 * TODO: This code already exists as a C++ version in the class ContextMenu so we can simply wrap it here.
 *       In future ContextMenu and the (to be created) class for the menu bar should then be derived from one common base class.
 */
void shift_icons(GtkWidget *menu, gpointer /* user_data */)
{
    ContextMenu *contextmenu = static_cast<ContextMenu *>(Glib::wrap(menu));
    contextmenu->ShiftIcons();
}

/**
 * Appends a custom menu UI from a verb.
 *
 * @see ContextMenu::AppendItemFromVerb for a c++ified alternative. Consider dropping sp_ui_menu_append_item_from_verb when c++ifying interface.cpp.
 *
 * @param menu      The menu to which the item will be appended
 * @param verb      The verb from which the item's label, action and icon (optionally) will be read
 * @param view
 * @param show_icon True if an icon should be displayed before the menu item's label
 * @param radio     True if a radio button should be displayed next to the menu item
 * @param group     The radio button group that the item should belong to
 *
 * @details The show_icon flag should be used very sparingly because menu icons are not recommended
 *          any longer under the GNOME HIG.  Also, note that the text appears after the icon, and
 *          so will be indented relative to "normal" menu items.  As such, menus will look best if
 *          all the items with icons are grouped together between a pair of separators.
 */
static GtkWidget *sp_ui_menu_append_item_from_verb(GtkMenu                  *menu,
                                                   Inkscape::Verb           *verb,
                                                   Inkscape::UI::View::View *view,
                                                   bool                      show_icon = false,
                                                   bool                      radio     = false,
                                                   Gtk::RadioMenuItem::Group *group    = nullptr)
{
    Gtk::Widget *item;

    // Just create a menu separator if this isn't a real action. Otherwise, create a real menu item
    if (verb->get_code() == SP_VERB_NONE) {
        item = new Gtk::SeparatorMenuItem();
    } else {
        SPAction *action = verb->get_action(Inkscape::ActionContext(view));

        if (!action) return nullptr;

        // Create the menu item itself, either as a radio menu item, or just
        // a regular menu item depending on whether the "radio" flag is set
        if (radio) {
            item = new Gtk::RadioMenuItem(*group);
        } else {
            item = new Gtk::MenuItem();
        }

        // Now create the label and add it to the menu item
        GtkWidget *label = gtk_accel_label_new(action->name);
        gtk_label_set_markup_with_mnemonic( GTK_LABEL(label), action->name);

#if GTK_CHECK_VERSION(3,16,0)
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
#else
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
#endif
        sp_shortcut_add_accelerator(item->gobj(), sp_shortcut_get_primary(verb));
        gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(label), item->gobj());

        // If there is an image associated with the action, then we can add it as an icon for the menu item.
        if (show_icon && action->image) {
            item->set_name("ImageMenuItem");  // custom name to identify our "ImageMenuItems"
            GtkWidget *icon = sp_get_icon_image(action->image, GTK_ICON_SIZE_MENU);

            // create a box to hold icon and label as GtkMenuItem derives from GtkBin and can only hold one child
            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

            gtk_container_add(GTK_CONTAINER(item->gobj()), box);
        } else {
            gtk_container_add(GTK_CONTAINER(item->gobj()), label);
        }

        action->signal_set_sensitive.connect(
            sigc::bind<0>(
                sigc::ptr_fun(&gtk_widget_set_sensitive),
                item->gobj()));
        action->signal_set_name.connect(
            sigc::bind<0>(
                sigc::ptr_fun(&sp_ui_menu_item_set_name),
                item->gobj()));

        if (!action->sensitive) {
            item->set_sensitive(false);
        }

        gtk_widget_set_events(item->gobj(), GDK_KEY_PRESS_MASK);

        g_object_set_data(G_OBJECT(item->gobj()), "view", (gpointer) view);
        g_signal_connect( G_OBJECT(item->gobj()), "activate", G_CALLBACK(sp_ui_menu_activate), action );
        g_signal_connect( G_OBJECT(item->gobj()), "select", G_CALLBACK(sp_ui_menu_select_action), action );
        g_signal_connect( G_OBJECT(item->gobj()), "deselect", G_CALLBACK(sp_ui_menu_deselect_action), action );
    }

    item->show_all();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item->gobj());

    return item->gobj();

} // end of sp_ui_menu_append_item_from_verb


Glib::ustring getLayoutPrefPath( Inkscape::UI::View::View *view )
{
    Glib::ustring prefPath;

    if (reinterpret_cast<SPDesktop*>(view)->is_focusMode()) {
        prefPath = "/focus/";
    } else if (reinterpret_cast<SPDesktop*>(view)->is_fullscreen()) {
        prefPath = "/fullscreen/";
    } else {
        prefPath = "/window/";
    }

    return prefPath;
}

static void
checkitem_toggled(GtkCheckMenuItem *menuitem, gpointer user_data)
{
    gchar const *pref = (gchar const *) user_data;
    Inkscape::UI::View::View *view = (Inkscape::UI::View::View *) g_object_get_data(G_OBJECT(menuitem), "view");
    SPAction *action = (SPAction *) g_object_get_data(G_OBJECT(menuitem), "action");

    if (action) {

        sp_ui_menu_activate(menuitem, action);

    } else if (pref) {
        // All check menu items should have actions now, but just in case
        Glib::ustring pref_path = getLayoutPrefPath( view );
        pref_path += pref;
        pref_path += "/state";

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        gboolean checked = gtk_check_menu_item_get_active(menuitem);
        prefs->setBool(pref_path, checked);

        reinterpret_cast<SPDesktop*>(view)->layoutWidget();
    }
}

static bool getViewStateFromPref(Inkscape::UI::View::View *view, gchar const *pref)
{
    Glib::ustring pref_path = getLayoutPrefPath( view );
    pref_path += pref;
    pref_path += "/state";

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    return prefs->getBool(pref_path, true);
}

static gboolean checkitem_update(GtkWidget *widget, cairo_t * /*cr*/, gpointer user_data)
{
    GtkCheckMenuItem *menuitem=GTK_CHECK_MENU_ITEM(widget);

    gchar const *pref = (gchar const *) user_data;
    Inkscape::UI::View::View *view = (Inkscape::UI::View::View *) g_object_get_data(G_OBJECT(menuitem), "view");
    SPAction *action = (SPAction *) g_object_get_data(G_OBJECT(menuitem), "action");
    SPDesktop *dt = static_cast<SPDesktop*>(view);

    bool ison = false;
    if (action) {

        if (!strcmp(action->id, "ToggleGrid")) {
            ison = dt->gridsEnabled();
        } else if (!strcmp(action->id, "EditGuidesToggleLock")) {
            ison = dt->namedview->lockguides;
        } else if (!strcmp(action->id, "ToggleGuides")) {
            ison = dt->namedview->getGuides();
        } else if (!strcmp(action->id, "ViewCmsToggle")) {
            ison = dt->colorProfAdjustEnabled();
        } else if (!strcmp(action->id, "ViewSplitModeToggle")) {
            ison = dt->splitMode();
        } else if (!strcmp(action->id, "ViewXRayToggle")) {
            ison = dt->xrayMode();
        } else {
            ison = getViewStateFromPref(view, pref);
        }
    } else if (pref) {
        // The Show/Hide menu items without actions
        ison = getViewStateFromPref(view, pref);
    }

    g_signal_handlers_block_by_func(G_OBJECT(menuitem), (gpointer)(GCallback)checkitem_toggled, user_data);
    gtk_check_menu_item_set_active(menuitem, ison);
    g_signal_handlers_unblock_by_func(G_OBJECT(menuitem), (gpointer)(GCallback)checkitem_toggled, user_data);

    return FALSE;
}


static void taskToggled(GtkCheckMenuItem *menuitem, gpointer userData)
{
    if ( gtk_check_menu_item_get_active(menuitem) ) {
        gint taskNum = GPOINTER_TO_INT(userData);
        taskNum = (taskNum < 0) ? 0 : (taskNum > 2) ? 2 : taskNum;

        Inkscape::UI::View::View *view = (Inkscape::UI::View::View *) g_object_get_data(G_OBJECT(menuitem), "view");

        // note: this will change once more options are in the task set support:
        Inkscape::UI::UXManager::getInstance()->setTask( dynamic_cast<SPDesktop*>(view), taskNum );
    }
}


/**
 * Callback function to update the status of the radio buttons in the View -> Display mode menu (Normal, No Filters, Outline) and Color display mode.
 */
static gboolean update_view_menu(GtkWidget *widget, cairo_t * /*cr*/, gpointer user_data)
{
    SPAction *action = (SPAction *) user_data;
    g_assert(action->id != nullptr);

    Inkscape::UI::View::View *view = (Inkscape::UI::View::View *) g_object_get_data(G_OBJECT(widget), "view");
    SPDesktop *dt = static_cast<SPDesktop*>(view);
    Inkscape::RenderMode mode = dt->getMode();
    Inkscape::ColorMode colormode = dt->getColorMode();

    bool new_state = false;
    if (!strcmp(action->id, "ViewModeNormal")) {
        new_state = mode == Inkscape::RENDERMODE_NORMAL;
    } else if (!strcmp(action->id, "ViewModeNoFilters")) {
        new_state = mode == Inkscape::RENDERMODE_NO_FILTERS;
    } else if (!strcmp(action->id, "ViewModeOutline")) {
        new_state = mode == Inkscape::RENDERMODE_OUTLINE;
    } else if (!strcmp(action->id, "ViewModeVisibleHairlines")) {
        new_state = mode == Inkscape::RENDERMODE_VISIBLE_HAIRLINES;
    } else if (!strcmp(action->id, "ViewColorModeNormal")) {
        new_state = colormode == Inkscape::COLORMODE_NORMAL;
    } else if (!strcmp(action->id, "ViewColorModeGrayscale")) {
        new_state = colormode == Inkscape::COLORMODE_GRAYSCALE;
    } else if (!strcmp(action->id, "ViewColorModePrintColorsPreview")) {
        new_state = colormode == Inkscape::COLORMODE_PRINT_COLORS_PREVIEW;
    } else {
        g_warning("update_view_menu does not handle this verb");
    }

    if (new_state) { //only one of the radio buttons has to be activated; the others will automatically be deactivated
        if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget))) {
            // When the GtkMenuItem version of the "activate" signal has been emitted by a GtkRadioMenuItem, there is a second
            // emission as the most recently active item is toggled to inactive. This is dealt with before the original signal is handled.
            // This emission however should not invoke any actions, hence we block it here:
            temporarily_block_actions = true;
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
            temporarily_block_actions = false;
        }
    }

    return FALSE;
}

static void
sp_ui_menu_append_check_item_from_verb(GtkMenu *menu, Inkscape::UI::View::View *view, gchar const *pref, Inkscape::Verb *verb)
{
    unsigned int shortcut = sp_shortcut_get_primary(verb);
    SPAction *action = verb->get_action(Inkscape::ActionContext(view));
    GtkWidget *item = gtk_check_menu_item_new_with_mnemonic(action->name);

    sp_shortcut_add_accelerator(item, shortcut);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    g_object_set_data(G_OBJECT(item), "view", (gpointer) view);
    g_object_set_data(G_OBJECT(item), "action", (gpointer) action);

    g_signal_connect( G_OBJECT(item), "toggled", (GCallback) checkitem_toggled, (void *) pref);
    g_signal_connect( G_OBJECT(item), "draw", (GCallback) checkitem_update, (void *) pref);

    checkitem_update(item, nullptr, (void *)pref);

    g_signal_connect( G_OBJECT(item), "select", G_CALLBACK(sp_ui_menu_select_action), action );
    g_signal_connect( G_OBJECT(item), "deselect", G_CALLBACK(sp_ui_menu_deselect_action), action );
}

static void
sp_recent_open(GtkRecentChooser *recent_menu, gpointer /*user_data*/)
{
    // dealing with the bizarre filename convention in Inkscape for now
    gchar *uri = gtk_recent_chooser_get_current_uri(GTK_RECENT_CHOOSER(recent_menu));
    gchar *local_fn = g_filename_from_uri(uri, nullptr, nullptr);
    gchar *utf8_fn = g_filename_to_utf8(local_fn, -1, nullptr, nullptr, nullptr);
    sp_file_open(utf8_fn, nullptr);
    g_free(utf8_fn);
    g_free(local_fn);
    g_free(uri);
}

static void
sp_ui_checkboxes_menus(GtkMenu *m, Inkscape::UI::View::View *view)
{
    //sp_ui_menu_append_check_item_from_verb(m, view, "menu", 0);
    sp_ui_menu_append_check_item_from_verb(m, view, "commands",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_COMMANDS_TOOLBAR));
    sp_ui_menu_append_check_item_from_verb(m, view,"snaptoolbox",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_SNAP_TOOLBAR));
    sp_ui_menu_append_check_item_from_verb(m, view, "toppanel",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_TOOL_TOOLBAR));
    sp_ui_menu_append_check_item_from_verb(m, view, "toolbox",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_TOOLBOX));
    sp_ui_menu_append_check_item_from_verb(m, view, "rulers",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_RULERS));
    sp_ui_menu_append_check_item_from_verb(m, view, "scrollbars",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_SCROLLBARS));
    sp_ui_menu_append_check_item_from_verb(m, view, "panels",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_PALETTE));
    sp_ui_menu_append_check_item_from_verb(m, view, "statusbar",
                                           Inkscape::Verb::get(SP_VERB_TOGGLE_STATUSBAR));
}


static void addTaskMenuItems(GtkMenu *menu, Inkscape::UI::View::View *view)
{
    gchar const* data[] = {
        C_("Interface setup", "Default"), _("Default interface setup"),
        C_("Interface setup", "Custom"), _("Setup for custom task"),
        C_("Interface setup", "Wide"), _("Setup for widescreen work"),
        nullptr, nullptr
    };

    Gtk::RadioMenuItem::Group group;
    int count = 0;
    gint active = Inkscape::UI::UXManager::getInstance()->getDefaultTask( dynamic_cast<SPDesktop*>(view) );
    for (gchar const **strs = data; strs[0]; strs += 2, count++)
    {
        Gtk::RadioMenuItem *item = new Gtk::RadioMenuItem(group,Glib::ustring(strs[0]));
        if ( count == active )
        {
            item->set_active();
        }

        g_object_set_data( G_OBJECT(item->gobj()), "view", view );
        g_signal_connect( G_OBJECT(item->gobj()), "toggled", reinterpret_cast<GCallback>(taskToggled), GINT_TO_POINTER(count) );
        g_signal_connect( G_OBJECT(item->gobj()), "select", G_CALLBACK(sp_ui_menu_select), const_cast<gchar*>(strs[1]) );
        g_signal_connect( G_OBJECT(item->gobj()), "deselect", G_CALLBACK(sp_ui_menu_deselect), 0 );

        item->show();
        gtk_menu_shell_append( GTK_MENU_SHELL(menu), GTK_WIDGET(item->gobj()) );
    }
}


/**
 * Observer that updates the recent list's max document count.
 */
class MaxRecentObserver : public Inkscape::Preferences::Observer {
public:
    MaxRecentObserver(GtkWidget *recent_menu) :
        Observer("/options/maxrecentdocuments/value"),
        _rm(recent_menu)
    {}
    void notify(Inkscape::Preferences::Entry const &e) override {
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(_rm), e.getInt());
        // hack: the recent menu doesn't repopulate after changing the limit, so we force it
        g_signal_emit_by_name((gpointer) gtk_recent_manager_get_default(), "changed");
    }
private:
    GtkWidget *_rm;
};

/**
 * This function turns XML into a menu.
 *
 *  This function is realitively simple as it just goes through the XML
 *  and parses the individual elements.  In the case of a submenu, it
 *  just calls itself recursively.  Because it is only reasonable to have
 *  a couple of submenus, it is unlikely this will go more than two or
 *  three times.
 *
 *  In the case of an unrecognized verb, a menu item is made to identify
 *  the verb that is missing, and display that.  The menu item is also made
 *  insensitive.
 *
 * @param  menus        This is the XML that defines the menu
 * @param  menu         Menu to be added to
 * @param  view         The View that this menu is being built for
 * @param  show_icons   Whether to show icons (can be overridden by the current XML::Node and preferences)
 */
static void sp_ui_build_dyn_menus(Inkscape::XML::Node *menus, GtkWidget *menu, Inkscape::UI::View::View *view,
                                  bool show_icons = true)
{
    if (menus == nullptr) return;
    if (menu == nullptr)  return;
    Gtk::RadioMenuItem::Group group;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int show_icons_pref = prefs->getInt("/theme/menuIcons", 0);

    for (Inkscape::XML::Node *menu_pntr = menus; menu_pntr != nullptr; menu_pntr = menu_pntr->next()) {

        // Check if the "show-icons" attribute is set, and set the flag here accordingly
        bool show_icons_curr = show_icons;
        if (show_icons_pref == 1) {          // enable all icons
            show_icons_curr = true;
        } else if (show_icons_pref == -1) {  // disable all icons
            show_icons_curr = false;
        } else {                             // read from XML file or inherit (=keep) previous value if unset
            const char *str = menu_pntr->attribute("show-icons");
            if (str) {
                if (!g_ascii_strcasecmp(str, "yes") || !g_ascii_strcasecmp(str, "true") || !g_ascii_strcasecmp(str, "1")) {
                    show_icons_curr = true;
                } else if (!g_ascii_strcasecmp(str, "no") || !g_ascii_strcasecmp(str, "false") || !g_ascii_strcasecmp(str, "0")) {
                    show_icons_curr = false;
                } else {
                    g_warning("Invalid value for attribute 'show-icons': '%s'", str);
                }
            }
        }

        if (!strcmp(menu_pntr->name(), "inkscape")) {
            sp_ui_build_dyn_menus(menu_pntr->firstChild(), menu, view, show_icons_curr);
        }
        if (!strcmp(menu_pntr->name(), "submenu")) {
            GtkWidget *mitem;
            if (menu_pntr->attribute("_name") != nullptr) {
                mitem = gtk_menu_item_new_with_mnemonic(_(menu_pntr->attribute("_name")));
            } else {
                mitem = gtk_menu_item_new_with_mnemonic(menu_pntr->attribute("name"));
            }
            GtkWidget *submenu = gtk_menu_new();
            sp_ui_build_dyn_menus(menu_pntr->firstChild(), submenu, view, show_icons_curr);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), GTK_WIDGET(submenu));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
            g_signal_connect(submenu, "map", G_CALLBACK(shift_icons), NULL);
            continue;
        }
        if (!strcmp(menu_pntr->name(), "verb")) {
            gchar const *verb_name = menu_pntr->attribute("verb-id");
            Inkscape::Verb *verb = Inkscape::Verb::getbyid(verb_name);

            if (verb != nullptr) {
                if (menu_pntr->attribute("radio") != nullptr) {
                    GtkWidget *item = sp_ui_menu_append_item_from_verb (GTK_MENU(menu), verb, view, false, true, &group);
                    if (menu_pntr->attribute("default") != nullptr) {
                        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
                    }
                    if (verb->get_code() != SP_VERB_NONE) {
                        SPAction *action = verb->get_action(Inkscape::ActionContext(view));
                        g_signal_connect( G_OBJECT(item), "draw", (GCallback) update_view_menu, (void *) action);
                    }
                } else if (menu_pntr->attribute("check") != nullptr) {
                    if (verb->get_code() != SP_VERB_NONE) {
                        SPAction *action = verb->get_action(Inkscape::ActionContext(view));
                        sp_ui_menu_append_check_item_from_verb(GTK_MENU(menu), view, nullptr, verb);
                    }
                } else {
                    sp_ui_menu_append_item_from_verb(GTK_MENU(menu), verb, view, show_icons_curr);
                    Gtk::RadioMenuItem::Group group2;
                    group = group2;
                }
            } else {
                gchar string[120];
                g_snprintf(string, 120, _("Verb \"%s\" Unknown"), verb_name);
                string[119] = '\0'; /* may not be terminated */
                GtkWidget *item = gtk_menu_item_new_with_label(string);
                gtk_widget_set_sensitive(item, false);
                gtk_widget_show(item);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            }
            continue;
        }
        if (!strcmp(menu_pntr->name(), "separator")) {
            GtkWidget *item = gtk_separator_menu_item_new();
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            continue;
        }

        if (!strcmp(menu_pntr->name(), "recent-file-list")) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();

            // create recent files menu
            int max_recent = prefs->getInt("/options/maxrecentdocuments/value");
            GtkWidget *recent_menu = gtk_recent_chooser_menu_new_for_manager(gtk_recent_manager_get_default());
            gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_menu), max_recent);
            // sort most recently used documents first to preserve previous behavior
            gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(recent_menu), GTK_RECENT_SORT_MRU);
            g_signal_connect(G_OBJECT(recent_menu), "item-activated", G_CALLBACK(sp_recent_open), (gpointer) nullptr);

            // add filter to only open files added by Inkscape
            GtkRecentFilter *inkscape_only_filter = gtk_recent_filter_new();
            gtk_recent_filter_add_application(inkscape_only_filter, g_get_prgname());
            gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(recent_menu), inkscape_only_filter);

            gtk_recent_chooser_set_show_tips (GTK_RECENT_CHOOSER(recent_menu), TRUE);
            gtk_recent_chooser_set_show_not_found (GTK_RECENT_CHOOSER(recent_menu), FALSE);

            GtkWidget *recent_item = gtk_menu_item_new_with_mnemonic(_("Open _Recent"));
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(recent_item));
            // this will just sit and update the list's item count
            static MaxRecentObserver *mro = new MaxRecentObserver(recent_menu);
            prefs->addObserver(*mro);
            continue;
        }
        if (!strcmp(menu_pntr->name(), "objects-checkboxes")) {
            sp_ui_checkboxes_menus(GTK_MENU(menu), view);
            continue;
        }
        if (!strcmp(menu_pntr->name(), "task-checkboxes")) {
            addTaskMenuItems(GTK_MENU(menu), view);
            continue;
        }
    }
}

GtkWidget *sp_ui_main_menubar(Inkscape::UI::View::View *view)
{
    GtkWidget *mbar = gtk_menu_bar_new();
    sp_ui_build_dyn_menus(INKSCAPE.get_menus()->parent(), mbar, view);
    return mbar;
}

void
sp_ui_import_files(gchar *buffer)
{
    gchar** l = g_uri_list_extract_uris(buffer);
    for (unsigned int i=0; i < g_strv_length(l); i++) {
        gchar *f = g_filename_from_uri (l[i], nullptr, nullptr);
        sp_ui_import_one_file_with_check(f, nullptr);
        g_free(f);
    }
    g_strfreev(l);
}

static void
sp_ui_import_one_file_with_check(gpointer filename, gpointer /*unused*/)
{
    if (filename) {
        if (strlen((char const *)filename) > 2)
            sp_ui_import_one_file((char const *)filename);
    }
}

static void
sp_ui_import_one_file(char const *filename)
{
    SPDocument *doc = SP_ACTIVE_DOCUMENT;
    if (!doc) return;

    if (filename == nullptr) return;

    // Pass off to common implementation
    // TODO might need to get the proper type of Inkscape::Extension::Extension
    file_import( doc, filename, nullptr );
}

void
sp_ui_error_dialog(gchar const *message)
{
    GtkWidget *dlg;
    gchar *safeMsg = Inkscape::IO::sanitizeString(message);

    dlg = gtk_message_dialog_new(nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE, "%s", safeMsg);
    sp_transientize(dlg);
    gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    g_free(safeMsg);
}

bool
sp_ui_overwrite_file(gchar const *filename)
{
    bool return_value = FALSE;

    if (Inkscape::IO::file_test(filename, G_FILE_TEST_EXISTS)) {
        Gtk::Window *window = SP_ACTIVE_DESKTOP->getToplevel();
        gchar* baseName = g_path_get_basename( filename );
        gchar* dirName = g_path_get_dirname( filename );
        GtkWidget* dialog = gtk_message_dialog_new_with_markup( window->gobj(),
                                                                (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                                GTK_MESSAGE_QUESTION,
                                                                GTK_BUTTONS_NONE,
                                                                _( "<span weight=\"bold\" size=\"larger\">A file named \"%s\" already exists. Do you want to replace it?</span>\n\n"
                                                                   "The file already exists in \"%s\". Replacing it will overwrite its contents." ),
                                                                baseName,
                                                                dirName
            );
        gtk_dialog_add_buttons( GTK_DIALOG(dialog),
                                _("_Cancel"), GTK_RESPONSE_NO,
                                _("Replace"), GTK_RESPONSE_YES,
                                NULL );
        gtk_dialog_set_default_response( GTK_DIALOG(dialog), GTK_RESPONSE_YES );

        if ( gtk_dialog_run( GTK_DIALOG(dialog) ) == GTK_RESPONSE_YES ) {
            return_value = TRUE;
        } else {
            return_value = FALSE;
        }
        gtk_widget_destroy(dialog);
        g_free( baseName );
        g_free( dirName );
    } else {
        return_value = TRUE;
    }

    return return_value;
}

static void
sp_ui_menu_item_set_name(GtkWidget *data, Glib::ustring const &name)
{
    if (data || GTK_IS_BIN (data)) {
        void *child = gtk_bin_get_child (GTK_BIN (data));
        // child is either
        // - a GtkLabel (if the menu has no accel key or icon)
        // - a GtkBox (if item has an accel key or image)
        //   in which case we need to find the GtkLabel in the box
        // - something else we do not expect (yet)
        if (child != nullptr) {
            if (GTK_IS_LABEL(child)) {
                gtk_label_set_markup_with_mnemonic(GTK_LABEL (child), name.c_str());
            } else if (GTK_IS_BOX(child)) {
                std::vector<Gtk::Widget*> children = Glib::wrap(GTK_CONTAINER(child))->get_children();
                for (auto child: children) {
                    Gtk::Label *label = dynamic_cast<Gtk::Label *>(child);
                    if (label) {
                        label->set_markup_with_mnemonic(name);
                        break;
                    }
                }
            } else {
                // sp_ui_menu_append_item_from_verb might have learned to set a menu item in yet another way...
                g_assert_not_reached();
            }
        }
    }
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
