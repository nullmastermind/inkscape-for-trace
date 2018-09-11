// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Inkscape registry tool
 *//*
 * Authors:
 *   see git history
 *   Bob Jamison
 * Copyright (C) 2005-2018 Authors
 * 
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_REGISTRYTOOL_H
#define SEEN_REGISTRYTOOL_H

namespace Glib {
    class ustring;
}

/**
 * Inkscape Registry Tool
 *
 * This simple tool is intended for allowing Inkscape to append subdirectories
 * to its path.  This will allow extensions and other files to be accesses
 * without explicit user intervention.
 */
class RegistryTool
{
public:

    RegistryTool()
        {}

    virtual ~RegistryTool()
        {}

    /**
     * Set the string value of a key/name registry entry.
     */ 
    bool setStringValue(const Glib::ustring &key,
                        const Glib::ustring &valueName,
                        const Glib::ustring &value);

    /**
     * Get the full path, directory, and base file name of this running executable.
     */ 
    bool getExeInfo(Glib::ustring &fullPath,
                    Glib::ustring &path,
                    Glib::ustring &exeName);

    /**
     * Append our subdirectories to the Application Path for this
     * application.
     */  
    bool setPathInfo();


};

#endif // SEEN_REGISTRYTOOL_H

