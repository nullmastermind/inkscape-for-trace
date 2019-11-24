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

#include "desktop.h"
#include "document.h"
#include "enums.h"
#include "file.h"
#include "inkscape.h"
#include "inkscape-window.h"
#include "preferences.h"
#include "shortcuts.h"

#include "extension/db.h"
#include "extension/effect.h"
#include "extension/find_extension_by_mime.h"
#include "extension/input.h"

#include "helper/action.h"

#include "io/sys.h"

#include "object/sp-namedview.h"
#include "object/sp-root.h"

#include "ui/dialog-events.h"
#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/inkscape-preferences.h"
#include "ui/dialog/layer-properties.h"
#include "ui/interface.h"

#include "ui/view/svg-view-widget.h"

#include "widgets/desktop-widget.h"

static void sp_ui_import_one_file(char const *filename);
static void sp_ui_import_one_file_with_check(gpointer filename, gpointer unused);

void
sp_ui_new_view()
{
    SPDocument *document;

    document = SP_ACTIVE_DOCUMENT;
    if (!document) return;

    ConcreteInkscapeApplication<Gtk::Application>* app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());

    InkscapeWindow* win = app->window_open (document);
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

    ConcreteInkscapeApplication<Gtk::Application>* app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());

    InkscapeWindow* window = SP_ACTIVE_DESKTOP->getInkscapeWindow();

    // If closing the last document, open a new document so Inkscape doesn't quit.
    std::list<SPDesktop *> desktops;
    INKSCAPE.get_all_desktops(desktops);
    if (desktops.size() == 1) {

        SPDocument* old_document = window->get_document();

        Glib::ustring template_path = sp_file_default_template_uri();
        SPDocument *doc = app->document_new (template_path);

        app->document_swap (window, doc);

        if (app->document_window_count(old_document) == 0) {
            app->document_close(old_document);
        }

        // Are these necessary?
        sp_namedview_window_from_document(dt);
        sp_namedview_update_layers_from_document(dt);

    } else {

        app->destroy_window (window);
    }
}


unsigned int
sp_ui_close_all()
{

    ConcreteInkscapeApplication<Gtk::Application>* app = &(ConcreteInkscapeApplication<Gtk::Application>::get_instance());

    app->destroy_all();

    return true;
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
