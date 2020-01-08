// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * macro for checking glibmm version
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2020 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_GLIBMM_VERSION
#define SEEN_GLIBMM_VERSION

#include <glibmm.h>

#ifndef GLIBMM_CHECK_VERSION

#if !defined(GLIBMM_MAJOR_VERSION) || !defined(GLIBMM_MINOR_VERSION) || !defined(GLIBMM_MICRO_VERSION)
    #error "Missing defines for glibmm version (GLIBMM_MAJOR_VERSION / GLIBMM_MINOR_VERSION / GLIBMM_MICRO_VERSION)"
#endif

/**
 * Check glibmm version
 *
 * This is adapted from the upstream gtk macro for use with glibmm
 *
 * @major: major version (e.g. 1 for version 1.2.5)
 * @minor: minor version (e.g. 2 for version 1.2.5)
 * @micro: micro version (e.g. 5 for version 1.2.5)
 *
 * Returns %TRUE if the version of the glibmm header files
 * is the same as or newer than the passed-in version.
 *
 * Returns: %TRUE if glibmm headers are new enough
 */
#define GLIBMM_CHECK_VERSION(major,minor,micro)                            \
    (GLIBMM_MAJOR_VERSION > (major) ||                                     \
    (GLIBMM_MAJOR_VERSION == (major) && GLIBMM_MINOR_VERSION > (minor)) ||  \
    (GLIBMM_MAJOR_VERSION == (major) && GLIBMM_MINOR_VERSION == (minor) &&  \
     GLIBMM_MICRO_VERSION >= (micro)))

#endif // #ifndef GLIBMM_CHECK_VERSION

#endif // SEEN_GLIBMM_VERSION
