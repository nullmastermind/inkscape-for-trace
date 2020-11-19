// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * The main Inkscape application.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>
#include <iomanip>
#include <cerrno>  // History file
#include <regex>

#include <glibmm/i18n.h>  // Internationalization

#ifdef HAVE_CONFIG_H
# include "config.h"      // Defines ENABLE_NLS
#endif

#include "inkscape-application.h"
#include "inkscape-window.h"

#include "auto-save.h"            // Auto-save
#include "desktop.h"              // Access to window
#include "file.h"                 // sp_file_convert_dpi
#include "inkscape.h"             // Inkscape::Application

#include "include/glibmm_version.h"

#include "inkgc/gc-core.h"        // Garbage Collecting init
#include "debug/logger.h"         // INKSCAPE_DEBUG_LOG support

#include "io/file.h"              // File open (command line).
#include "io/resource.h"          // TEMPLATE
#include "io/resource-manager.h"  // Fix up references.

#include "object/sp-root.h"       // Inkscape version.

#include "ui/interface.h"         // sp_ui_error_dialog
#include "ui/dialog/font-substitution.h"  // Warn user about font substitution.
#include "ui/shortcuts.h"         // Shortcuts... init
#include "widgets/desktop-widget.h" // Close without saving dialog

#include "util/units.h"           // Redimension window

#include "actions/actions-base.h"         // Actions
#include "actions/actions-file.h"         // Actions
#include "actions/actions-object.h"       // Actions
#include "actions/actions-object-align.h" // Actions
#include "actions/actions-output.h"       // Actions
#include "actions/actions-selection.h"    // Actions
#include "actions/actions-transform.h"    // Actions
#include "actions/actions-window.h"       // Actions

#ifdef GDK_WINDOWING_QUARTZ
#include <gtkosxapplication.h>
#endif

#ifdef WITH_DBUS
# include "extension/dbus/dbus-init.h"
#endif

#ifdef ENABLE_NLS
// Native Language Support - shouldn't this always be used?
#include "helper/gettext.h"   // gettext init
#endif // ENABLE_NLS

#ifdef WITH_GNU_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "io/resource.h"
using Inkscape::IO::Resource::UIS;

// This is a bit confusing as there are two ways to handle command line arguments and files
// depending on if the Gio::APPLICATION_HANDLES_OPEN and/or Gio::APPLICATION_HANDLES_COMMAND_LINE
// flags are set. If the open flag is set and the command line not, the all the remainng arguments
// after calling on_handle_local_options() are assumed to be filenames.

// Add document to app.
void
InkscapeApplication::document_add(SPDocument* document)
{
    if (document) {
        auto it = _documents.find(document);
        if (it == _documents.end()) {
            _documents[document] = std::vector<InkscapeWindow*>();
        } else {
            // Should never happen.
            std::cerr << "InkscapeApplication::add_document: Document already opened!" << std::endl;
        }
    } else {
        // Should never happen!
        std::cerr << "InkscapeApplication::add_document: No document!" << std::endl;
    }
}

// New document, add it to app. TODO: This should really be open_document with option to strip template data.
SPDocument*
InkscapeApplication::document_new(const std::string &Template)
{
    // Open file
    SPDocument *document = ink_file_new(Template);
    if (document) {
        document_add(document);

        // Set viewBox if it doesn't exist.
        if (!document->getRoot()->viewBox_set) {
            document->setViewBox();
        }

    } else {
        std::cerr << "InkscapeApplication::new_document: failed to open new document!" << std::endl;
    }

    return document;
}


// Open a document, add it to app.
SPDocument*
InkscapeApplication::document_open(const Glib::RefPtr<Gio::File>& file, bool *cancelled)
{
    // Open file
    SPDocument *document = ink_file_open(file, cancelled);

    if (document) {
        document->setVirgin(false); // Prevents replacing document in same window during file open.

        document_add (document);
    } else if (cancelled == nullptr || !(*cancelled)) {
        std::cerr << "InkscapeApplication::document_open: Failed to open: " << file->get_parse_name() << std::endl;
    }

    return document;
}


// Open a document, add it to app.
SPDocument*
InkscapeApplication::document_open(const std::string& data)
{
    // Open file
    SPDocument *document = ink_file_open(data);

    if (document) {
        document->setVirgin(false); // Prevents replacing document in same window during file open.

        document_add (document);
    } else {
        std::cerr << "InkscapeApplication::document_open: Failed to open memory document." << std::endl;
    }

    return document;
}


/** Swap out one document for another in a window... maybe this should disappear.
 *  Does not delete old document!
 */
bool
InkscapeApplication::document_swap(InkscapeWindow* window, SPDocument* document)
{
    if (!document || !window) {
        std::cerr << "InkscapeAppliation::swap_document: Missing window or document!" << std::endl;
        return false;
    }

    SPDesktop* desktop = window->get_desktop();
    SPDocument* old_document = window->get_document();
    desktop->change_document(document);
    document->emitResizedSignal(document->getWidth().value("px"), document->getHeight().value("px"));

    // We need to move window from the old document to the new document.

    // Find old document
    auto it = _documents.find(old_document);
    if (it != _documents.end()) {

        // Remove window from document map.
        auto it2 = std::find(it->second.begin(), it->second.end(), window);
        if (it2 != it->second.end()) {
            it->second.erase(it2);
        } else {
            std::cerr << "InkscapeApplication::swap_document: Window not found!" << std::endl;
        }

    } else {
        std::cerr << "InkscapeApplication::swap_document: Document not in map!" << std::endl;
    }

    // Find new document
    it = _documents.find(document);
    if (it != _documents.end()) {
        it->second.push_back(window);
    } else {
        std::cerr << "InkscapeApplication::swap_document: Document not in map!" << std::endl;
    }

    // To be removed (add/delete once per window)!
    INKSCAPE.add_document(document);
    INKSCAPE.remove_document(old_document);

    // ActionContext should be removed once verbs are gone but we use it for now.
    Inkscape::ActionContext context = INKSCAPE.action_context_for_document(document);
    _active_document  = document;
    _active_selection = context.getSelection();
    _active_view      = context.getView();
    _active_window    = window;
    return true;
}

/** Revert document: open saved document and swap it for each window.
 */
bool
InkscapeApplication::document_revert(SPDocument* document)
{
    // Find saved document.
    gchar const *path = document->getDocumentURI();
    if (!path) {
        std::cerr << "InkscapeApplication::revert_document: Document never saved, cannot revert." << std::endl;
        return false;
    }

    // Open saved document.
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(document->getDocumentURI());
    SPDocument* new_document = document_open (file);
    if (!new_document) {
        std::cerr << "InkscapeApplication::revert_document: Cannot open saved document!" << std::endl;
        return false;
    }

    // Allow overwriting current document.
    document->setVirgin(true);

    auto it = _documents.find(document);
    if (it != _documents.end()) {

        // Swap reverted document in all windows.
        for (auto it2 : it->second) {

            SPDesktop* desktop = it2->get_desktop();

            // Remember current zoom and view.
            double zoom = desktop->current_zoom();
            Geom::Point c = desktop->current_center();

            bool reverted = document_swap (it2, new_document);

            if (reverted) {
                desktop->zoom_absolute(c, zoom, false);
            } else {
                std::cerr << "InkscapeApplication::revert_document: Revert failed!" << std::endl;
            }
        }

        document_close (document);
    } else {
        std::cerr << "InkscapeApplication::revert_document: Document not found!" << std::endl;
        return false;
    }
    return true;
}



/** Close a document, remove from app. No checking is done on modified status, etc.
 */
void
InkscapeApplication::document_close(SPDocument* document)
{
    if (document) {

        auto it = _documents.find(document);
        if (it != _documents.end()) {
            if (it->second.size() != 0) {
                std::cerr << "InkscapeApplication::close_document: Window vector not empty!" << std::endl;
            }
            _documents.erase(it);
        } else {
            std::cerr << "InkscapeApplication::close_document: Document not registered with application." << std::endl;
        }

        delete document;

    } else {
        std::cerr << "InkscapeApplication::close_document: No document!" << std::endl;
    }
}


/** Return number of windows with document.
 */
unsigned
InkscapeApplication::document_window_count(SPDocument* document)
{
    unsigned count = 0;

    auto it = _documents.find(document);
    if (it != _documents.end()) {
        count = it->second.size();
    } else {
        std::cerr << "InkscapeApplication::document_window_count: Document not in map!" << std::endl;
    }

    return count;
}

/** Fix up a document if necessary (Only fixes that require GUI).
 */
void
InkscapeApplication::document_fix(InkscapeWindow* window)
{
    // Most fixes are handled when document is opened in SPDocument::createDoc().
    // But some require the GUI to be present. These are handled here.

    if (_with_gui) {

        SPDocument* document = window->get_document();

        // Perform a fixup pass for hrefs.
        if ( Inkscape::ResourceManager::getManager().fixupBrokenLinks(document) ) {
            Glib::ustring msg = _("Broken links have been changed to point to existing files.");
            SPDesktop* desktop = window->get_desktop();
            if (desktop != nullptr) {
                desktop->showInfoDialog(msg);
            }
        }

        // Fix dpi (pre-92 files).
        if ( sp_version_inside_range( document->getRoot()->version.inkscape, 0, 1, 0, 92 ) ) {
            sp_file_convert_dpi(document);
        }

        // Check for font substitutions, requires text to have been rendered.
        Inkscape::UI::Dialog::FontSubstitution::getInstance().checkFontSubstitutions(document);
    }
}


/** Get a list of open documents (from document map).
 */
std::vector<SPDocument*>
InkscapeApplication::get_documents()
{
    std::vector<SPDocument*> documents;
    for (auto &i : _documents) {
        documents.push_back(i.first);
    }
    return documents;
}



// Take an already open document and create a new window, adding window to document map.
InkscapeWindow*
InkscapeApplication::window_open(SPDocument* document)
{
    // Once we've removed Inkscape::Application (separating GUI from non-GUI stuff)
    // it will be more easy to start up the GUI after-the-fact. Until then, prevent
    // opening a window if GUI not selected at start-up time.
    if (!_with_gui) {
        std::cerr << "InkscapeApplication::window_open: Not in gui mode!" << std::endl;
        return nullptr;
    }

    InkscapeWindow* window = new InkscapeWindow(document);
    // TODO Add window to application. (Instead of in InkscapeWindow constructor.)

    // To be removed (add once per window)!
    INKSCAPE.add_document(document);

    // ActionContext should be removed once verbs are gone but we use it for now.
    Inkscape::ActionContext context = INKSCAPE.action_context_for_document(document);
    _active_selection = context.getSelection();
    _active_view      = context.getView();
    _active_document  = document;
    _active_window    = window;

    auto it = _documents.find(document);
    if (it != _documents.end()) {
        it->second.push_back(window);
    } else {
        std::cerr << "InkscapeApplication::window_open: Document not in map!" << std::endl;
    }

    document_fix(window); // May need flag to prevent this from being called more than once.

    return window;
}


// Close a window. Does not delete document.
void
InkscapeApplication::window_close(InkscapeWindow* window)
{
    // std::cout << "InkscapeApplication::close_window" << std::endl;
    // dump();

    if (window) {

        SPDocument* document = window->get_document();
        if (document) {

            // To be removed (remove once per window)!
            /* bool last = */ INKSCAPE.remove_document(document);

            // Leave active document alone (maybe should find new active window and reset variables).
            _active_selection = nullptr;
            _active_view      = nullptr;
            _active_window    = nullptr;

            // Remove window from document map.
            auto it = _documents.find(document);
            if (it != _documents.end()) {
                auto it2 = std::find(it->second.begin(), it->second.end(), window);
                if (it2 != it->second.end()) {
                    it->second.erase(it2);
                    delete window; // Results in call to SPDesktop::destroy()
                } else {
                    std::cerr << "InkscapeApplication::close_window: window not found!" << std::endl;
                }
            } else {
                std::cerr << "InkscapeApplication::close_window: document not in map!" << std::endl;
            }
        } else {
            std::cerr << "InkscapeApplication::close_window: No document!" << std::endl;
        }

    } else {
        std::cerr << "InkscapeApplication::close_window: No window!" << std::endl;
    }

    // dump();
}


// Closes active window (useful for scripting).
void
InkscapeApplication::window_close_active()
{
    if (_active_window) {
        window_close (_active_window);
    } else {
        std::cerr << "InkscapeApplication::window_close_active: no active window!" << std::endl;
    }
}


/** Update windows in response to:
 *  - New active window
 *  - Document change
 *  - Selection change
 */
void
InkscapeApplication::windows_update(SPDocument* document)
{
    // Find windows:
    auto it = _documents.find( document );
    if (it != _documents.end()) {
        std::vector<InkscapeWindow*> windows = it->second;
        // std::cout << "InkscapeApplication::update_windows: windows size: " << windows.size() << std::endl;
        // Loop over InkscapeWindows.
        // Loop over DialogWindows. TBD
    } else {
        // std::cout << "InkscapeApplication::update_windows: no windows found" << std::endl;
    }
}

/** Debug function
 */
void
InkscapeApplication::dump()
{
    std::cout << "InkscapeApplication::dump()" << std::endl;
    std::cout << "  Documents: " << _documents.size() << std::endl;
    for (auto i : _documents) {
        std::cout << "    Document: " << (i.first->getDocumentName()?i.first->getDocumentName():"unnamed") << std::endl;
        for (auto j : i.second) {
            std::cout << "      Window: " << j->get_title() << std::endl;
        }
    }
}

static InkscapeApplication *_instance = nullptr;

InkscapeApplication &InkscapeApplication::singleton()
{
    if (!_instance) {
        _instance = new InkscapeApplication();
    }
    return *_instance;
}

InkscapeApplication *InkscapeApplication::instance()
{
    return _instance;
}

void
InkscapeApplication::_start_main_option_section(const Glib::ustring& section_name)
{
#ifndef _WIN32
    // Avoid outputting control characters to non-tty destinations.
    //
    // However, isatty() is not useful on Windows
    //   - it doesn't recognize mintty and similar terminals
    //   - it doesn't work in cmd.exe either, where we have to use the inkscape.com wrapper, connecting stdout to a pipe
    if (!isatty(fileno(stdout))) {
        return;
    }
#endif

    auto *gapp = gio_app();

    if (section_name.empty()) {
        gapp->add_main_option_entry(Gio::Application::OPTION_TYPE_BOOL, Glib::ustring("\b\b  "));
    } else {
        gapp->add_main_option_entry(Gio::Application::OPTION_TYPE_BOOL, Glib::ustring("\b\b  \n") + section_name + ":");
    }
}

// Note: We tried using Gio::APPLICATION_CAN_OVERRIDE_APP_ID instead of
// Gio::APPLICATION_NON_UNIQUE. The advantages of this is that copy/paste between windows would be
// more reliable and that we wouldn't have multiple Inkscape instance writing to the preferences
// file at the same time (if started as separate processes). This caused problems with batch
// processing files and with extensions as they rely on having multiple instances of Inkscape
// running independently. In principle one can use --gapplication-app-id to run a new instance of
// Inkscape but this with our current structure fails with the error message:
// "g_application_set_application_id: assertion '!application->priv->is_registered' failed".
// It also require generating new id's for each separate Inkscape instance required.

InkscapeApplication::InkscapeApplication()
{
    using T = Gio::Application;

    auto app_id = "org.inkscape.Inkscape";
    auto flags = Gio::APPLICATION_HANDLES_OPEN | // Use default file opening.
                 Gio::APPLICATION_NON_UNIQUE;

    if (gtk_init_check(nullptr, nullptr)) {
        g_set_prgname(app_id);
        _gio_application = Gtk::Application::create(app_id, flags);
    } else {
        _gio_application = Gio::Application::create(app_id, flags);
        _with_gui = false;
    }

    auto *gapp = gio_app();

    gapp->signal_activate().connect([this]() { this->on_activate(); });
    gapp->signal_open().connect(sigc::mem_fun(*this, &InkscapeApplication::on_open));

    // ==================== Initializations =====================
    // Garbage Collector
    Inkscape::GC::init();

#ifndef NDEBUG
    // Use environment variable INKSCAPE_DEBUG_LOG=log.txt for event logging
    Inkscape::Debug::Logger::init();
#endif

#ifdef ENABLE_NLS
    // Native Language Support (shouldn't this always be used?).
    Inkscape::initialize_gettext();
#endif

    // Autosave
    Inkscape::AutoSave::getInstance().init(this);

    // Don't set application name for now. We don't use it anywhere but
    // it overrides the name used for adding recently opened files and breaks the Gtk::RecentFilter
    // Glib::set_application_name(N_("Inkscape - A Vector Drawing Program"));  // After gettext() init.

    // ======================== Actions =========================
    add_actions_base(this);         // actions that are GUI independent
    add_actions_file(this);         // actions for file handling
    add_actions_object(this);       // actions for object manipulation
    add_actions_object_align(this); // actions for object alignment
    add_actions_output(this);       // actions for file export
    add_actions_selection(this);    // actions for object selection
    add_actions_transform(this);    // actions for transforming selected objects
    add_actions_window(this);       // actions for windows


    // ====================== Command Line ======================

    // Will automatically handle character conversions.
    // Note: OPTION_TYPE_FILENAME => std::string, OPTION_TYPE_STRING => Glib::ustring.

#if GLIBMM_CHECK_VERSION(2,56,0)
    // Additional informational strings for --help output
    // TODO: Claims to be translated automatically, but seems broken, so pass already translated strings
    gapp->set_option_context_parameter_string(_("file1 [file2 [fileN]]"));
    gapp->set_option_context_summary(_("Process (or open) one or more files."));
    gapp->set_option_context_description(Glib::ustring("\n") + _("Examples:") + '\n'
            + "  " + Glib::ustring::compose(_("Export input SVG (%1) to PDF (%2) format:"), "in.svg", "out.pdf") + '\n'
            + '\t' + "inkscape --export-filename=out.pdf in.svg\n"
            + "  " + Glib::ustring::compose(_("Export input files (%1) to PNG format keeping original name (%2):"), "in1.svg, in2.svg", "in1.png, in2.png") + '\n'
            + '\t' + "inkscape --export-type=png in1.svg in2.svg\n"
            + "  " + Glib::ustring::compose(_("See %1 and %2 for more details."), "'man inkscape'", "http://wiki.inkscape.org/wiki/index.php/Using_the_Command_Line"));
#endif

    // clang-format off
    // General
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "version",                 'V', N_("Print Inkscape version"),                                                  "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "system-data-directory",  '\0', N_("Print system data directory"),                                             "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "user-data-directory",    '\0', N_("Print user data directory"),                                               "");

    // Open/Import
    _start_main_option_section(_("File import"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "pipe",                    'p', N_("Read input file from standard input (stdin)"),                             "");
    gapp->add_main_option_entry(T::OPTION_TYPE_INT,      "pdf-page",               '\0', N_("PDF page number to import"),                                       N_("PAGE"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "pdf-poppler",            '\0', N_("Use poppler when importing via commandline"),                              "");
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "convert-dpi-method",     '\0', N_("Method used to convert pre-0.92 document dpi, if needed: [none|scale-viewbox|scale-document]"), N_("METHOD"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "no-convert-text-baseline-spacing", '\0', N_("Do not fix pre-0.92 document's text baseline spacing on opening"), "");

    // Export - File and File Type
    _start_main_option_section(_("File export"));
    gapp->add_main_option_entry(T::OPTION_TYPE_FILENAME, "export-filename",        'o', N_("Output file name (defaults to input filename; file type is guessed from extension if present; use '-' to write to stdout)"), N_("FILENAME"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-overwrite",      '\0', N_("Overwrite input file (otherwise add '_out' suffix if type doesn't change"), "");
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-type",           '\0', N_("File type(s) to export: [svg,png,ps,eps,pdf,emf,wmf,xaml]"), N_("TYPE[,TYPE]*"));
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-extension",      '\0', N_("Extension ID to use for exporting"),                         N_("EXTENSION-ID"));

    // Export - Geometry
    _start_main_option_section(_("Export geometry"));                                                                                                                        // B = PNG, S = SVG, P = PS/EPS/PDF
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-area-page",       'C', N_("Area to export is page"),                                                   ""); // BSP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-area-drawing",    'D', N_("Area to export is whole drawing (ignoring page size)"),                     ""); // BSP
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-area",            'a', N_("Area to export in SVG user units"),                          N_("x0:y0:x1:y1")); // BSP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-area-snap",      '\0', N_("Snap the bitmap export area outwards to the nearest integer values"),       ""); // Bxx
    gapp->add_main_option_entry(T::OPTION_TYPE_DOUBLE,   "export-dpi",             'd', N_("Resolution for bitmaps and rasterized filters; default is 96"),      N_("DPI")); // BxP
    gapp->add_main_option_entry(T::OPTION_TYPE_INT,      "export-width",           'w', N_("Bitmap width in pixels (overrides --export-dpi)"),                 N_("WIDTH")); // Bxx
    gapp->add_main_option_entry(T::OPTION_TYPE_INT,      "export-height",          'h', N_("Bitmap height in pixels (overrides --export-dpi)"),               N_("HEIGHT")); // Bxx
    gapp->add_main_option_entry(T::OPTION_TYPE_INT,      "export-margin",         '\0', N_("Margin around export area: units of page size for SVG, mm for PS/PDF"), N_("MARGIN")); // xSP

    // Export - Options
    _start_main_option_section(_("Export options"));
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-id",              'i', N_("ID(s) of object(s) to export"),                   N_("OBJECT-ID[;OBJECT-ID]*")); // BSP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-id-only",         'j', N_("Hide all objects except object with ID selected by export-id"),             ""); // BSx
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-plain-svg",       'l', N_("Remove Inkscape-specific SVG attributes/properties"),                       ""); // xSx
    gapp->add_main_option_entry(T::OPTION_TYPE_INT,      "export-ps-level",       '\0', N_("Postscript level (2 or 3); default is 3"),                         N_("LEVEL")); // xxP
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-pdf-version",    '\0', N_("PDF version (1.4 or 1.5); default is 1.5"),                      N_("VERSION")); // xxP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-text-to-path",    'T', N_("Convert text to paths (PS/EPS/PDF/SVG)"),                                   ""); // xxP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-latex",          '\0', N_("Export text separately to LaTeX file (PS/EPS/PDF)"),                        ""); // xxP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-ignore-filters", '\0', N_("Render objects without filters instead of rasterizing (PS/EPS/PDF)"),       ""); // xxP
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "export-use-hints",       't', N_("Use stored filename and DPI hints when exporting object selected by --export-id"), ""); // Bxx
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-background",      'b', N_("Background color for exported bitmaps (any SVG color string)"),         N_("COLOR")); // Bxx
    gapp->add_main_option_entry(T::OPTION_TYPE_DOUBLE,   "export-background-opacity", 'y', N_("Background opacity for exported bitmaps (0.0 to 1.0, or 1 to 255)"), N_("VALUE")); // Bxx
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "export-png-color-mode", '\0', N_("Color mode (bit depth and color type) for exported bitmaps (Gray_1/Gray_2/Gray_4/Gray_8/Gray_16/RGB_8/RGB_16/GrayAlpha_8/GrayAlpha_16/RGBA_8/RGBA_16)"), N_("COLOR-MODE")); // Bxx

    // Query - Geometry
    _start_main_option_section(_("Query object/document geometry"));
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "query-id",               'I', N_("ID(s) of object(s) to be queried"),              N_("OBJECT-ID[,OBJECT-ID]*"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "query-all",              'S', N_("Print bounding boxes of all objects"),                                     "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "query-x",                'X', N_("X coordinate of drawing or object (if specified by --query-id)"),          "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "query-y",                'Y', N_("Y coordinate of drawing or object (if specified by --query-id)"),          "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "query-width",            'W', N_("Width of drawing or object (if specified by --query-id)"),                 "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "query-height",           'H', N_("Height of drawing or object (if specified by --query-id)"),                "");

    // Processing
    _start_main_option_section(_("Advanced file processing"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "vacuum-defs",           '\0', N_("Remove unused definitions from the <defs> section(s) of document"),        "");
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "select",                '\0', N_("Select objects: comma-separated list of IDs"),   N_("OBJECT-ID[,OBJECT-ID]*"));

    // Actions
    _start_main_option_section();
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "actions",                'a', N_("List of actions (with optional arguments) to execute"),     N_("ACTION(:ARG)[;ACTION(:ARG)]*"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "action-list",           '\0', N_("List all available actions"),                                               "");

    // Verbs
    _start_main_option_section();
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "verb",                  '\0', N_("List of verbs to execute"),                                 N_("VERB[;VERB]*"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "verb-list",             '\0', N_("List all available verbs"),                                                 "");

    // Interface
    _start_main_option_section(_("Interface"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "with-gui",               'g', N_("With graphical user interface (required by some actions/verbs)"),           "");
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "batch-process",         '\0', N_("Close GUI after executing all actions/verbs"),"");
    _start_main_option_section();
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "shell",                 '\0', N_("Start Inkscape in interactive shell mode"),                                 "");

#ifdef WITH_DBUS
    _start_main_option_section(_("D-Bus"));
    gapp->add_main_option_entry(T::OPTION_TYPE_BOOL,     "dbus-listen",           '\0', N_("Enter a listening loop for D-Bus messages in console mode"),                "");
    gapp->add_main_option_entry(T::OPTION_TYPE_STRING,   "dbus-name",             '\0', N_("Specify the D-Bus name; default is 'org.inkscape'"),            N_("BUS-NAME"));
#endif // WITH_DBUS
    // clang-format on

    gapp->signal_handle_local_options().connect(sigc::mem_fun(*this, &InkscapeApplication::on_handle_local_options));

    // This is normally called for us... but after the "handle_local_options" signal is emitted. If
    // we want to rely on actions for handling options, we need to call it here. This appears to
    // have no unwanted side-effect. It will also trigger the call to on_startup().
    gapp->register_application();
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean osx_openfile_callback(GtkosxApplication *, gchar const *, InkscapeApplication *);
static gboolean osx_quit_callback(GtkosxApplication *, InkscapeApplication *);
#endif

void
InkscapeApplication::on_startup2()
{
    // This should be completely rewritten.
    Inkscape::Application::create(_with_gui);

    if (!_with_gui) {
        return;
    }

    auto *gapp = gio_app();

    // ======================= Actions (GUI) ======================
    gapp->add_action("new",    sigc::mem_fun(*this, &InkscapeApplication::on_new   ));
    gapp->add_action("quit",   sigc::mem_fun(*this, &InkscapeApplication::on_quit  ));

    // ========================= GUI Init =========================
    Gtk::Window::set_default_icon_name("org.inkscape.Inkscape");

    // Shortcuts
    // For verbs, shortcuts need to be setup before GUI elements are created! After verbs are gone,
    // this can be removed. Initialization will then happen on first call to Shortcuts::getInstance()
    // in Inkscape::Window constructor which will allow verification that actions exist
    // before shortcuts are added.
    // Shortcuts for actions can be set before the actions are created.
    Inkscape::Shortcuts::getInstance().init();

#ifdef GDK_WINDOWING_QUARTZ
    GtkosxApplication *osxapp = gtkosx_application_get();
    g_signal_connect(G_OBJECT(osxapp), "NSApplicationOpenFile", G_CALLBACK(osx_openfile_callback), this);
    g_signal_connect(G_OBJECT(osxapp), "NSApplicationBlockTermination", G_CALLBACK(osx_quit_callback), this);
#endif
}

/** Create a window given a document. This is used internally in InkscapeApplication.
 */
InkscapeWindow*
InkscapeApplication::create_window(SPDocument *document, bool replace)
{
    if (!gtk_app()) {
        g_assert_not_reached();
        return nullptr;
    }

    SPDocument *old_document = _active_document;
    InkscapeWindow* window = InkscapeApplication::get_active_window();

    if (replace && old_document && window) {
        document_swap (window, document);

        // Delete old document if no longer attached to any window.
        auto it = _documents.find (old_document);
        if (it != _documents.end()) {
            if (it->second.size() == 0) {
                document_close (old_document);
            }
        }

        document->emitResizedSignal(document->getWidth().value("px"), document->getHeight().value("px"));

    } else {
        window = window_open (document);
    }
    window->show();

    return window;
}


/** Create a window given a Gio::File. This is what most external functions should call.
  *
  * @param file - The filename to open as a Gio::File object
*/
void
InkscapeApplication::create_window(const Glib::RefPtr<Gio::File>& file)
{
    if (!gtk_app()) {
        g_assert_not_reached();
        return;
    }

    SPDocument* document = nullptr;
    InkscapeWindow* window = nullptr;
    bool cancelled = false;

    if (file) {
        document = document_open(file, &cancelled);
        if (document) {
            // Remember document so much that we'll add it to recent documents
            auto recentmanager = Gtk::RecentManager::get_default();
            recentmanager->add_item (file->get_uri());

            SPDocument* old_document = _active_document;
            bool replace = old_document && old_document->getVirgin();

            window = create_window (document, replace);

        } else if (!cancelled) {
            std::cerr << "ConcreteInkscapeApplication<T>::create_window: Failed to load: "
                      << file->get_parse_name() << std::endl;

            gchar *text = g_strdup_printf(_("Failed to load the requested file %s"), file->get_parse_name().c_str());
            sp_ui_error_dialog(text);
            g_free(text);
        }

    } else {
        std::string Template =
            Inkscape::IO::Resource::get_filename(Inkscape::IO::Resource::TEMPLATES, "default.svg", true);
        document = document_new (Template);
        if (document) {
            window = window_open (document);
        } else {
            std::cerr << "ConcreteInkscapeApplication<T>::create_window: Failed to open default template! " << Template << std::endl;
        }
    }

    _active_document = document;
    _active_window   = window;

#ifdef WITH_DBUS
    if (window) {
        SPDesktop* desktop = window->get_desktop();
        if (desktop) {
            Inkscape::Extension::Dbus::dbus_init_desktop_interface(desktop);
        } else {
            std::cerr << "ConcreteInkscapeApplication<T>::create_window: Failed to create desktop!" << std::endl;
        }
    }
#endif
}

/** Destroy a window. Aborts if document needs saving.
 *  Returns true if window destroyed.
 */
bool
InkscapeApplication::destroy_window(InkscapeWindow* window)
{
    if (!gtk_app()) {
        g_assert_not_reached();
        return false;
    }

    SPDocument* document = window->get_document();

    // Remove document if no windows left.
    if (document) {
        auto it = _documents.find(document);
        if (it != _documents.end()) {

            // If only one window for document:
            if (it->second.size() == 1) {
                // Check if document needs saving.
                bool abort = window->get_desktop_widget()->shutdown();
                if (abort) {
                    return false;
                }
            }

            window_close(window);

            if (it->second.size() == 0) {
                document_close (document);
            }

        } else {
            std::cerr << "ConcreteInkscapeApplication<Gtk::Application>::destroy_window: Could not find document!" << std::endl;
        }
    }

    // Debug
    // auto windows = get_windows();
    // std::cout << "destroy_windows: app windows size: " << windows.size() << std::endl;

    return true;
}

void
InkscapeApplication::destroy_all()
{
    if (!gtk_app()) {
        g_assert_not_reached();
        return;
    }

    while (_documents.size() != 0) {
        auto it = _documents.begin();
        if (!it->second.empty()) {
            auto it2 = it->second.begin();
            if (!destroy_window (*it2)) return; // If destroy aborted, we need to stop exit.
        }
    }
}

/** Common processing for documents
 */
void
InkscapeApplication::process_document(SPDocument* document, std::string output_path)
{
    // Add to Inkscape::Application...
    INKSCAPE.add_document(document);

    // Are we doing one file at a time? In that case, we don't recreate new windows for each file.
    bool replace = _use_pipe || _batch_process;

    // Open window if needed (reuse window if we are doing one file at a time inorder to save overhead).
    if (_with_gui) {
        _active_window = create_window(document, replace);
    }

    // ActionContext should be removed once verbs are gone but we use it for now.
    Inkscape::ActionContext context = INKSCAPE.action_context_for_document(document);
    _active_document  = document;
    _active_selection = context.getSelection();
    _active_view      = context.getView();

    document->ensureUpToDate(); // Or queries don't work!

    // process_file
    for (auto action: _command_line_actions) {
        if (!_gio_application->has_action(action.first)) {
            std::cerr << "ConcreteInkscapeApplication<T>::process_document: Unknown action name: " <<  action.first << std::endl;
        }
        _gio_application->activate_action( action.first, action.second );
    }

    if (_use_shell) {
        shell();
    }

    // Only if --export-filename, --export-type --export-overwrite, or --export-use-hints are used.
    if (_auto_export) {
        // Save... can't use action yet.
        _file_export.do_export(document, output_path);
    }
}


// Open document window with default document or pipe. Either this or on_open() is called.
void
InkscapeApplication::on_activate()
{
    on_startup2();

    std::string output;

    // Create new document, either from pipe or from template.
    SPDocument *document = nullptr;

    if (_use_pipe) {

        // Create document from pipe in.
        std::istreambuf_iterator<char> begin(std::cin), end;
        std::string s(begin, end);
        document = document_open (s);
        output = "-";

    } else {

        // Create a blank document from template
        std::string Template =
            Inkscape::IO::Resource::get_filename(Inkscape::IO::Resource::TEMPLATES, "default.svg", true);
        document = document_new (Template);
    }

    if (!document) {
        std::cerr << "ConcreteInksacpeApplication::on_activate: failed to created document!" << std::endl;
        return;
    }

    // Process document (command line actions, shell, create window)
    process_document (document, output);
}

// Open document window for each file. Either this or on_activate() is called.
// type_vec_files == std::vector<Glib::RefPtr<Gio::File> >
void
InkscapeApplication::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint)
{
    on_startup2();
    if(_pdf_poppler)
        INKSCAPE.set_pdf_poppler(_pdf_poppler);
    if(_pdf_page)
        INKSCAPE.set_pdf_page(_pdf_page);

    if (files.size() > 1 && !_file_export.export_filename.empty()) {
        std::cerr << "ConcreteInkscapeApplication<Gtk::Application>::on_open: "
                     "Can't use '--export-filename' with multiple input files "
                     "(output file would be overwritten for each input file). "
                     "Please use '--export-type' instead and rename manually."
                  << std::endl;
        return;
    }

    for (auto file : files) {

        // Open file
        SPDocument *document = document_open (file);
        if (!document) {
            std::cerr << "ConcreteInkscapeApplication::on_open: failed to create document!" << std::endl;
            continue;
        }

        // Process document (command line actions, shell, create window)
        process_document (document, file->get_path());
    }

    if (_batch_process) {
        // If with_gui, we've reused a window for each file. We must quit to destroy it.
        gio_app()->quit();
    }
}

void
InkscapeApplication::parse_actions(const Glib::ustring& input, action_vector_t& action_vector)
{
    const auto re_colon = Glib::Regex::create("\\s*:\\s*");

    // Split action list
    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple("\\s*;\\s*", input);
    for (auto token : tokens) {
        // Note: split into 2 tokens max ("param:value"); allows value to contain colon (e.g. abs. paths on Windows)
        std::vector<Glib::ustring> tokens2 = re_colon->split(token, 0,  static_cast<Glib::RegexMatchFlags>(0), 2);
        std::string action;
        std::string value;
        if (tokens2.size() > 0) {
            action = tokens2[0];
        }
        if (tokens2.size() > 1) {
            value = tokens2[1];
        }

        Glib::RefPtr<Gio::Action> action_ptr = _gio_application->lookup_action(action);
        if (action_ptr) {
            // Doesn't seem to be a way to test this using the C++ binding without Glib-CRITICAL errors.
            const  GVariantType* gtype = g_action_get_parameter_type(action_ptr->gobj());
            if (gtype) {
                // With value.
                Glib::VariantType type = action_ptr->get_parameter_type();
                if (type.get_string() == "b") {
                    bool b = false;
                    if (value == "1" || value == "true" || value.empty()) {
                        b = true;
                    } else if (value == "0" || value == "false") {
                        b = false;
                    } else {
                        std::cerr << "InkscapeApplication::parse_actions: Invalid boolean value: " << action << ":" << value << std::endl;
                    }
                    action_vector.push_back(
                        std::make_pair( action, Glib::Variant<bool>::create(b)));
                } else if (type.get_string() == "i") {
                    action_vector.push_back(
                        std::make_pair( action, Glib::Variant<int>::create(std::stoi(value))));
                } else if (type.get_string() == "d") {
                    action_vector.push_back(
                        std::make_pair( action, Glib::Variant<double>::create(std::stod(value))));
                } else if (type.get_string() == "s") {
                    action_vector.push_back(
                        std::make_pair( action, Glib::Variant<Glib::ustring>::create(value) ));
                } else {
                    std::cerr << "InkscapeApplication::parse_actions: unhandled action value: "
                              << action << ": " << type.get_string() << std::endl;
                }
            } else {
                // Stateless (i.e. no value).
                action_vector.push_back( std::make_pair( action, Glib::VariantBase() ) );
            }
        } else {
            // Assume a verb
            // std::cerr << "InkscapeApplication::parse_actions: '"
            //           << action << "' is not a valid action! Assuming verb!" << std::endl;
            action_vector.push_back(
                std::make_pair("verb", Glib::Variant<Glib::ustring>::create(action)));
        }
    }
}

#ifdef WITH_GNU_READLINE

// For use in shell mode. Command completion of action names.
char* readline_generator (const char* text, int state)
{
    static std::vector<Glib::ustring> actions;

    // Fill the vector of action names.
    if (actions.size() == 0) {
        auto *app = InkscapeApplication::instance();
        actions = app->gio_app()->list_actions();
        std::sort(actions.begin(), actions.end());
    }

    static int list_index = 0;
    static int len = 0;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    const char* name = nullptr;
    while (list_index < actions.size()) {
        name = actions[list_index].c_str();
        list_index++;
        if (strncmp (name, text, len) == 0) {
            return (strdup(name));
        }
    }

    return ((char*)nullptr);
}

char** readline_completion(const char* text, int start, int end)
{
    char **matches = (char**)nullptr;

    // Match actions names, but only at start of line.
    // It would be nice to also match action names after a ';' but it's not possible as text won't include ';'.
    if (start == 0) {
        matches = rl_completion_matches (text, readline_generator);
    }

    return (matches);
}

void readline_init()
{
    rl_readline_name = "inkscape";
    rl_attempted_completion_function = readline_completion;
}
#endif // WITH_GNU_READLINE


// Once we don't need to create a window just to process verbs!
void
InkscapeApplication::shell()
{
    std::cout << "Inkscape interactive shell mode. Type 'action-list' to list all actions. "
              << "Type 'quit' to quit." << std::endl;
    std::cout << " Input of the form:" << std::endl;
    std::cout << " action1:arg1; action2;arg2; verb1; verb2; ..." << std::endl;
    if (!_with_gui) {
        std::cout << "Only verbs that don't require a desktop may be used." << std::endl;
    }

#ifdef WITH_GNU_READLINE
    auto history_file = Glib::build_filename(Inkscape::IO::Resource::profile_path(), "shell.history");

#ifdef _WIN32
    gchar *locale_filename = g_win32_locale_filename_from_utf8(history_file.c_str());
    if (locale_filename) {
        history_file = locale_filename;
        g_free(locale_filename);
    }
#endif

    static bool init = false;
    if (!init) {
        readline_init();
        using_history();
        init = true;

        int error = read_history(history_file.c_str());
        if (error && error != ENOENT) {
            std::cerr << "read_history error: " << std::strerror(error) << " " << history_file << std::endl;
        }
    }
#endif

    while (std::cin.good()) {
        bool eof = false;
        std::string input;

#ifdef WITH_GNU_READLINE
        char *readline_input = readline("> ");
        if (readline_input) {
            input = readline_input;
            if (input != "quit" && input != "q") {
                add_history(readline_input);
            }
        } else {
            eof = true;
        }
        free(readline_input);
#else
        std::cout << "> ";
        std::getline(std::cin, input);
#endif

        // Remove trailing space
        input = std::regex_replace(input, std::regex(" +$"), "");

        if (eof || input == "quit" || input == "q") {
            break;
        }

        action_vector_t action_vector;
        parse_actions(input, action_vector);
        for (auto action: action_vector) {
            _gio_application->activate_action( action.first, action.second );
        }

        // This would allow displaying the results of actions on the fly... but it needs to be well
        // vetted first.
        Glib::RefPtr<Glib::MainContext> context = Glib::MainContext::get_default();
        while (context->iteration(false)) {};
    }

#ifdef WITH_GNU_READLINE
    stifle_history(200); // ToDo: Make number a preference.
    int error = write_history(history_file.c_str());
    if (error) {
        std::cerr << "write_history error: " << std::strerror(error) << " " << history_file << std::endl;
    }
#endif

    if (_with_gui) {
        _gio_application->quit(); // Force closing windows.
    }
}


// ========================= Callbacks ==========================

/*
 * Handle command line options.
 *
 * Options are processed in the order they appear in this function.
 * We process in order: Print -> GUI -> Open -> Query -> Process -> Export.
 * For each file without GUI: Open -> Query -> Process -> Export
 * More flexible processing can be done via actions.
 */
int
InkscapeApplication::on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options)
{
    if (!options) {
        std::cerr << "InkscapeApplication::on_handle_local_options: options is null!" << std::endl;
        return -1; // Keep going
    }

    auto *gapp = gio_app();

    // ===================== QUERY =====================
    // These are processed first as they result in immediate program termination.
    if (options->contains("version")) {
        gapp->activate_action("inkscape-version");
        return EXIT_SUCCESS;
    }

    if (options->contains("system-data-directory")) {
        gapp->activate_action("system-data-directory");
        return EXIT_SUCCESS;
    }

    if (options->contains("user-data-directory")) {
        gapp->activate_action("user-data-directory");
        return EXIT_SUCCESS;
    }

    if (options->contains("verb-list")) {
        gapp->activate_action("verb-list");
        return EXIT_SUCCESS;
    }

    if (options->contains("action-list")) {
        gapp->activate_action("action-list");
        return EXIT_SUCCESS;
    }

    // For options without arguments.
    auto base = Glib::VariantBase();

    // ================== GUI and Shell ================

    // Use of most commmand line options turns off use of gui unless explicitly requested!
    // Listed in order that they appear in constructor.
    if (options->contains("pipe")                  ||

        options->contains("export-filename")       ||
        options->contains("export-overwrite")      ||
        options->contains("export-type")           ||

        options->contains("export-area-page")      ||
        options->contains("export-area-drawing")   ||
        options->contains("export-area")           ||
        options->contains("export-area-snap")      ||
        options->contains("export-dpi")            ||
        options->contains("export-width")          ||
        options->contains("export-height")         ||
        options->contains("export-margin")         ||
        options->contains("export-height")         ||

        options->contains("export-id")             ||
        options->contains("export-id-only")        ||
        options->contains("export-plain-svg")      ||
        options->contains("export-ps-level")       ||
        options->contains("export-pdf-version")    ||
        options->contains("export-text-to_path")   ||
        options->contains("export-latex")          ||
        options->contains("export-ignore-filters") ||
        options->contains("export-use-hints")      ||
        options->contains("export-background")     ||
        options->contains("export-background-opacity") ||
        options->contains("export-text-to_path")   ||

        options->contains("query-id")              ||
        options->contains("query-x")               ||
        options->contains("query-all")             ||
        options->contains("query-y")               ||
        options->contains("query-width")           ||
        options->contains("query-height")          ||

        options->contains("vacuum-defs")           ||
        options->contains("select")                ||
        options->contains("actions")               ||
        options->contains("verb")                  ||
        options->contains("shell")
        ) {
        _with_gui = false;
    }

    if (options->contains("with-gui")        ||
        options->contains("batch-process")
        ) {
        _with_gui = true; // Override turning GUI off
    }

    if (options->contains("batch-process"))  _batch_process = true;
    if (options->contains("shell"))          _use_shell = true;
    if (options->contains("pipe"))           _use_pipe  = true;


    // Enable auto-export
    if (options->contains("export-filename")  ||
        options->contains("export-type")      ||
        options->contains("export-overwrite") ||
        options->contains("export-use-hints")
        ) {
        _auto_export = true;
    }

    // ==================== ACTIONS ====================
    // Actions as an argument string: e.g.: --actions="query-id:rect1;query-x".
    // Actions will be processed in order that they are given in argument.
    Glib::ustring actions;
    if (options->contains("actions")) {
        options->lookup_value("actions", actions);
        parse_actions(actions, _command_line_actions);
    }


    // ================= OPEN/IMPORT ===================

    if (options->contains("pdf-poppler")) {
        _pdf_poppler = true;
    }
    if (options->contains("pdf-page")) {   // Maybe useful for other file types?
        int page = 0;
        options->lookup_value("pdf-page", page);
        _pdf_page = page;
    }

    if (options->contains("convert-dpi-method")) {
        Glib::ustring method;
        options->lookup_value("convert-dpi-method", method);
        if (!method.empty()) {
            _command_line_actions.push_back(
                std::make_pair("convert-dpi-method", Glib::Variant<Glib::ustring>::create(method)));
        }
    }

    if (options->contains("no-convert-text-baseline-spacing")) _command_line_actions.push_back(std::make_pair("no-convert-baseline", base));


    // ===================== QUERY =====================

    // 'query-id' should be processed first! Can be a comma-separated list.
    if (options->contains("query-id")) {
        Glib::ustring query_id;
        options->lookup_value("query-id", query_id);
        if (!query_id.empty()) {
            _command_line_actions.push_back(
                std::make_pair("select-by-id", Glib::Variant<Glib::ustring>::create(query_id)));
        }
    }

    if (options->contains("query-all"))    _command_line_actions.push_back(std::make_pair("query-all",   base));
    if (options->contains("query-x"))      _command_line_actions.push_back(std::make_pair("query-x",     base));
    if (options->contains("query-y"))      _command_line_actions.push_back(std::make_pair("query-y",     base));
    if (options->contains("query-width"))  _command_line_actions.push_back(std::make_pair("query-width", base));
    if (options->contains("query-height")) _command_line_actions.push_back(std::make_pair("query-height",base));


    // =================== PROCESS =====================

    // Note: this won't work with --verb="FileSave,FileClose" unless some additional verb changes the file. FIXME
    // One can use --verb="FileVacuum,FileSave,FileClose".
    if (options->contains("vacuum-defs"))  _command_line_actions.push_back(std::make_pair("vacuum-defs", base));

    if (options->contains("select")) {
        Glib::ustring select;
        options->lookup_value("select", select);
        if (!select.empty()) {
            _command_line_actions.push_back(
                std::make_pair("select", Glib::Variant<Glib::ustring>::create(select)));
        }
    }

    if (options->contains("verb")) {
        Glib::ustring verb;
        options->lookup_value("verb", verb);
        if (!verb.empty()) {
            _command_line_actions.push_back(
                std::make_pair("verb", Glib::Variant<Glib::ustring>::create(verb)));
        }
    }


    // ==================== EXPORT =====================
    if (options->contains("export-filename")) {
        options->lookup_value("export-filename",  _file_export.export_filename);
    }

    if (options->contains("export-type")) {
        options->lookup_value("export-type",      _file_export.export_type);
    }
    if (options->contains("export-extension")) {
        options->lookup_value("export-extension", _file_export.export_extension);
        _file_export.export_extension = _file_export.export_extension.lowercase();
    }

    if (options->contains("export-overwrite"))    _file_export.export_overwrite    = true;

    // Export - Geometry
    if (options->contains("export-area")) {
        options->lookup_value("export-area",      _file_export.export_area);
    }

    if (options->contains("export-area-drawing")) _file_export.export_area_drawing = true;
    if (options->contains("export-area-page"))    _file_export.export_area_page    = true;

    if (options->contains("export-margin")) {
        options->lookup_value("export-margin",    _file_export.export_margin);
    }

    if (options->contains("export-area-snap"))    _file_export.export_area_snap    = true;

    if (options->contains("export-width")) {
        options->lookup_value("export-width",     _file_export.export_width);
    }

    if (options->contains("export-height")) {
        options->lookup_value("export-height",    _file_export.export_height);
    }

    // Export - Options
    if (options->contains("export-id")) {
        options->lookup_value("export-id",        _file_export.export_id);
    }

    if (options->contains("export-id-only"))      _file_export.export_id_only     = true;
    if (options->contains("export-plain-svg"))    _file_export.export_plain_svg      = true;

    if (options->contains("export-dpi")) {
        options->lookup_value("export-dpi",       _file_export.export_dpi);
    }

    if (options->contains("export-ignore-filters")) _file_export.export_ignore_filters = true;
    if (options->contains("export-text-to-path"))   _file_export.export_text_to_path   = true;

    if (options->contains("export-ps-level")) {
        options->lookup_value("export-ps-level",  _file_export.export_ps_level);
    }

    if (options->contains("export-pdf-version")) {
        options->lookup_value("export-pdf-version", _file_export.export_pdf_level);
    }

    if (options->contains("export-latex"))        _file_export.export_latex       = true;
    if (options->contains("export-use-hints"))    _file_export.export_use_hints   = true;

    if (options->contains("export-background")) {
        options->lookup_value("export-background",_file_export.export_background);
    }

    if (options->contains("export-background-opacity")) {
        options->lookup_value("export-background-opacity", _file_export.export_background_opacity);
    }
    if (options->contains("export-png-color-mode")) {
        options->lookup_value("export-png-color-mode", _file_export.export_png_color_mode);
    }


    // ==================== D-BUS ======================

#ifdef WITH_DBUS
    // Before initializing extensions, we must set the DBus bus name if required
    if (options->contains("dbus-listen")) {
        std::string dbus_name;
        options->lookup_value("dbus-name", dbus_name);
        if (!dbus_name.empty()) {
            Inkscape::Extension::Dbus::dbus_set_bus_name(dbus_name.c_str());
        }
    }
#endif

    return -1; // Keep going
}

//   ========================  Actions  =========================

void
InkscapeApplication::on_new()
{
    create_window();
}

void
InkscapeApplication::on_quit()
{
    gio_app()->quit();
}

void
InkscapeApplication::print_action_list()
{
    auto const *gapp = gio_app();

    auto actions = gapp->list_actions();
    std::sort(actions.begin(), actions.end());
    for (auto const &action : actions) {
        Glib::ustring fullname("app." + action);
        std::cout << std::left << std::setw(20) << action
                  << ":  " << _action_extra_data.get_tooltip_for_action(fullname) << std::endl;
    }
}

//   ======================== macOS =============================

#ifdef GDK_WINDOWING_QUARTZ
/**
 * On macOS, handle dropping files on Inkscape.app icon and "Open With" file association.
 */
static gboolean osx_openfile_callback(GtkosxApplication *osxapp, gchar const *path, InkscapeApplication *app)
{
    auto ptr = Gio::File::create_for_path(path);
    g_return_val_if_fail(ptr, false);
    app->create_window(ptr);
    return true;
}

/**
 * Handle macOS terminating the application
 */
static gboolean osx_quit_callback(GtkosxApplication *, InkscapeApplication *app)
{
    app->destroy_all();
    return true;
}
#endif

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
