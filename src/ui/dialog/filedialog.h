// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Virtual base definitions for native file dialogs
 */
/* Authors:
 *   Bob Jamison <rwjj@earthlink.net>
 *   Joel Holdsworth
 *   Inkscape Guys
 *
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2007-2008 Joel Holdsworth
 * Copyright (C) 2004-2008, Inkscape Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef __FILE_DIALOG_H__
#define __FILE_DIALOG_H__

#include <vector>
#include <set>

#include "extension/system.h"

#include <glibmm/ustring.h>

class SPDocument;

namespace Inkscape {
namespace Extension {
class Extension;
class Output;
}
}

namespace Inkscape
{
namespace UI
{
namespace Dialog
{

/**
 * Used for setting filters and options, and
 * reading them back from user selections.
 */
enum FileDialogType
{
    SVG_TYPES,
    IMPORT_TYPES,
    EXPORT_TYPES,
    RASTER_TYPES,
    EXE_TYPES,
    SWATCH_TYPES,
    CUSTOM_TYPE
};

/**
 * Used for returning the type selected in a SaveAs
 */
enum FileDialogSelectionType {
    SVG_NAMESPACE,
    SVG_NAMESPACE_WITH_EXTENSIONS
    };


/**
 * Return true if the string ends with the given suffix
 */
bool hasSuffix(const Glib::ustring &str, const Glib::ustring &ext);

/**
 * Return true if the image is loadable by Gdk, else false
 */
bool isValidImageFile(const Glib::ustring &fileName);

/**
 * This class provides an implementation-independent API for
 * file "Open" dialogs.  Using a standard interface obviates the need
 * for ugly #ifdefs in file open code
 */
class FileOpenDialog
{
public:


    /**
     * Constructor ..  do not call directly
     * @param path the directory where to start searching
     * @param fileTypes one of FileDialogTypes
     * @param title the title of the dialog
     */
    FileOpenDialog()
        = default;;

    /**
     * Factory.
     * @param path the directory where to start searching
     * @param fileTypes one of FileDialogTypes
     * @param title the title of the dialog
     */
    static FileOpenDialog *create(Gtk::Window& parentWindow,
                                  const Glib::ustring &path,
                                  FileDialogType fileTypes,
                                  const char *title);


    /**
     * Destructor.
     * Perform any necessary cleanups.
     */
    virtual ~FileOpenDialog() = default;;

    /**
     * Show an OpenFile file selector.
     * @return the selected path if user selected one, else NULL
     */
    virtual bool show() = 0;

    /**
     * Return the 'key' (filetype) of the selection, if any
     * @return a pointer to a string if successful (which must
     * be later freed with g_free(), else NULL.
     */
    virtual Inkscape::Extension::Extension * getSelectionType() = 0;

    Glib::ustring getFilename();

    virtual std::vector<Glib::ustring> getFilenames() = 0;

    virtual Glib::ustring getCurrentDirectory() = 0;

    virtual void addFilterMenu(Glib::ustring name, Glib::ustring pattern) = 0;

protected:
    /**
     * Filename that was given
     */
    Glib::ustring myFilename;

}; //FileOpenDialog






/**
 * This class provides an implementation-independent API for
 * file "Save" dialogs.
 */
class FileSaveDialog
{
public:

    /**
     * Constructor.  Do not call directly .   Use the factory.
     * @param path the directory where to start searching
     * @param fileTypes one of FileDialogTypes
     * @param title the title of the dialog
     * @param key a list of file types from which the user can select
     */
    FileSaveDialog ()
        = default;;

    /**
     * Factory.
     * @param path the directory where to start searching
     * @param fileTypes one of FileDialogTypes
     * @param title the title of the dialog
     * @param key a list of file types from which the user can select
     */
    static FileSaveDialog *create(Gtk::Window& parentWindow,
                                  const Glib::ustring &path,
                                  FileDialogType fileTypes,
                                  const char *title,
                                  const Glib::ustring &default_key,
                                  const gchar *docTitle,
                                  const Inkscape::Extension::FileSaveMethod save_method);


    /**
     * Destructor.
     * Perform any necessary cleanups.
     */
    virtual ~FileSaveDialog() = default;;


    /**
     * Show an SaveAs file selector.
     * @return the selected path if user selected one, else NULL
     */
    virtual bool show() =0;

    /**
     * Return the 'key' (filetype) of the selection, if any
     * @return a pointer to a string if successful (which must
     * be later freed with g_free(), else NULL.
     */
    virtual Inkscape::Extension::Extension * getSelectionType() = 0;

    virtual void setSelectionType( Inkscape::Extension::Extension * key ) = 0;

    /**
     * Get the file name chosen by the user.   Valid after an [OK]
     */
    Glib::ustring getFilename ();

    /**
     * Get the document title chosen by the user.   Valid after an [OK]
     */
    Glib::ustring getDocTitle ();

    virtual Glib::ustring getCurrentDirectory() = 0;

    virtual void addFileType(Glib::ustring name, Glib::ustring pattern) = 0;

protected:

    /**
     * Filename that was given
     */
    Glib::ustring myFilename;

    /**
     * Doc Title that was given
     */
    Glib::ustring myDocTitle;

    /**
     * List of known file extensions.
     */
    std::map<Glib::ustring, Inkscape::Extension::Output*> knownExtensions;


    void appendExtension(Glib::ustring& path, Inkscape::Extension::Output* outputExtension);

}; //FileSaveDialog


} //namespace Dialog
} //namespace UI
} //namespace Inkscape

#endif /* __FILE_DIALOG_H__ */

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
