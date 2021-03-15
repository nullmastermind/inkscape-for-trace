// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Consolidates version info for Inkscape,
 * its various dependencies and the OS we're running on
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2021 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include <ostream>
#include <string>

#include <hb.h> // not indirectly included via gtk.h on macOS
#include <glib.h>
#include <glibmm.h>
#include <gtk/gtk.h>
#include <gtkmm.h>
#include <libxml2/libxml/xmlversion.h>
#include <libxslt/xsltconfig.h>
#include <poppler-config.h>

#include "inkscape-version.h" // Inkscape version


namespace Inkscape {

/**
 * Return Inkscape version string
 *
 * Returns the Inkscape version string including program name.
 *
 * @return version string
 */
std::string inkscape_version() {
    return std::string("Inkscape ") + Inkscape::version_string;
}

/**
 * Return OS version string
 *
 * Returns the OS version string including OS name.
 *
 * Relies on glib's 'g_get_os_info'.
 * Might be undefined on some OSs. "(unknown)" is returned in this case.
 *
 * @return version string
 */
std::string os_version() {
    std::string os_version_string = "(unknown)";

#if GLIB_CHECK_VERSION(2,64,0)
    char *os_name = g_get_os_info(G_OS_INFO_KEY_NAME);
    char *os_pretty_name = g_get_os_info(G_OS_INFO_KEY_PRETTY_NAME);
    if (os_pretty_name) {
        os_version_string = os_pretty_name;
    } else if (os_name) {
        os_version_string = os_name;
    }
    g_free(os_name);
    g_free(os_pretty_name);
#endif

    return os_version_string;
}

/**
 * Return full debug info
 *
 * Returns full debug info including:
 *  - Inkscape version
 *  - versions of various dependencies
 *  - OS name and version
 *
 * @return debug info
 */
std::string debug_info() {
    std::stringstream ss;

    ss << inkscape_version() << std::endl;
    ss << std::endl;
    ss << "    GLib version:     " << glib_major_version   << "." << glib_minor_version   << "." << glib_micro_version   << std::endl;
    ss << "    GTK version:      " << gtk_major_version    << "." << gtk_minor_version    << "." << gtk_micro_version    << std::endl;
    ss << "    glibmm version:   " << GLIBMM_MAJOR_VERSION << "." << GLIBMM_MINOR_VERSION << "." << GLIBMM_MICRO_VERSION << std::endl;
    ss << "    gtkmm version:    " << GTKMM_MAJOR_VERSION  << "." << GTKMM_MINOR_VERSION  << "." << GTKMM_MICRO_VERSION  << std::endl;
    ss << "    libxml2 version:  " << LIBXML_DOTTED_VERSION << std::endl;
    ss << "    libxslt version:  " << LIBXSLT_DOTTED_VERSION << std::endl;
    ss << "    Cairo version:    " << cairo_version_string() << std::endl;
    ss << "    Pango version:    " << pango_version_string() << std::endl;
    ss << "    HarfBuzz version: " << hb_version_string() << std::endl;
    ss << "    Poppler version:  " << POPPLER_VERSION << std::endl;
    ss << std::endl;
    ss << "    OS version:       " << os_version();

    return ss.str();
}

} // namespace Inkscape


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace .0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99: