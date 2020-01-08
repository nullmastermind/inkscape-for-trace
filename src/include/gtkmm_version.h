// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * macro for checking gtkmm version
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_GTKMM_VERSION
#define SEEN_GTKMM_VERSION

#include <gtkmm.h>

#ifndef GTKMM_CHECK_VERSION

#if !defined(GTKMM_MAJOR_VERSION) || !defined(GTKMM_MINOR_VERSION) || !defined(GTKMM_MICRO_VERSION)
    #error "Missing defines for gtkmm version (GTKMM_MAJOR_VERSION / GTKMM_MINOR_VERSION / GTKMM_MICRO_VERSION)"
#endif

/**
 * Check GtkMM version
 *
 * This is adapted from the upstream Gtk+ macro for use with GtkMM
 *
 * @major: major version (e.g. 1 for version 1.2.5)
 * @minor: minor version (e.g. 2 for version 1.2.5)
 * @micro: micro version (e.g. 5 for version 1.2.5)
 *
 * Returns %TRUE if the version of the GTK+ header files
 * is the same as or newer than the passed-in version.
 *
 * Returns: %TRUE if GTK+ headers are new enough
 */
#define GTKMM_CHECK_VERSION(major,minor,micro)                            \
    (GTKMM_MAJOR_VERSION > (major) ||                                     \
    (GTKMM_MAJOR_VERSION == (major) && GTKMM_MINOR_VERSION > (minor)) ||  \
    (GTKMM_MAJOR_VERSION == (major) && GTKMM_MINOR_VERSION == (minor) &&  \
     GTKMM_MICRO_VERSION >= (micro)))

#endif // #ifndef GTKMM_CHECK_VERSION

#endif // SEEN_GTKMM_VERSION
