// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2006 Johan Engelen, johan@shouraizou.nl
 * Copyright (C) 2004 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "dependency.h"
#include "db.h"
#include "extension.h"
#include "io/resource.h"

namespace Inkscape {
namespace Extension {

// These strings are for XML attribute comparisons and should not be translated;
// make sure to keep in sync with enum defined in dependency.h
gchar const * Dependency::_type_str[] = {
    "executable",
    "file",
    "extension",
};

// These strings are for XML attribute comparisons and should not be translated
// make sure to keep in sync with enum defined in dependency.h
gchar const * Dependency::_location_str[] = {
    "path",
    "extensions",
    "inx",
    "absolute",
};

/**
    \brief   Create a dependency using an XML definition
    \param   in_repr       XML definition of the dependency
    \param   extension     Reference to the extension requesting this dependency
    \param   default_type  Default file type of the dependency (unless overridden by XML definition's "type" attribute)

    This function mostly looks for the 'location' and 'type' attributes
    and turns them into the enums of the same name.  This makes things
    a little bit easier to use later.  Also, a pointer to the core
    content is pulled out -- also to make things easier.
*/
Dependency::Dependency (Inkscape::XML::Node * in_repr, const Extension *extension, type_t default_type)
    : _repr(in_repr)
    , _extension(extension)
    , _type(default_type)
{
    Inkscape::GC::anchor(_repr);

    if (const gchar * location = _repr->attribute("location")) {
        for (int i = 0; i < LOCATION_CNT && location != nullptr; i++) {
            if (!strcmp(location, _location_str[i])) {
                _location = (location_t)i;
                break;
            }
        }
    } else if (const gchar * location = _repr->attribute("reldir")) { // backwards-compatibility
        for (int i = 0; i < LOCATION_CNT && location != nullptr; i++) {
            if (!strcmp(location, _location_str[i])) {
                _location = (location_t)i;
                break;
            }
        }
    }

    const gchar * type = _repr->attribute("type");
    for (int i = 0; i < TYPE_CNT && type != nullptr; i++) {
        if (!strcmp(type, _type_str[i])) {
            _type = (type_t)i;
            break;
        }
    }

    _string = _repr->firstChild()->content();

    _description = _repr->attribute("description");
    if (_description == nullptr)
        _description = _repr->attribute("_description");

    return;
}

/**
    \brief   This dependency is not longer needed

    Unreference the XML structure.
*/
Dependency::~Dependency ()
{
    Inkscape::GC::release(_repr);
}

/**
    \brief   Check if the dependency passes.
    \return  Whether or not the dependency passes.

    This function depends largely on all of the enums.  The first level
    that is evaluated is the \c _type.

    If the type is \c TYPE_EXTENSION then the id for the extension is
    looked up in the database.  If the extension is found, and it is
    not deactivated, the dependency passes.

    If the type is \c TYPE_EXECUTABLE or \c TYPE_FILE things are getting
    even more interesting because now the \c _location variable is also
    taken into account.  First, the difference between the two is that
    the file test for \c TYPE_EXECUTABLE also tests to make sure the
    file is executable, besides checking that it exists.

    If the \c _location is \c LOCATION_EXTENSIONS then the \c INKSCAPE_EXTENSIONDIR
    is put on the front of the string with \c build_filename.  Then the
    appropriate filetest is run.

    If the \c _location is \c LOCATION_ABSOLUTE then the file test is
    run directly on the string.

    If the \c _location is \c LOCATION_PATH or not specified then the
    path is used to find the file.  Each entry in the path is stepped
    through, attached to the string, and then tested.  If the file is
    found then a TRUE is returned.  If we get all the way through the
    path then a FALSE is returned, the command could not be found.
*/
bool Dependency::check ()
{
    if (_string == nullptr) {
        return false;
    }

    _absolute_location = "";

    switch (_type) {
        case TYPE_EXTENSION: {
            Extension * myext = db.get(_string);
            if (myext == nullptr) return false;
            if (myext->deactivated()) return false;
            break;
        }
        case TYPE_EXECUTABLE:
        case TYPE_FILE: {
            Glib::FileTest filetest = Glib::FILE_TEST_EXISTS;

            std::string location(_string);

            // get potential file extension for later usage
            std::string extension;
            size_t index = location.find_last_of(".");
            if (index != std::string::npos) {
                extension = location.substr(index);
            }

            // check interpreted scripts as "file" for backwards-compatibility, even if "executable" was requested
            static const std::vector<std::string> interpreted = {".py", ".pl", ".rb"};
            if (!extension.empty() &&
                    std::find(interpreted.begin(), interpreted.end(), extension) != interpreted.end())
            {
                _type = TYPE_FILE;
            }

#ifndef _WIN32
            // There's no executable bit on Windows, so this is unreliable
            // glib would search for "executable types" instead, which are only {".exe", ".cmd", ".bat", ".com"},
            // and would therefore miss files without extension and other script files (like .py files)
            if (_type == TYPE_EXECUTABLE) {
                filetest = Glib::FILE_TEST_IS_EXECUTABLE;
            }
#endif

            switch (_location) {
                 // backwards-compatibility: location="extensions" will be deprecated as of Inkscape 1.1,
                 //                          use location="inx" instead
                case LOCATION_EXTENSIONS: {
                    // get_filename will warn if the resource isn't found, while returning an empty string.
                    std::string temploc =
                        Inkscape::IO::Resource::get_filename_string(Inkscape::IO::Resource::EXTENSIONS, location.c_str());
                    if (!temploc.empty()) {
                        location = temploc;
                        _absolute_location = temploc;
                        break;
                    }
                    /* Look for deprecated locations next */
                    auto deprloc = g_build_filename("inkex", "deprecated-simple", location.c_str(), nullptr);
                    std::string tempdepr =
                        Inkscape::IO::Resource::get_filename_string(Inkscape::IO::Resource::EXTENSIONS, deprloc, false, true);
                    g_free(deprloc);
                    if (!tempdepr.empty()) {
                        location = tempdepr;
                        _absolute_location = tempdepr;
                        break;
                    }
                // PASS THROUGH!!! - also check inx location for backwards-compatibility,
                //                   notably to make extension manager work
                //                   (installs into subfolders of "extensions" directory)
                }
                case LOCATION_INX: {
                    std::string base_directory = _extension->get_base_directory();
                    if (base_directory.empty()) {
                        g_warning("Dependency '%s' requests location relative to .inx file, "
                                  "which is unknown for extension '%s'", _string, _extension->get_id());
                    }
                    std::string absolute_location = Glib::build_filename(base_directory, location);
                    if (!Glib::file_test(absolute_location, filetest)) {
                        return false;
                    }
                    _absolute_location = absolute_location;
                    break;
                }
                case LOCATION_ABSOLUTE: {
                    // TODO: should we check if the directory actually is absolute and/or sanitize the filename somehow?
                    if (!Glib::file_test(location, filetest)) {
                        return false;
                    }
                    _absolute_location = location;
                    break;
                }
                /* The default case is to look in the path */
                case LOCATION_PATH:
                default: {
                    // TODO: we can likely use g_find_program_in_path (or its glibmm equivalent) for executable types

                    gchar * path = g_strdup(g_getenv("PATH"));

                    if (path == nullptr) {
                        /* There is no `PATH' in the environment.
                           The default search path is the current directory */
                        path = g_strdup(G_SEARCHPATH_SEPARATOR_S);
                    }

                    gchar * orig_path = path;

                    for (; path != nullptr;) {
                        gchar * local_path; // to have the path after detection of the separator
                        std::string final_name;

                        local_path = path;
                        path = g_utf8_strchr(path, -1, G_SEARCHPATH_SEPARATOR);
                        /* Not sure whether this is UTF8 happy, but it would seem
                           like it considering that I'm searching (and finding)
                           the ':' character */
                        if (path != nullptr) {
                            path[0] = '\0';
                            path++;
                        }

                        if (*local_path == '\0') {
                            final_name = _string;
                        } else {
                            final_name = Glib::build_filename(local_path, _string);
                        }

                        if (Glib::file_test(final_name, filetest)) {
                            g_free(orig_path);
                            _absolute_location = final_name;
                            return true;
                        }

#ifdef _WIN32
                        // Unfortunately file extensions tend to be different on Windows and we can't know
                        // which one it is, so try all extensions glib assumes to be executable.
                        // As we can only guess here, return the version without extension if either one is found,
                        // so that we don't accidentally override (or conflict with) some g_spawn_* magic.
                        if (_type == TYPE_EXECUTABLE) {
                            static const std::vector<std::string> extensions = {".exe", ".cmd", ".bat", ".com"};
                            if (extension.empty() ||
                                    std::find(extensions.begin(), extensions.end(), extension) == extensions.end())
                            {
                                for (auto extension : extensions) {
                                    if (Glib::file_test(final_name + extension, filetest)) {
                                        g_free(orig_path);
                                        _absolute_location = final_name;
                                        return true;
                                    }
                                }
                            }
                        }
#endif
                    }

                    g_free(orig_path);
                    return false; /* Reverse logic in this one */
                }
            } /* switch _location */
            break;
        } /* TYPE_FILE, TYPE_EXECUTABLE */
        default:
            return false;
    } /* switch _type */

    return true;
}

/**
    \brief   Accessor to the name attribute.
    \return  A string containing the name of the dependency.

    Returns the name of the dependency as found in the configuration file.

*/
const gchar* Dependency::get_name()
{
    return _string;
}

/**
    \brief   Path of this dependency
    \return  Absolute path to the dependency file
             (or an empty string if dependency was not found or is of TYPE_EXTENSION)

    Returns the verified absolute path of the dependency file.
    This value is only available after checking the Dependency by calling Dependency::check().
*/
std::string Dependency::get_path()
{
    if (_type == TYPE_EXTENSION) {
        g_warning("Requested absolute path of dependency '%s' which is of 'extension' type.", _string);
        return "";
    }
    if (_absolute_location == UNCHECKED) {
        g_warning("Requested absolute path of dependency '%s' which is unchecked.", _string);
        return "";
    }

    return _absolute_location;
}

/**
    \brief   Print out a dependency to a string.
*/
Glib::ustring Dependency::info_string()
{
    Glib::ustring str = Glib::ustring::compose("%1:\n\t%2: %3\n\t%4: %5\n\t%6: %7",
                                               _("Dependency"),
                                               _("type"),     _(_type_str[_type]),
                                               _("location"), _(_location_str[_location]),
                                               _("string"),     _string);

    if (_description) {
        str += Glib::ustring::compose("\n\t%1: %2\n", _("  description: "), _(_description));
    }

    return str;
}

} }  /* namespace Inkscape, Extension */

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
