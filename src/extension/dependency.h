// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_DEPENDENCY_H__
#define INKSCAPE_EXTENSION_DEPENDENCY_H__

#include <glibmm/ustring.h>
#include "xml/repr.h"

namespace Inkscape {
namespace Extension {

class Extension;

/** \brief  A class to represent a dependency for an extension.  There
            are different things that can be done in a dependency, and
            this class takes care of all of them. */
class Dependency {

public:
    /** \brief  All the possible types of dependencies. */
    enum type_t {
        TYPE_EXECUTABLE, /**< Look for an executable */
        TYPE_FILE,       /**< Look to make sure a file exists */
        TYPE_EXTENSION,  /**< Make sure a specific extension is loaded and functional */
        TYPE_CNT         /**< Number of types */
    };
    
    /** \brief  All of the possible locations to look for the dependency. */
    enum location_t {
        LOCATION_PATH,       /**< Look in the PATH for this dependency  - historically this is the default
                                  (it's a bit odd for interpreted script files but makes sense for other executables) */
        LOCATION_EXTENSIONS, /**< Look in the extensions directory
                                  (note: this can be in both, user and system locations!) */
        LOCATION_INX,        /**< Look relative to the inx file's location */
        LOCATION_ABSOLUTE,   /**< This dependency is already defined in absolute terms */
        LOCATION_CNT         /**< Number of locations to look */
    };
    
private:
    static constexpr const char *UNCHECKED = "---unchecked---";
    
    /** \brief  The XML representation of the dependency. */
    Inkscape::XML::Node * _repr;
    /** \brief  The string that is in the XML tags pulled out. */
    const gchar * _string = nullptr;
    /** \brief  The description of the dependency for the users. */
    const gchar * _description = nullptr;
    /** \brief  The absolute path to the dependency file determined while checking this dependency. */
    std::string  _absolute_location = UNCHECKED;

    /** \brief  Storing the type of this particular dependency. */
    type_t _type = TYPE_FILE;
    /** \brief  The location to look for this particular dependency. */
    location_t _location = LOCATION_PATH;

    /** \brief  Strings to represent the different enum values in
                \c type_t in the XML */
    static gchar const * _type_str[TYPE_CNT];
    /** \brief  Strings to represent the different enum values in
                \c location_t in the XML */
    static gchar const * _location_str[LOCATION_CNT];

    /** \brief  Reference to the extension requesting this dependency. */
    const Extension *_extension;

public:
    Dependency (Inkscape::XML::Node *in_repr, const Extension *extension, type_t type=TYPE_FILE);
    virtual ~Dependency ();
    bool check();
    const gchar* get_name();
    std::string get_path();

    Glib::ustring info_string();
}; /* class Dependency */


} }  /* namespace Extension, Inkscape */

#endif /* INKSCAPE_EXTENSION_DEPENDENCY_H__ */

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
