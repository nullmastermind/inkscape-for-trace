// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Implementation of the file dialog interfaces defined in filedialogimpl.h.
 */
/* Authors:
 *   Bob Jamison
 *   Joel Holdsworth
 *   Bruno Dilly
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *
 * Copyright (C) 2004-2007 Bob Jamison
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2007-2008 Joel Holdsworth
 * Copyright (C) 2004-2007 The Inkscape Organization
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <iostream>

#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <glibmm/regex.h>
#include <gtkmm/expander.h>

#include "filedialogimpl-gtkmm.h"

#include "document.h"
#include "inkscape.h"
#include "path-prefix.h"
#include "preferences.h"

#include "extension/db.h"
#include "extension/input.h"
#include "extension/output.h"

#include "io/resource.h"
#include "io/sys.h"

#include "ui/dialog-events.h"
#include "ui/view/svg-view-widget.h"

// Routines from file.cpp
#undef INK_DUMP_FILENAME_CONV

#ifdef INK_DUMP_FILENAME_CONV
void dump_str(const gchar *str, const gchar *prefix);
void dump_ustr(const Glib::ustring &ustr);
#endif



namespace Inkscape {
namespace UI {
namespace Dialog {



//########################################################################
//### U T I L I T Y
//########################################################################

void fileDialogExtensionToPattern(Glib::ustring &pattern, Glib::ustring &extension)
{
    for (unsigned int ch : extension) {
        if (Glib::Unicode::isalpha(ch)) {
            pattern += '[';
            pattern += Glib::Unicode::toupper(ch);
            pattern += Glib::Unicode::tolower(ch);
            pattern += ']';
        } else {
            pattern += ch;
        }
    }
}


void findEntryWidgets(Gtk::Container *parent, std::vector<Gtk::Entry *> &result)
{
    if (!parent) {
        return;
    }
    std::vector<Gtk::Widget *> children = parent->get_children();
    for (auto child : children) {
        GtkWidget *wid = child->gobj();
        if (GTK_IS_ENTRY(wid))
            result.push_back(dynamic_cast<Gtk::Entry *>(child));
        else if (GTK_IS_CONTAINER(wid))
            findEntryWidgets(dynamic_cast<Gtk::Container *>(child), result);
    }
}

void findExpanderWidgets(Gtk::Container *parent, std::vector<Gtk::Expander *> &result)
{
    if (!parent)
        return;
    std::vector<Gtk::Widget *> children = parent->get_children();
    for (auto child : children) {
        GtkWidget *wid = child->gobj();
        if (GTK_IS_EXPANDER(wid))
            result.push_back(dynamic_cast<Gtk::Expander *>(child));
        else if (GTK_IS_CONTAINER(wid))
            findExpanderWidgets(dynamic_cast<Gtk::Container *>(child), result);
    }
}


/*#########################################################################
### F I L E     D I A L O G    B A S E    C L A S S
#########################################################################*/

void FileDialogBaseGtk::internalSetup()
{
    // Open executable file dialogs don't need the preview panel
    if (_dialogType != EXE_TYPES) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool enablePreview   = prefs->getBool(preferenceBase + "/enable_preview", true);
        bool enableSVGExport = prefs->getBool(preferenceBase + "/enable_svgexport", false);

        previewCheckbox.set_label(Glib::ustring(_("Enable preview")));
        previewCheckbox.set_active(enablePreview);

        previewCheckbox.signal_toggled().connect(sigc::mem_fun(*this, &FileDialogBaseGtk::_previewEnabledCB));

        svgexportCheckbox.set_label(Glib::ustring(_("Export as SVG 1.1 per settings in Preference Dialog.")));
        svgexportCheckbox.set_active(enableSVGExport);

        svgexportCheckbox.signal_toggled().connect(sigc::mem_fun(*this, &FileDialogBaseGtk::_svgexportEnabledCB));

        // Catch selection-changed events, so we can adjust the text widget
        signal_update_preview().connect(sigc::mem_fun(*this, &FileDialogBaseGtk::_updatePreviewCallback));

        //###### Add a preview widget
        set_preview_widget(svgPreview);
        set_preview_widget_active(enablePreview);
        set_use_preview_label(false);
    }
}


void FileDialogBaseGtk::cleanup(bool showConfirmed)
{
    if (_dialogType != EXE_TYPES) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (showConfirmed) {
            prefs->setBool(preferenceBase + "/enable_preview", previewCheckbox.get_active());
        }
    }
}


void FileDialogBaseGtk::_previewEnabledCB()
{
    bool enabled = previewCheckbox.get_active();
    set_preview_widget_active(enabled);
    if (enabled) {
        _updatePreviewCallback();
    } else {
        // Clears out any current preview image.
        svgPreview.showNoPreview();
    }
}

void FileDialogBaseGtk::_svgexportEnabledCB()
{
    bool enabled = svgexportCheckbox.get_active();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool(preferenceBase + "/enable_svgexport", enabled);
}



/**
 * Callback for checking if the preview needs to be redrawn
 */
void FileDialogBaseGtk::_updatePreviewCallback()
{
    Glib::ustring fileName = get_preview_filename();
    bool enabled = previewCheckbox.get_active();

    if (fileName.empty()) {
        fileName = get_preview_uri();
    }

    if (enabled && !fileName.empty()) {
        svgPreview.set(fileName, _dialogType);
    } else {
        svgPreview.showNoPreview();
    }
}


/*#########################################################################
### F I L E    O P E N
#########################################################################*/

/**
 * Constructor.  Not called directly.  Use the factory.
 */
FileOpenDialogImplGtk::FileOpenDialogImplGtk(Gtk::Window &parentWindow, const Glib::ustring &dir,
                                             FileDialogType fileTypes, const Glib::ustring &title)
    : FileDialogBaseGtk(parentWindow, title, Gtk::FILE_CHOOSER_ACTION_OPEN, fileTypes, "/dialogs/open")
{


    if (_dialogType == EXE_TYPES) {
        /* One file at a time */
        set_select_multiple(false);
    } else {
        /* And also Multiple Files */
        set_select_multiple(true);
    }

    set_local_only(false);

    /* Initialize to Autodetect */
    extension = nullptr;
    /* No filename to start out with */
    myFilename = "";

    /* Set our dialog type (open, import, etc...)*/
    _dialogType = fileTypes;


    /* Set the pwd and/or the filename */
    if (dir.size() > 0) {
        Glib::ustring udir(dir);
        Glib::ustring::size_type len = udir.length();
        // leaving a trailing backslash on the directory name leads to the infamous
        // double-directory bug on win32
        if (len != 0 && udir[len - 1] == '\\')
            udir.erase(len - 1);
        if (_dialogType == EXE_TYPES) {
            set_filename(udir.c_str());
        } else {
            set_current_folder(udir.c_str());
        }
    }

    if (_dialogType != EXE_TYPES) {
        set_extra_widget(previewCheckbox);
    }

    //###### Add the file types menu
    createFilterMenu();

    add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    set_default(*add_button(_("_Open"), Gtk::RESPONSE_OK));

    //###### Allow easy access to our examples folder
    if (Inkscape::IO::file_test(INKSCAPE_EXAMPLESDIR, G_FILE_TEST_EXISTS) &&
        Inkscape::IO::file_test(INKSCAPE_EXAMPLESDIR, G_FILE_TEST_IS_DIR) && g_path_is_absolute(INKSCAPE_EXAMPLESDIR)) {
        add_shortcut_folder(INKSCAPE_EXAMPLESDIR);
    }
}

/**
 * Destructor
 */
FileOpenDialogImplGtk::~FileOpenDialogImplGtk()
= default;

void FileOpenDialogImplGtk::addFilterMenu(Glib::ustring name, Glib::ustring pattern)
{
    auto allFilter = Gtk::FileFilter::create();
    allFilter->set_name(_(name.c_str()));
    allFilter->add_pattern(pattern);
    extensionMap[Glib::ustring(_("All Files"))] = nullptr;
    add_filter(allFilter);
}

void FileOpenDialogImplGtk::createFilterMenu()
{
    if (_dialogType == CUSTOM_TYPE) {
        return;
    }

    if (_dialogType == EXE_TYPES) {
        auto allFilter = Gtk::FileFilter::create();
        allFilter->set_name(_("All Files"));
        allFilter->add_pattern("*");
        extensionMap[Glib::ustring(_("All Files"))] = nullptr;
        add_filter(allFilter);
    } else {
        auto allInkscapeFilter = Gtk::FileFilter::create();
        allInkscapeFilter->set_name(_("All Inkscape Files"));

        auto allFilter = Gtk::FileFilter::create();
        allFilter->set_name(_("All Files"));
        allFilter->add_pattern("*");

        auto allImageFilter = Gtk::FileFilter::create();
        allImageFilter->set_name(_("All Images"));

        auto allVectorFilter = Gtk::FileFilter::create();
        allVectorFilter->set_name(_("All Vectors"));

        auto allBitmapFilter = Gtk::FileFilter::create();
        allBitmapFilter->set_name(_("All Bitmaps"));
        extensionMap[Glib::ustring(_("All Inkscape Files"))] = nullptr;
        add_filter(allInkscapeFilter);

        extensionMap[Glib::ustring(_("All Files"))] = nullptr;
        add_filter(allFilter);

        extensionMap[Glib::ustring(_("All Images"))] = nullptr;
        add_filter(allImageFilter);

        extensionMap[Glib::ustring(_("All Vectors"))] = nullptr;
        add_filter(allVectorFilter);

        extensionMap[Glib::ustring(_("All Bitmaps"))] = nullptr;
        add_filter(allBitmapFilter);

        // patterns added dynamically below
        Inkscape::Extension::DB::InputList extension_list;
        Inkscape::Extension::db.get_input_list(extension_list);

        for (auto imod : extension_list)
        {
            // FIXME: would be nice to grey them out instead of not listing them
            if (imod->deactivated())
                continue;

            Glib::ustring upattern("*");
            Glib::ustring extension = imod->get_extension();
            fileDialogExtensionToPattern(upattern, extension);

            Glib::ustring uname(imod->get_filetypename(true));

            auto filter = Gtk::FileFilter::create();
            filter->set_name(uname);
            filter->add_pattern(upattern);
            add_filter(filter);
            extensionMap[uname] = imod;

// g_message("ext %s:%s '%s'\n", ioext->name, ioext->mimetype, upattern.c_str());
            allInkscapeFilter->add_pattern(upattern);
            if (strncmp("image", imod->get_mimetype(), 5) == 0)
                allImageFilter->add_pattern(upattern);

            // uncomment this to find out all mime types supported by Inkscape import/open
            // g_print ("%s\n", imod->get_mimetype());

            // I don't know of any other way to define "bitmap" formats other than by listing them
            if (strncmp("image/png", imod->get_mimetype(), 9) == 0 ||
                strncmp("image/jpeg", imod->get_mimetype(), 10) == 0 ||
                strncmp("image/gif", imod->get_mimetype(), 9) == 0 ||
                strncmp("image/x-icon", imod->get_mimetype(), 12) == 0 ||
                strncmp("image/x-navi-animation", imod->get_mimetype(), 22) == 0 ||
                strncmp("image/x-cmu-raster", imod->get_mimetype(), 18) == 0 ||
                strncmp("image/x-xpixmap", imod->get_mimetype(), 15) == 0 ||
                strncmp("image/bmp", imod->get_mimetype(), 9) == 0 ||
                strncmp("image/vnd.wap.wbmp", imod->get_mimetype(), 18) == 0 ||
                strncmp("image/tiff", imod->get_mimetype(), 10) == 0 ||
                strncmp("image/x-xbitmap", imod->get_mimetype(), 15) == 0 ||
                strncmp("image/x-tga", imod->get_mimetype(), 11) == 0 ||
                strncmp("image/x-pcx", imod->get_mimetype(), 11) == 0)
            {
                allBitmapFilter->add_pattern(upattern);
             } else {
                allVectorFilter->add_pattern(upattern);
            }
        }
    }
    return;
}

/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
bool FileOpenDialogImplGtk::show()
{
    set_modal(TRUE); // Window
    sp_transientize(GTK_WIDGET(gobj())); // Make transient
    gint b = run(); // Dialog
    svgPreview.showNoPreview();
    hide();

    if (b == Gtk::RESPONSE_OK) {
        // This is a hack, to avoid the warning messages that
        // Gtk::FileChooser::get_filter() returns
        // should be:  Gtk::FileFilter *filter = get_filter();
        GtkFileChooser *gtkFileChooser = Gtk::FileChooser::gobj();
        GtkFileFilter *filter = gtk_file_chooser_get_filter(gtkFileChooser);
        if (filter) {
            // Get which extension was chosen, if any
            extension = extensionMap[gtk_file_filter_get_name(filter)];
        }
        myFilename = get_filename();

        if (myFilename.empty()) {
            myFilename = get_uri();
        }

        cleanup(true);
        return true;
    } else {
        cleanup(false);
        return false;
    }
}



/**
 * Get the file extension type that was selected by the user. Valid after an [OK]
 */
Inkscape::Extension::Extension *FileOpenDialogImplGtk::getSelectionType()
{
    return extension;
}


/**
 * Get the file name chosen by the user.   Valid after an [OK]
 */
Glib::ustring FileOpenDialogImplGtk::getFilename()
{
    return myFilename;
}


/**
 * To Get Multiple filenames selected at-once.
 */
std::vector<Glib::ustring> FileOpenDialogImplGtk::getFilenames()
{
    auto result_tmp = get_filenames();

    // Copy filenames to a vector of type Glib::ustring
    std::vector<Glib::ustring> result;

    for (auto it : result_tmp)
        result.emplace_back(it);

    if (result.empty()) {
        result = get_uris();
    }

    return result;
}

Glib::ustring FileOpenDialogImplGtk::getCurrentDirectory()
{
    return get_current_folder();
}



//########################################################################
//# F I L E    S A V E
//########################################################################

/**
 * Constructor
 */
FileSaveDialogImplGtk::FileSaveDialogImplGtk(Gtk::Window &parentWindow, const Glib::ustring &dir,
                                             FileDialogType fileTypes, const Glib::ustring &title,
                                             const Glib::ustring & /*default_key*/, const gchar *docTitle,
                                             const Inkscape::Extension::FileSaveMethod save_method)
    : FileDialogBaseGtk(parentWindow, title, Gtk::FILE_CHOOSER_ACTION_SAVE, fileTypes,
                        (save_method == Inkscape::Extension::FILE_SAVE_METHOD_SAVE_COPY) ? "/dialogs/save_copy"
                                                                                         : "/dialogs/save_as")
    , save_method(save_method)
    , fromCB(false)
{
    FileSaveDialog::myDocTitle = docTitle;

    /* One file at a time */
    set_select_multiple(false);

    set_local_only(false);

    /* Initialize to Autodetect */
    extension = nullptr;
    /* No filename to start out with */
    myFilename = "";

    /* Set our dialog type (save, export, etc...)*/
    _dialogType = fileTypes;

    /* Set the pwd and/or the filename */
    if (dir.size() > 0) {
        Glib::ustring udir(dir);
        Glib::ustring::size_type len = udir.length();
        // leaving a trailing backslash on the directory name leads to the infamous
        // double-directory bug on win32
        if ((len != 0) && (udir[len - 1] == '\\')) {
            udir.erase(len - 1);
        }
        myFilename = udir;
    }

    //###### Add the file types menu
    // createFilterMenu();

    //###### Do we want the .xxx extension automatically added?
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    fileTypeCheckbox.set_label(Glib::ustring(_("Append filename extension automatically")));
    if (save_method == Inkscape::Extension::FILE_SAVE_METHOD_SAVE_COPY) {
        fileTypeCheckbox.set_active(prefs->getBool("/dialogs/save_copy/append_extension", true));
    } else {
        fileTypeCheckbox.set_active(prefs->getBool("/dialogs/save_as/append_extension", true));
    }

    if (_dialogType != CUSTOM_TYPE)
        createFileTypeMenu();

    fileTypeComboBox.set_size_request(200, 40);
    fileTypeComboBox.signal_changed().connect(sigc::mem_fun(*this, &FileSaveDialogImplGtk::fileTypeChangedCallback));


    childBox.pack_start(checksBox);
    childBox.pack_end(fileTypeComboBox);
    checksBox.pack_start(fileTypeCheckbox);
    checksBox.pack_start(previewCheckbox);
    checksBox.pack_start(svgexportCheckbox);

    set_extra_widget(childBox);

    // Let's do some customization
    fileNameEntry = nullptr;
    Gtk::Container *cont = get_toplevel();
    std::vector<Gtk::Entry *> entries;
    findEntryWidgets(cont, entries);
    // g_message("Found %d entry widgets\n", entries.size());
    if (!entries.empty()) {
        // Catch when user hits [return] on the text field
        fileNameEntry = entries[0];
        fileNameEntry->signal_activate().connect(
            sigc::mem_fun(*this, &FileSaveDialogImplGtk::fileNameEntryChangedCallback));
    }
    signal_selection_changed().connect(
        sigc::mem_fun(*this, &FileSaveDialogImplGtk::fileNameChanged));

    // Let's do more customization
    std::vector<Gtk::Expander *> expanders;
    findExpanderWidgets(cont, expanders);
    // g_message("Found %d expander widgets\n", expanders.size());
    if (!expanders.empty()) {
        // Always show the file list
        Gtk::Expander *expander = expanders[0];
        expander->set_expanded(true);
    }

    // allow easy access to the user's own templates folder
    using namespace Inkscape::IO::Resource;
    char const *templates = Inkscape::IO::Resource::get_path(USER, TEMPLATES);
    if (Inkscape::IO::file_test(templates, G_FILE_TEST_EXISTS) &&
        Inkscape::IO::file_test(templates, G_FILE_TEST_IS_DIR) && g_path_is_absolute(templates)) {
        add_shortcut_folder(templates);
    }

    // if (extension == NULL)
    //    checkbox.set_sensitive(FALSE);

    add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    set_default(*add_button(_("_Save"), Gtk::RESPONSE_OK));

    show_all_children();
}

/**
 * Destructor
 */
FileSaveDialogImplGtk::~FileSaveDialogImplGtk()
= default;

/**
 * Callback for fileNameEntry widget
 */
void FileSaveDialogImplGtk::fileNameEntryChangedCallback()
{
    if (!fileNameEntry)
        return;

    Glib::ustring fileName = fileNameEntry->get_text();
    if (!Glib::get_charset()) // If we are not utf8
        fileName = Glib::filename_to_utf8(fileName);

    // g_message("User hit return.  Text is '%s'\n", fileName.c_str());

    if (!Glib::path_is_absolute(fileName)) {
        // try appending to the current path
        // not this way: fileName = get_current_folder() + "/" + fileName;
        std::vector<Glib::ustring> pathSegments;
        pathSegments.emplace_back(get_current_folder());
        pathSegments.push_back(fileName);
        fileName = Glib::build_filename(pathSegments);
    }

    // g_message("path:'%s'\n", fileName.c_str());

    if (Glib::file_test(fileName, Glib::FILE_TEST_IS_DIR)) {
        set_current_folder(fileName);
    } else if (/*Glib::file_test(fileName, Glib::FILE_TEST_IS_REGULAR)*/ true) {
        // dialog with either (1) select a regular file or (2) cd to dir
        // simulate an 'OK'
        set_filename(fileName);
        response(Gtk::RESPONSE_OK);
    }
}



/**
 * Callback for fileNameEntry widget
 */
void FileSaveDialogImplGtk::fileTypeChangedCallback()
{
    int sel = fileTypeComboBox.get_active_row_number();
    if ((sel < 0) || (sel >= (int)fileTypes.size()))
        return;

    FileType type = fileTypes[sel];
    // g_message("selected: %s\n", type.name.c_str());

    extension = type.extension;
    auto filter = Gtk::FileFilter::create();
    filter->add_pattern(type.pattern);
    set_filter(filter);

    if (fromCB) {
        //do not update if called from a name change
        fromCB = false;
        return;
    }

    updateNameAndExtension();
}

void FileSaveDialogImplGtk::fileNameChanged() {
    Glib::ustring name = get_filename();
    Glib::ustring::size_type pos = name.rfind('.');
    if ( pos == Glib::ustring::npos ) return;
    Glib::ustring ext = name.substr( pos ).casefold();
    if (extension && Glib::ustring(dynamic_cast<Inkscape::Extension::Output *>(extension)->get_extension()).casefold() == ext ) return;
    if (knownExtensions.find(ext) == knownExtensions.end()) return;
    fromCB = true;
    fileTypeComboBox.set_active_text(knownExtensions[ext]->get_filetypename(true));
}

void FileSaveDialogImplGtk::addFileType(Glib::ustring name, Glib::ustring pattern)
{
    //#Let user choose
    FileType guessType;
    guessType.name = name;
    guessType.pattern = pattern;
    guessType.extension = nullptr;
    fileTypeComboBox.append(guessType.name);
    fileTypes.push_back(guessType);


    fileTypeComboBox.set_active(0);
    fileTypeChangedCallback(); // call at least once to set the filter
}

void FileSaveDialogImplGtk::createFileTypeMenu()
{
    Inkscape::Extension::DB::OutputList extension_list;
    Inkscape::Extension::db.get_output_list(extension_list);
    knownExtensions.clear();

    for (auto omod : extension_list) {
        // FIXME: would be nice to grey them out instead of not listing them
        if (omod->deactivated())
            continue;

        FileType type;
        type.name = omod->get_filetypename(true);
        type.pattern = "*";
        Glib::ustring extension = omod->get_extension();
        knownExtensions.insert(std::pair<Glib::ustring, Inkscape::Extension::Output*>(extension.casefold(), omod));
        fileDialogExtensionToPattern(type.pattern, extension);
        type.extension = omod;
        fileTypeComboBox.append(type.name);
        fileTypes.push_back(type);
    }

    //#Let user choose
    FileType guessType;
    guessType.name = _("Guess from extension");
    guessType.pattern = "*";
    guessType.extension = nullptr;
    fileTypeComboBox.append(guessType.name);
    fileTypes.push_back(guessType);


    fileTypeComboBox.set_active(0);
    fileTypeChangedCallback(); // call at least once to set the filter
}



/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
bool FileSaveDialogImplGtk::show()
{
    change_path(myFilename);
    set_modal(TRUE); // Window
    sp_transientize(GTK_WIDGET(gobj())); // Make transient
    gint b = run(); // Dialog
    svgPreview.showNoPreview();
    set_preview_widget_active(false);
    hide();

    if (b == Gtk::RESPONSE_OK) {
        updateNameAndExtension();
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();

        // Store changes of the "Append filename automatically" checkbox back to preferences.
        if (save_method == Inkscape::Extension::FILE_SAVE_METHOD_SAVE_COPY) {
            prefs->setBool("/dialogs/save_copy/append_extension", fileTypeCheckbox.get_active());
        } else {
            prefs->setBool("/dialogs/save_as/append_extension", fileTypeCheckbox.get_active());
        }

        Inkscape::Extension::store_file_extension_in_prefs((extension != nullptr ? extension->get_id() : ""), save_method);

        cleanup(true);

        return true;
    } else {
        cleanup(false);
        return false;
    }
}


/**
 * Get the file extension type that was selected by the user. Valid after an [OK]
 */
Inkscape::Extension::Extension *FileSaveDialogImplGtk::getSelectionType()
{
    return extension;
}

void FileSaveDialogImplGtk::setSelectionType(Inkscape::Extension::Extension *key)
{
    // If no pointer to extension is passed in, look up based on filename extension.
    if (!key) {
        // Not quite UTF-8 here.
        gchar *filenameLower = g_ascii_strdown(myFilename.c_str(), -1);
        for (int i = 0; !key && (i < (int)fileTypes.size()); i++) {
            Inkscape::Extension::Output *ext = dynamic_cast<Inkscape::Extension::Output *>(fileTypes[i].extension);
            if (ext && ext->get_extension()) {
                gchar *extensionLower = g_ascii_strdown(ext->get_extension(), -1);
                if (g_str_has_suffix(filenameLower, extensionLower)) {
                    key = fileTypes[i].extension;
                }
                g_free(extensionLower);
            }
        }
        g_free(filenameLower);
    }

    // Ensure the proper entry in the combo box is selected.
    if (key) {
        extension = key;
        gchar const *extensionID = extension->get_id();
        if (extensionID) {
            for (int i = 0; i < (int)fileTypes.size(); i++) {
                Inkscape::Extension::Extension *ext = fileTypes[i].extension;
                if (ext) {
                    gchar const *id = ext->get_id();
                    if (id && (strcmp(extensionID, id) == 0)) {
                        int oldSel = fileTypeComboBox.get_active_row_number();
                        if (i != oldSel) {
                            fileTypeComboBox.set_active(i);
                        }
                        break;
                    }
                }
            }
        }
    }
}

Glib::ustring FileSaveDialogImplGtk::getCurrentDirectory()
{
    return get_current_folder();
}


/*void
FileSaveDialogImplGtk::change_title(const Glib::ustring& title)
{
    set_title(title);
}*/

/**
  * Change the default save path location.
  */
void FileSaveDialogImplGtk::change_path(const Glib::ustring &path)
{
    myFilename = path;

    if (Glib::file_test(myFilename, Glib::FILE_TEST_IS_DIR)) {
        // fprintf(stderr,"set_current_folder(%s)\n",myFilename.c_str());
        set_current_folder(myFilename);
    } else {
        // fprintf(stderr,"set_filename(%s)\n",myFilename.c_str());
        if (Glib::file_test(myFilename, Glib::FILE_TEST_EXISTS)) {
            set_filename(myFilename);
        } else {
            std::string dirName = Glib::path_get_dirname(myFilename);
            if (dirName != get_current_folder()) {
                set_current_folder(dirName);
            }
        }
        Glib::ustring basename = Glib::path_get_basename(myFilename);
        // fprintf(stderr,"set_current_name(%s)\n",basename.c_str());
        try
        {
            set_current_name(Glib::filename_to_utf8(basename));
        }
        catch (Glib::ConvertError &e)
        {
            g_warning("Error converting save filename to UTF-8.");
            // try a fallback.
            set_current_name(basename);
        }
    }
}

void FileSaveDialogImplGtk::updateNameAndExtension()
{
    // Pick up any changes the user has typed in.
    Glib::ustring tmp = get_filename();

    if (tmp.empty()) {
        tmp = get_uri();
    }

    if (!tmp.empty()) {
        myFilename = tmp;
    }

    Inkscape::Extension::Output *newOut = extension ? dynamic_cast<Inkscape::Extension::Output *>(extension) : nullptr;
    if (fileTypeCheckbox.get_active() && newOut) {
        // Append the file extension if it's not already present and display it in the file name entry field
        appendExtension(myFilename, newOut);
        change_path(myFilename);
    }
}


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
