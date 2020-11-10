// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * File/Print operations.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Chema Celorio <chema@celorio.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *   Stephen Silver <sasilver@users.sourceforge.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   David Xiong
 *   Tavmjong Bah
 *
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 1999-2016 Authors
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/** @file
 * @note This file needs to be cleaned up extensively.
 * What it probably needs is to have one .h file for
 * the API, and two or more .cpp files for the implementations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <gtkmm.h>

#include "file.h"
#include "inkscape-application.h"
#include "inkscape-window.h"

#include "desktop.h"
#include "document-undo.h"
#include "event-log.h"
#include "id-clash.h"
#include "inkscape-version.h"
#include "inkscape.h"
#include "message-stack.h"
#include "path-prefix.h"
#include "print.h"
#include "rdf.h"
#include "selection-chemistry.h"
#include "verbs.h"

#include "extension/db.h"
#include "extension/effect.h"
#include "extension/input.h"
#include "extension/output.h"

#include "helper/png-write.h"

#include "io/file.h"
#include "io/resource.h"
#include "io/resource-manager.h"
#include "io/sys.h"

#include "object/sp-defs.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "object/sp-use.h"
#include "style.h"

#include "ui/dialog/font-substitution.h"
#include "ui/dialog/filedialog.h"
#include "ui/interface.h"
#include "ui/tools/tool-base.h"
#include "widgets/desktop-widget.h"

#include "svg/svg.h" // for sp_svg_transform_write, used in sp_import_document
#include "xml/rebase-hrefs.h"
#include "xml/sp-css-attr.h"

using Inkscape::DocumentUndo;
using Inkscape::IO::Resource::TEMPLATES;
using Inkscape::IO::Resource::USER;

#ifdef WITH_DBUS
#include "extension/dbus/dbus-init.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

//#define INK_DUMP_FILENAME_CONV 1
#undef INK_DUMP_FILENAME_CONV

//#define INK_DUMP_FOPEN 1
#undef INK_DUMP_FOPEN

void dump_str(gchar const *str, gchar const *prefix);
void dump_ustr(Glib::ustring const &ustr);


/*######################
## N E W
######################*/

/**
 * Create a blank document and add it to the desktop
 * Input: empty string or template file name.
 */
SPDesktop *sp_file_new(const std::string &templ)
{
    auto *app = InkscapeApplication::instance();

    SPDocument* doc = app->document_new (templ);
    if (!doc) {
        std::cerr << "sp_file_new: failed to open document: " << templ << std::endl;
    }
    InkscapeWindow* win = app->window_open (doc);

    SPDesktop* desktop = win->get_desktop();

#ifdef WITH_DBUS
    Inkscape::Extension::Dbus::dbus_init_desktop_interface(desktop);
#endif

    return desktop;
}

std::string sp_file_default_template_uri()
{
    return Inkscape::IO::Resource::get_filename_string(TEMPLATES, "default.svg", true);
}

SPDesktop* sp_file_new_default()
{
    SPDesktop* desk = sp_file_new(sp_file_default_template_uri());
    //rdf_add_from_preferences( SP_ACTIVE_DOCUMENT );

    return desk;
}


/*######################
## D E L E T E
######################*/

/**
 *  Perform document closures preceding an exit()
 */
void sp_file_exit()
{
    if (SP_ACTIVE_DESKTOP == nullptr) {
        // We must be in console mode
        auto app = Gio::Application::get_default();
        g_assert(app);
        app->quit();
    } else {
        sp_ui_close_all();
        // no need to call inkscape_exit here; last document being closed will take care of that
    }
}


/**
 *  Handle prompting user for "do you want to revert"?  Revert on "OK"
 */
void sp_file_revert_dialog()
{
    SPDesktop  *desktop = SP_ACTIVE_DESKTOP;
    g_assert(desktop != nullptr);

    SPDocument *doc = desktop->getDocument();
    g_assert(doc != nullptr);

    Inkscape::XML::Node *repr = doc->getReprRoot();
    g_assert(repr != nullptr);

    gchar const *uri = doc->getDocumentURI();
    if (!uri) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved yet.  Cannot revert."));
        return;
    }

    bool do_revert = true;
    if (doc->isModifiedSinceSave()) {
        Glib::ustring tmpString = Glib::ustring::compose(_("Changes will be lost! Are you sure you want to reload document %1?"), uri);
        bool response = desktop->warnDialog (tmpString);
        if (!response) {
            do_revert = false;
        }
    }

    bool reverted = false;
    if (do_revert) {
        auto *app = InkscapeApplication::instance();
        reverted = app->document_revert (doc);
    }

    if (reverted) {
        desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Document reverted."));
    } else {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not reverted."));
    }
}

void dump_str(gchar const *str, gchar const *prefix)
{
    Glib::ustring tmp;
    tmp = prefix;
    tmp += " [";
    size_t const total = strlen(str);
    for (unsigned i = 0; i < total; i++) {
        gchar *const tmp2 = g_strdup_printf(" %02x", (0x0ff & str[i]));
        tmp += tmp2;
        g_free(tmp2);
    }

    tmp += "]";
    g_message("%s", tmp.c_str());
}

void dump_ustr(Glib::ustring const &ustr)
{
    char const *cstr = ustr.c_str();
    char const *data = ustr.data();
    Glib::ustring::size_type const byteLen = ustr.bytes();
    Glib::ustring::size_type const dataLen = ustr.length();
    Glib::ustring::size_type const cstrLen = strlen(cstr);

    g_message("   size: %lu\n   length: %lu\n   bytes: %lu\n    clen: %lu",
              gulong(ustr.size()), gulong(dataLen), gulong(byteLen), gulong(cstrLen) );
    g_message( "  ASCII? %s", (ustr.is_ascii() ? "yes":"no") );
    g_message( "  UTF-8? %s", (ustr.validate() ? "yes":"no") );

    try {
        Glib::ustring tmp;
        for (Glib::ustring::size_type i = 0; i < ustr.bytes(); i++) {
            tmp = "    ";
            if (i < dataLen) {
                Glib::ustring::value_type val = ustr.at(i);
                gchar* tmp2 = g_strdup_printf( (((val & 0xff00) == 0) ? "  %02x" : "%04x"), val );
                tmp += tmp2;
                g_free( tmp2 );
            } else {
                tmp += "    ";
            }

            if (i < byteLen) {
                int val = (0x0ff & data[i]);
                gchar *tmp2 = g_strdup_printf("    %02x", val);
                tmp += tmp2;
                g_free( tmp2 );
                if ( val > 32 && val < 127 ) {
                    tmp2 = g_strdup_printf( "   '%c'", (gchar)val );
                    tmp += tmp2;
                    g_free( tmp2 );
                } else {
                    tmp += "    . ";
                }
            } else {
                tmp += "       ";
            }

            if ( i < cstrLen ) {
                int val = (0x0ff & cstr[i]);
                gchar* tmp2 = g_strdup_printf("    %02x", val);
                tmp += tmp2;
                g_free(tmp2);
                if ( val > 32 && val < 127 ) {
                    tmp2 = g_strdup_printf("   '%c'", (gchar) val);
                    tmp += tmp2;
                    g_free( tmp2 );
                } else {
                    tmp += "    . ";
                }
            } else {
                tmp += "            ";
            }

            g_message( "%s", tmp.c_str() );
        }
    } catch (...) {
        g_message("XXXXXXXXXXXXXXXXXX Exception" );
    }
    g_message("---------------");
}

/**
 *  Display an file Open selector.  Open a document if OK is pressed.
 *  Can select single or multiple files for opening.
 */
void
sp_file_open_dialog(Gtk::Window &parentWindow, gpointer /*object*/, gpointer /*data*/)
{
    //# Get the current directory for finding files
    static Glib::ustring open_path;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if(open_path.empty())
    {
        Glib::ustring attr = prefs->getString("/dialogs/open/path");
        if (!attr.empty()) open_path = attr;
    }

    //# Test if the open_path directory exists
    if (!Inkscape::IO::file_test(open_path.c_str(),
              (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        open_path = "";

#ifdef _WIN32
    //# If no open path, default to our win32 documents folder
    if (open_path.empty())
    {
        // The path to the My Documents folder is read from the
        // value "HKEY_CURRENT_USER\Software\Windows\CurrentVersion\Explorer\Shell Folders\Personal"
        HKEY key = NULL;
        if(RegOpenKeyExA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
            0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
        {
            WCHAR utf16path[_MAX_PATH];
            DWORD value_type;
            DWORD data_size = sizeof(utf16path);
            if(RegQueryValueExW(key, L"Personal", NULL, &value_type,
                (BYTE*)utf16path, &data_size) == ERROR_SUCCESS)
            {
                g_assert(value_type == REG_SZ);
                gchar *utf8path = g_utf16_to_utf8(
                    (const gunichar2*)utf16path, -1, NULL, NULL, NULL);
                if(utf8path)
                {
                    open_path = Glib::ustring(utf8path);
                    g_free(utf8path);
                }
            }
        }
    }
#endif

    //# If no open path, default to our home directory
    if (open_path.empty())
    {
        open_path = g_get_home_dir();
        open_path.append(G_DIR_SEPARATOR_S);
    }

    //# Create a dialog
    Inkscape::UI::Dialog::FileOpenDialog *openDialogInstance =
              Inkscape::UI::Dialog::FileOpenDialog::create(
                 parentWindow, open_path,
                 Inkscape::UI::Dialog::SVG_TYPES,
                 _("Select file to open"));

    //# Show the dialog
    bool const success = openDialogInstance->show();

    //# Save the folder the user selected for later
    open_path = openDialogInstance->getCurrentDirectory();

    if (!success)
    {
        delete openDialogInstance;
        return;
    }

    // FIXME: This is silly to have separate code paths for opening one vs many files!

    //# User selected something.  Get name and type
    Glib::ustring fileName = openDialogInstance->getFilename();

    // Inkscape::Extension::Extension *fileType =
    //         openDialogInstance->getSelectionType();

    //# Code to check & open if multiple files.
    std::vector<Glib::ustring> flist = openDialogInstance->getFilenames();

    //# We no longer need the file dialog object - delete it
    delete openDialogInstance;
    openDialogInstance = nullptr;

    auto *app = InkscapeApplication::instance();

    //# Iterate through filenames if more than 1
    if (flist.size() > 1)
    {
        for (const auto & i : flist)
        {
            fileName = i;

            Glib::ustring newFileName = Glib::filename_to_utf8(fileName);
            if ( newFileName.size() > 0 )
                fileName = newFileName;
            else
                g_warning( "ERROR CONVERTING OPEN FILENAME TO UTF-8" );

#ifdef INK_DUMP_FILENAME_CONV
            g_message("Opening File %s\n", fileName.c_str());
#endif

            Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(fileName);
            app->create_window (file);
        }

        return;
    }


    if (!fileName.empty())
    {
        Glib::ustring newFileName = Glib::filename_to_utf8(fileName);

        if ( newFileName.size() > 0)
            fileName = newFileName;
        else
            g_warning( "ERROR CONVERTING OPEN FILENAME TO UTF-8" );

        open_path = Glib::path_get_dirname (fileName);
        open_path.append(G_DIR_SEPARATOR_S);
        prefs->setString("/dialogs/open/path", open_path);

        Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(fileName);
        app->create_window (file);
    }

    return;
}


/*######################
## V A C U U M
######################*/

/**
 * Remove unreferenced defs from the defs section of the document.
 */
void sp_file_vacuum(SPDocument *doc)
{
    unsigned int diff = doc->vacuumDocument();

    DocumentUndo::done(doc, SP_VERB_FILE_VACUUM,
                       _("Clean up document"));

    SPDesktop *dt = SP_ACTIVE_DESKTOP;
    if (dt != nullptr) {
        // Show status messages when in GUI mode
        if (diff > 0) {
            dt->messageStack()->flashF(Inkscape::NORMAL_MESSAGE,
                    ngettext("Removed <b>%i</b> unused definition in &lt;defs&gt;.",
                            "Removed <b>%i</b> unused definitions in &lt;defs&gt;.",
                            diff),
                    diff);
        } else {
            dt->messageStack()->flash(Inkscape::NORMAL_MESSAGE,  _("No unused definitions in &lt;defs&gt;."));
        }
    }
}



/*######################
## S A V E
######################*/

/**
 * This 'save' function called by the others below
 *
 * \param    official  whether to set :output_module and :modified in the
 *                     document; is true for normal save, false for temporary saves
 */
static bool
file_save(Gtk::Window &parentWindow, SPDocument *doc, const Glib::ustring &uri,
          Inkscape::Extension::Extension *key, bool checkoverwrite, bool official,
          Inkscape::Extension::FileSaveMethod save_method)
{
    if (!doc || uri.size()<1) //Safety check
        return false;

    Inkscape::Version save = doc->getRoot()->version.inkscape;
    doc->getReprRoot()->setAttribute("inkscape:version", Inkscape::version_string);
    try {
        Inkscape::Extension::save(key, doc, uri.c_str(),
                                  false,
                                  checkoverwrite, official,
                                  save_method);
    } catch (Inkscape::Extension::Output::no_extension_found &e) {
        gchar *safeUri = Inkscape::IO::sanitizeString(uri.c_str());
        gchar *text = g_strdup_printf(_("No Inkscape extension found to save document (%s).  This may have been caused by an unknown filename extension."), safeUri);
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        sp_ui_error_dialog(text);
        g_free(text);
        g_free(safeUri);
        // Restore Inkscape version
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    } catch (Inkscape::Extension::Output::file_read_only &e) {
        gchar *safeUri = Inkscape::IO::sanitizeString(uri.c_str());
        gchar *text = g_strdup_printf(_("File %s is write protected. Please remove write protection and try again."), safeUri);
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        sp_ui_error_dialog(text);
        g_free(text);
        g_free(safeUri);
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    } catch (Inkscape::Extension::Output::save_failed &e) {
        gchar *safeUri = Inkscape::IO::sanitizeString(uri.c_str());
        gchar *text = g_strdup_printf(_("File %s could not be saved."), safeUri);
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        sp_ui_error_dialog(text);
        g_free(text);
        g_free(safeUri);
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    } catch (Inkscape::Extension::Output::save_cancelled &e) {
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    } catch (Inkscape::Extension::Output::export_id_not_found &e) {
        gchar *text = g_strdup_printf(_("File could not be saved:\nNo object with ID '%s' found."), e.id);
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        sp_ui_error_dialog(text);
        g_free(text);
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    } catch (Inkscape::Extension::Output::no_overwrite &e) {
        return sp_file_save_dialog(parentWindow, doc, save_method);
    } catch (std::exception &e) {
        gchar *safeUri = Inkscape::IO::sanitizeString(uri.c_str());
        gchar *text = g_strdup_printf(_("File %s could not be saved.\n\n"
                                        "The following additional information was returned by the output extension:\n"
                                        "'%s'"), safeUri, e.what());
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        sp_ui_error_dialog(text);
        g_free(text);
        g_free(safeUri);
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    } catch (...) {
        g_critical("Extension '%s' threw an unspecified exception.", key->get_id());
        gchar *safeUri = Inkscape::IO::sanitizeString(uri.c_str());
        gchar *text = g_strdup_printf(_("File %s could not be saved."), safeUri);
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        sp_ui_error_dialog(text);
        g_free(text);
        g_free(safeUri);
        doc->getReprRoot()->setAttribute("inkscape:version", sp_version_to_string( save ));
        return false;
    }

    if (SP_ACTIVE_DESKTOP) {
        if (! SP_ACTIVE_DESKTOP->event_log) {
            g_message("file_save: ->event_log == NULL. please report to bug #967416");
        }
        if (! SP_ACTIVE_DESKTOP->messageStack()) {
            g_message("file_save: ->messageStack() == NULL. please report to bug #967416");
        }
    } else {
        g_message("file_save: SP_ACTIVE_DESKTOP == NULL. please report to bug #967416");
    }

    SP_ACTIVE_DESKTOP->event_log->rememberFileSave();
    Glib::ustring msg;
    if (doc->getDocumentURI() == nullptr) {
        msg = Glib::ustring::format(_("Document saved."));
    } else {
        msg = Glib::ustring::format(_("Document saved."), " ", doc->getDocumentURI());
    }
    SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::NORMAL_MESSAGE, msg.c_str());
    return true;
}


/**
 * Check if a string ends with another string.
 * \todo Find a better code file to put this general purpose method
 */
static bool hasEnding (Glib::ustring const &fullString, Glib::ustring const &ending)
{
    if (fullString.length() > ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

/**
 *  Display a SaveAs dialog.  Save the document if OK pressed.
 */
bool
sp_file_save_dialog(Gtk::Window &parentWindow, SPDocument *doc, Inkscape::Extension::FileSaveMethod save_method)
{
    Inkscape::Extension::Output *extension = nullptr;
    bool is_copy = (save_method == Inkscape::Extension::FILE_SAVE_METHOD_SAVE_COPY);

    // Note: default_extension has the format "org.inkscape.output.svg.inkscape", whereas
    //       filename_extension only uses ".svg"
    Glib::ustring default_extension;
    Glib::ustring filename_extension = ".svg";

    default_extension= Inkscape::Extension::get_file_save_extension(save_method);
    //g_message("%s: extension name: '%s'", __FUNCTION__, default_extension);

    extension = dynamic_cast<Inkscape::Extension::Output *>
        (Inkscape::Extension::db.get(default_extension.c_str()));

    if (extension)
        filename_extension = extension->get_extension();

    Glib::ustring save_path = Inkscape::Extension::get_file_save_path(doc, save_method);

    if (!Inkscape::IO::file_test(save_path.c_str(),
          (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        save_path.clear();

    if (save_path.empty())
        save_path = g_get_home_dir();

    Glib::ustring save_loc = save_path;
    save_loc.append(G_DIR_SEPARATOR_S);

    int i = 1;
    if ( !doc->getDocumentURI() ) {
        // We are saving for the first time; create a unique default filename
        save_loc = save_loc + _("drawing") + filename_extension;

        while (Inkscape::IO::file_test(save_loc.c_str(), G_FILE_TEST_EXISTS)) {
            save_loc = save_path;
            save_loc.append(G_DIR_SEPARATOR_S);
            save_loc = save_loc + Glib::ustring::compose(_("drawing-%1"), i++) + filename_extension;
        }
    } else {
        save_loc.append(Glib::path_get_basename(doc->getDocumentURI()));
    }

    // convert save_loc from utf-8 to locale
    // is this needed any more, now that everything is handled in
    // Inkscape::IO?
    Glib::ustring save_loc_local = Glib::filename_from_utf8(save_loc);

    if (!save_loc_local.empty())
        save_loc = save_loc_local;

    //# Show the SaveAs dialog
    char const * dialog_title;
    if (is_copy) {
        dialog_title = (char const *) _("Select file to save a copy to");
    } else {
        dialog_title = (char const *) _("Select file to save to");
    }
    gchar* doc_title = doc->getRoot()->title();
    Inkscape::UI::Dialog::FileSaveDialog *saveDialog =
        Inkscape::UI::Dialog::FileSaveDialog::create(
            parentWindow,
            save_loc,
            Inkscape::UI::Dialog::SVG_TYPES,
            dialog_title,
            default_extension,
            doc_title ? doc_title : "",
            save_method
            );

    saveDialog->setSelectionType(extension);

    bool success = saveDialog->show();
    if (!success) {
        delete saveDialog;
        if(doc_title) g_free(doc_title);
        return success;
    }

    // set new title here (call RDF to ensure metadata and title element are updated)
    rdf_set_work_entity(doc, rdf_find_entity("title"), saveDialog->getDocTitle().c_str());

    Glib::ustring fileName = saveDialog->getFilename();
    Inkscape::Extension::Extension *selectionType = saveDialog->getSelectionType();

    delete saveDialog;
    saveDialog = nullptr;
    if(doc_title) g_free(doc_title);

    if (!fileName.empty()) {
        Glib::ustring newFileName = Glib::filename_to_utf8(fileName);

        if (!newFileName.empty())
            fileName = newFileName;
        else
            g_warning( "Error converting filename for saving to UTF-8." );

        Inkscape::Extension::Output *omod = dynamic_cast<Inkscape::Extension::Output *>(selectionType);
        if (omod) {
            Glib::ustring save_extension = (omod->get_extension()) ? (omod->get_extension()) : "";
            if ( !hasEnding(fileName, save_extension) ) {
                fileName += save_extension;
            }
        }

        // FIXME: does the argument !is_copy really convey the correct meaning here?
        success = file_save(parentWindow, doc, fileName, selectionType, TRUE, !is_copy, save_method);

        if (success && doc->getDocumentURI()) {
            // getDocumentURI does not return an actual URI... it is an UTF-8 encoded filename (!)
            std::string filename = Glib::filename_from_utf8(doc->getDocumentURI());
            Glib::ustring uri = Glib::filename_to_uri(filename);

            Glib::RefPtr<Gtk::RecentManager> recent = Gtk::RecentManager::get_default();
            recent->add_item(uri);
        }

        save_path = Glib::path_get_dirname(fileName);
        Inkscape::Extension::store_save_path_in_prefs(save_path, save_method);

        return success;
    }


    return false;
}


/**
 * Save a document, displaying a SaveAs dialog if necessary.
 */
bool
sp_file_save_document(Gtk::Window &parentWindow, SPDocument *doc)
{
    bool success = true;

    if (doc->isModifiedSinceSave()) {
        if ( doc->getDocumentURI() == nullptr )
        {
            // Hier sollte in Argument mitgegeben werden, das anzeigt, da� das Dokument das erste
            // Mal gespeichert wird, so da� als default .svg ausgew�hlt wird und nicht die zuletzt
            // benutzte "Save as ..."-Endung
            return sp_file_save_dialog(parentWindow, doc, Inkscape::Extension::FILE_SAVE_METHOD_INKSCAPE_SVG);
        } else {
            Glib::ustring extension = Inkscape::Extension::get_file_save_extension(Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS);
            Glib::ustring fn = g_strdup(doc->getDocumentURI());
            // Try to determine the extension from the uri; this may not lead to a valid extension,
            // but this case is caught in the file_save method below (or rather in Extension::save()
            // further down the line).
            Glib::ustring ext = "";
            Glib::ustring::size_type pos = fn.rfind('.');
            if (pos != Glib::ustring::npos) {
                // FIXME: this could/should be more sophisticated (see FileSaveDialog::appendExtension()),
                // but hopefully it's a reasonable workaround for now
                ext = fn.substr( pos );
            }
            success = file_save(parentWindow, doc, fn, Inkscape::Extension::db.get(ext.c_str()), FALSE, TRUE, Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS);
            if (success == false) {
                // give the user the chance to change filename or extension
                return sp_file_save_dialog(parentWindow, doc, Inkscape::Extension::FILE_SAVE_METHOD_INKSCAPE_SVG);
            }
        }
    } else {
        Glib::ustring msg;
        if ( doc->getDocumentURI() == nullptr )
        {
            msg = Glib::ustring::format(_("No changes need to be saved."));
        } else {
            msg = Glib::ustring::format(_("No changes need to be saved."), " ", doc->getDocumentURI());
        }
        SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::WARNING_MESSAGE, msg.c_str());
        success = TRUE;
    }

    return success;
}


/**
 * Save a document.
 */
bool
sp_file_save(Gtk::Window &parentWindow, gpointer /*object*/, gpointer /*data*/)
{
    if (!SP_ACTIVE_DOCUMENT)
        return false;

    SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::IMMEDIATE_MESSAGE, _("Saving document..."));

    sp_namedview_document_from_window(SP_ACTIVE_DESKTOP);
    return sp_file_save_document(parentWindow, SP_ACTIVE_DOCUMENT);
}


/**
 *  Save a document, always displaying the SaveAs dialog.
 */
bool
sp_file_save_as(Gtk::Window &parentWindow, gpointer /*object*/, gpointer /*data*/)
{
    if (!SP_ACTIVE_DOCUMENT)
        return false;
    sp_namedview_document_from_window(SP_ACTIVE_DESKTOP);
    return sp_file_save_dialog(parentWindow, SP_ACTIVE_DOCUMENT, Inkscape::Extension::FILE_SAVE_METHOD_SAVE_AS);
}



/**
 *  Save a copy of a document, always displaying a sort of SaveAs dialog.
 */
bool
sp_file_save_a_copy(Gtk::Window &parentWindow, gpointer /*object*/, gpointer /*data*/)
{
    if (!SP_ACTIVE_DOCUMENT)
        return false;
    sp_namedview_document_from_window(SP_ACTIVE_DESKTOP);
    return sp_file_save_dialog(parentWindow, SP_ACTIVE_DOCUMENT, Inkscape::Extension::FILE_SAVE_METHOD_SAVE_COPY);
}

/**
 *  Save a copy of a document as template.
 */
bool
sp_file_save_template(Gtk::Window &parentWindow, Glib::ustring name,
    Glib::ustring author, Glib::ustring description, Glib::ustring keywords,
    bool isDefault)
{
    if (!SP_ACTIVE_DOCUMENT || name.length() == 0)
        return true;

    auto document = SP_ACTIVE_DOCUMENT;

    DocumentUndo::setUndoSensitive(document, false);

    auto root = document->getReprRoot();
    auto xml_doc = document->getReprDoc();

    auto templateinfo_node = xml_doc->createElement("inkscape:templateinfo");
    Inkscape::GC::release(templateinfo_node);

    auto element_node = xml_doc->createElement("inkscape:name");
    Inkscape::GC::release(element_node);

    element_node->appendChild(xml_doc->createTextNode(name.c_str()));
    templateinfo_node->appendChild(element_node);

    if (author.length() != 0) {

        element_node = xml_doc->createElement("inkscape:author");
        Inkscape::GC::release(element_node);

        element_node->appendChild(xml_doc->createTextNode(author.c_str()));
        templateinfo_node->appendChild(element_node);
    }

    if (description.length() != 0) {

        element_node = xml_doc->createElement("inkscape:shortdesc");
        Inkscape::GC::release(element_node);

        element_node->appendChild(xml_doc->createTextNode(description.c_str()));
        templateinfo_node->appendChild(element_node);

    }

    element_node = xml_doc->createElement("inkscape:date");
    Inkscape::GC::release(element_node);

    element_node->appendChild(xml_doc->createTextNode(
        Glib::DateTime::create_now_local().format("%F").c_str()));
    templateinfo_node->appendChild(element_node);

    if (keywords.length() != 0) {

        element_node = xml_doc->createElement("inkscape:keywords");
        Inkscape::GC::release(element_node);

        element_node->appendChild(xml_doc->createTextNode(keywords.c_str()));
        templateinfo_node->appendChild(element_node);

    }

    root->appendChild(templateinfo_node);

    // Escape filenames for windows users, but filenames are not URIs so
    // Allow UTF-8 and don't escape spaces witch are popular chars.
    auto encodedName = Glib::uri_escape_string(name, " ", true);
    encodedName.append(".svg");

    auto filename = Inkscape::IO::Resource::get_path_ustring(USER, TEMPLATES, encodedName.c_str());

    auto operation_confirmed = sp_ui_overwrite_file(filename.c_str());

    if (operation_confirmed) {

        file_save(parentWindow, document, filename,
            Inkscape::Extension::db.get(".svg"), false, false,
            Inkscape::Extension::FILE_SAVE_METHOD_INKSCAPE_SVG);

        if (isDefault) {
            // save as "default.svg" by default (so it works independently of UI language), unless
            // a localized template like "default.de.svg" is already present (which overrides "default.svg")
            Glib::ustring default_svg_localized = Glib::ustring("default.") + _("en") + ".svg";
            filename = Inkscape::IO::Resource::get_path_ustring(USER, TEMPLATES, default_svg_localized.c_str());

            if (!Inkscape::IO::file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
                filename = Inkscape::IO::Resource::get_path_ustring(USER, TEMPLATES, "default.svg");
            }

            file_save(parentWindow, document, filename,
                Inkscape::Extension::db.get(".svg"), false, false,
                Inkscape::Extension::FILE_SAVE_METHOD_INKSCAPE_SVG);
        }
    }
    
    // remove this node from current document after saving it as template
    root->removeChild(templateinfo_node);

    DocumentUndo::setUndoSensitive(document, true);

    return operation_confirmed;
}



/*######################
## I M P O R T
######################*/

/**
 * Paste the contents of a document into the active desktop.
 * @param clipdoc The document to paste
 * @param in_place Whether to paste the selection where it was when copied
 * @pre @c clipdoc is not empty and items can be added to the current layer
 */
void sp_import_document(SPDesktop *desktop, SPDocument *clipdoc, bool in_place)
{
    //TODO: merge with file_import()

    SPDocument *target_document = desktop->getDocument();
    Inkscape::XML::Node *root = clipdoc->getReprRoot();
    Inkscape::XML::Node *target_parent = desktop->currentLayer()->getRepr();

    // copy definitions
    desktop->doc()->importDefs(clipdoc);

    Inkscape::XML::Node* clipboard = nullptr;
    // copy objects
    std::vector<Inkscape::XML::Node*> pasted_objects;
    for (Inkscape::XML::Node *obj = root->firstChild() ; obj ; obj = obj->next()) {
        // Don't copy metadata, defs, named views and internal clipboard contents to the document
        if (!strcmp(obj->name(), "svg:defs")) {
            continue;
        }
        if (!strcmp(obj->name(), "svg:metadata")) {
            continue;
        }
        if (!strcmp(obj->name(), "sodipodi:namedview")) {
            continue;
        }
        if (!strcmp(obj->name(), "inkscape:clipboard")) {
            clipboard = obj;
            continue;
        }
        Inkscape::XML::Node *obj_copy = obj->duplicate(target_document->getReprDoc());
        target_parent->appendChild(obj_copy);
        Inkscape::GC::release(obj_copy);

        pasted_objects.push_back(obj_copy);

        // if we are pasting a clone to an already existing object, its
        // transform is relative to the document, not to its original (see ui/clipboard.cpp)
        SPUse *use = dynamic_cast<SPUse *>(obj_copy);
        if (use) {
            SPItem *original = use->get_original();
            if (original) {
                Geom::Affine relative_use_transform = original->transform.inverse() * use->transform;
                obj_copy->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(relative_use_transform));
            }
        }
    }

    std::vector<Inkscape::XML::Node*> pasted_objects_not;
    if(clipboard) //???? Removed dead code can cause any bug, need to reimplement undead
    for (Inkscape::XML::Node *obj = clipboard->firstChild() ; obj ; obj = obj->next()) {
        if(target_document->getObjectById(obj->attribute("id"))) continue;
        Inkscape::XML::Node *obj_copy = obj->duplicate(target_document->getReprDoc());
        SPObject * pasted = desktop->currentLayer()->appendChildRepr(obj_copy);
        Inkscape::GC::release(obj_copy);
        SPLPEItem * pasted_lpe_item = dynamic_cast<SPLPEItem *>(pasted);
        if (pasted_lpe_item){
            pasted_lpe_item->forkPathEffectsIfNecessary(1);
        }
        pasted_objects_not.push_back(obj_copy);
    }
    Inkscape::Selection *selection = desktop->getSelection();
    selection->setReprList(pasted_objects_not);
    Geom::Affine doc2parent = SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
    selection->applyAffine(desktop->dt2doc() * doc2parent * desktop->doc2dt(), true, false, false);
    selection->deleteItems();

    // Change the selection to the freshly pasted objects
    selection->setReprList(pasted_objects);
    for (auto item : selection->items()) {
        SPLPEItem *pasted_lpe_item = dynamic_cast<SPLPEItem *>(item);
        if (pasted_lpe_item) {
            pasted_lpe_item->forkPathEffectsIfNecessary(1);
        }
    }
    // Apply inverse of parent transform
    selection->applyAffine(desktop->dt2doc() * doc2parent * desktop->doc2dt(), true, false, false);



    // Update (among other things) all curves in paths, for bounds() to work
    target_document->ensureUpToDate();

    // move selection either to original position (in_place) or to mouse pointer
    Geom::OptRect sel_bbox = selection->visualBounds();
    if (sel_bbox) {
        // get offset of selection to original position of copied elements
        Geom::Point pos_original;
        Inkscape::XML::Node *clipnode = sp_repr_lookup_name(root, "inkscape:clipboard", 1);
        if (clipnode) {
            Geom::Point min, max;
            sp_repr_get_point(clipnode, "min", &min);
            sp_repr_get_point(clipnode, "max", &max);
            pos_original = Geom::Point(min[Geom::X], max[Geom::Y]);
        }
        Geom::Point offset = pos_original - sel_bbox->corner(3);

        if (!in_place) {
            SnapManager &m = desktop->namedview->snap_manager;
            m.setup(desktop);
            sp_event_context_discard_delayed_snap_event(desktop->event_context);

            // get offset from mouse pointer to bbox center, snap to grid if enabled
            Geom::Point mouse_offset = desktop->point() - sel_bbox->midpoint();
            offset = m.multipleOfGridPitch(mouse_offset - offset, sel_bbox->midpoint() + offset) + offset;
            m.unSetup();
        }

        selection->moveRelative(offset);
    }
    target_document->emitReconstructionFinish();
}


/**
 *  Import a resource.  Called by sp_file_import()
 */
SPObject *
file_import(SPDocument *in_doc, const Glib::ustring &uri,
               Inkscape::Extension::Extension *key)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    bool cancelled = false;

    // store mouse pointer location before opening any dialogs, so we can drop the item where initially intended
    auto pointer_location = desktop->point();

    //DEBUG_MESSAGE( fileImport, "file_import( in_doc:%p uri:[%s], key:%p", in_doc, uri, key );
    SPDocument *doc;
    try {
        doc = Inkscape::Extension::open(key, uri.c_str());
    } catch (Inkscape::Extension::Input::no_extension_found &e) {
        doc = nullptr;
    } catch (Inkscape::Extension::Input::open_failed &e) {
        doc = nullptr;
    } catch (Inkscape::Extension::Input::open_cancelled &e) {
        doc = nullptr;
        cancelled = true;
    }

    if (doc != nullptr) {
        Inkscape::XML::rebase_hrefs(doc, in_doc->getDocumentBase(), false);
        Inkscape::XML::Document *xml_in_doc = in_doc->getReprDoc();

        prevent_id_clashes(doc, in_doc);

        SPCSSAttr *style = sp_css_attr_from_object(doc->getRoot());

        // Count the number of top-level items in the imported document.
        guint items_count = 0;
        SPObject *o = nullptr;
        for (auto& child: doc->getRoot()->children) {
            if (SP_IS_ITEM(&child)) {
                items_count++;
                o = &child;
            }
        }

        //ungroup if necessary
        bool did_ungroup = false;
        while(items_count==1 && o && SP_IS_GROUP(o) && o->children.size()==1){
            std::vector<SPItem *>v;
            sp_item_group_ungroup(SP_GROUP(o),v,false);
            o = v.empty() ? nullptr : v[0];
            did_ungroup=true;
        }

        // Create a new group if necessary.
        Inkscape::XML::Node *newgroup = nullptr;
        const auto & al = style->attributeList();
        if ((style && !al.empty()) || items_count > 1) {
            newgroup = xml_in_doc->createElement("svg:g");
            sp_repr_css_set(newgroup, style, "style");
        }

        // Determine the place to insert the new object.
        // This will be the current layer, if possible.
        // FIXME: If there's no desktop (command line run?) we need
        //        a document:: method to return the current layer.
        //        For now, we just use the root in this case.
        SPObject *place_to_insert;
        if (desktop) {
            place_to_insert = desktop->currentLayer();
        } else {
            place_to_insert = in_doc->getRoot();
        }

        in_doc->importDefs(doc);

        // Construct a new object representing the imported image,
        // and insert it into the current document.
        SPObject *new_obj = nullptr;
        for (auto& child: doc->getRoot()->children) {
            if (SP_IS_ITEM(&child)) {
                Inkscape::XML::Node *newitem = did_ungroup ? o->getRepr()->duplicate(xml_in_doc) : child.getRepr()->duplicate(xml_in_doc);

                // convert layers to groups, and make sure they are unlocked
                // FIXME: add "preserve layers" mode where each layer from
                //        import is copied to the same-named layer in host
                newitem->removeAttribute("inkscape:groupmode");
                newitem->removeAttribute("sodipodi:insensitive");

                if (newgroup) newgroup->appendChild(newitem);
                else new_obj = place_to_insert->appendChildRepr(newitem);
            }

            // don't lose top-level defs or style elements
            else if (child.getRepr()->type() == Inkscape::XML::NodeType::ELEMENT_NODE) {
                const gchar *tag = child.getRepr()->name();
                if (!strcmp(tag, "svg:style")) {
                    in_doc->getRoot()->appendChildRepr(child.getRepr()->duplicate(xml_in_doc));
                }
            }
        }
        in_doc->emitReconstructionFinish();
        if (newgroup) new_obj = place_to_insert->appendChildRepr(newgroup);

        // release some stuff
        if (newgroup) Inkscape::GC::release(newgroup);
        if (style) sp_repr_css_attr_unref(style);

        // select and move the imported item
        if (new_obj && SP_IS_ITEM(new_obj)) {
            Inkscape::Selection *selection = desktop->getSelection();
            selection->set(SP_ITEM(new_obj));

            // preserve parent and viewBox transformations
            // c2p is identity matrix at this point unless ensureUpToDate is called
            doc->ensureUpToDate();
            Geom::Affine affine = doc->getRoot()->c2p * SP_ITEM(place_to_insert)->i2doc_affine().inverse();
            selection->applyAffine(desktop->dt2doc() * affine * desktop->doc2dt(), true, false, false);

            // move to mouse pointer
            {
                desktop->getDocument()->ensureUpToDate();
                Geom::OptRect sel_bbox = selection->visualBounds();
                if (sel_bbox) {
                    Geom::Point m( pointer_location - sel_bbox->midpoint() );
                    selection->moveRelative(m, false);
                }
            }
        }

        DocumentUndo::done(in_doc, SP_VERB_FILE_IMPORT,
                           _("Import"));
        return new_obj;
    } else if (!cancelled) {
        gchar *text = g_strdup_printf(_("Failed to load the requested file %s"), uri.c_str());
        sp_ui_error_dialog(text);
        g_free(text);
    }

    return nullptr;
}


/**
 *  Display an Open dialog, import a resource if OK pressed.
 */
void
sp_file_import(Gtk::Window &parentWindow)
{
    static Glib::ustring import_path;

    SPDocument *doc = SP_ACTIVE_DOCUMENT;
    if (!doc)
        return;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if(import_path.empty())
    {
        Glib::ustring attr = prefs->getString("/dialogs/import/path");
        if (!attr.empty()) import_path = attr;
    }

    //# Test if the import_path directory exists
    if (!Inkscape::IO::file_test(import_path.c_str(),
              (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        import_path = "";

    //# If no open path, default to our home directory
    if (import_path.empty())
    {
        import_path = g_get_home_dir();
        import_path.append(G_DIR_SEPARATOR_S);
    }

    // Create new dialog (don't use an old one, because parentWindow has probably changed)
    Inkscape::UI::Dialog::FileOpenDialog *importDialogInstance =
             Inkscape::UI::Dialog::FileOpenDialog::create(
                 parentWindow,
                 import_path,
                 Inkscape::UI::Dialog::IMPORT_TYPES,
                 (char const *)_("Select file to import"));

    bool success = importDialogInstance->show();
    if (!success) {
        delete importDialogInstance;
        return;
    }

    typedef std::vector<Glib::ustring> pathnames;
    pathnames flist(importDialogInstance->getFilenames());

    // Get file name and extension type
    Glib::ustring fileName = importDialogInstance->getFilename();
    Inkscape::Extension::Extension *selection = importDialogInstance->getSelectionType();

    delete importDialogInstance;
    importDialogInstance = nullptr;

    //# Iterate through filenames if more than 1
    if (flist.size() > 1)
    {
        for (const auto & i : flist)
        {
            fileName = i;

            Glib::ustring newFileName = Glib::filename_to_utf8(fileName);
            if (!newFileName.empty())
                fileName = newFileName;
            else
                g_warning("ERROR CONVERTING IMPORT FILENAME TO UTF-8");

#ifdef INK_DUMP_FILENAME_CONV
            g_message("Importing File %s\n", fileName.c_str());
#endif
            file_import(doc, fileName, selection);
        }

        return;
    }


    if (!fileName.empty()) {

        Glib::ustring newFileName = Glib::filename_to_utf8(fileName);

        if (!newFileName.empty())
            fileName = newFileName;
        else
            g_warning("ERROR CONVERTING IMPORT FILENAME TO UTF-8");

        import_path = Glib::path_get_dirname(fileName);
        import_path.append(G_DIR_SEPARATOR_S);
        prefs->setString("/dialogs/import/path", import_path);

        file_import(doc, fileName, selection);
    }

    return;
}

/*######################
## P R I N T
######################*/


/**
 *  Print the current document, if any.
 */
void
sp_file_print(Gtk::Window& parentWindow)
{
    SPDocument *doc = SP_ACTIVE_DOCUMENT;
    if (doc)
        sp_print_document(parentWindow, doc);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
