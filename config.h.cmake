// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef _CONFIG_H_
#define _CONFIG_H_


/* Name of package */
#define PACKAGE "${PROJECT_NAME}"

/* Define to the full name of this package. */
#define PACKAGE_NAME "${PROJECT_NAME}"

/* Translation domain used */
#define GETTEXT_PACKAGE "${PROJECT_NAME}"

/* Localization directory */
#define PACKAGE_LOCALE_DIR "${PACKAGE_LOCALE_DIR}"
#define PACKAGE_LOCALE_DIR_ABSOLUTE "${CMAKE_INSTALL_PREFIX}/${PACKAGE_LOCALE_DIR}"

/* Base data directory -- only path-prefix.h should use it! */
#define INKSCAPE_DATADIR "${INKSCAPE_DATADIR}"


/* Use AutoPackage? */
#cmakedefine ENABLE_BINRELOC 1

/* define to 1 if you have lcms version 2.x */
#cmakedefine HAVE_LIBLCMS2 1

/* always defined to indicate that i18n is enabled */
#cmakedefine ENABLE_NLS 1

/* Build with OSX .app data dir paths? */
#cmakedefine ENABLE_OSX_APP_LOCATIONS 1

/* Whether the Cairo PDF backend is available */
#cmakedefine PANGO_ENABLE_ENGINE 1

/* Build with GDL 3.6 or higher */
#cmakedefine WITH_GDL_3_6 1

/* Define to 1 if you have the <ieeefp.h> header file. */
#cmakedefine HAVE_IEEEFP_H 1

/* Define to 1 if you have the `mallinfo' function. */
#cmakedefine HAVE_MALLINFO 1

/* Define to 1 if you have the <malloc.h> header file. */
#cmakedefine HAVE_MALLOC_H 1

/* Use OpenMP (via cmake) */
#cmakedefine HAVE_OPENMP 1

/* Use aspell for built-in spellchecker */
#cmakedefine HAVE_ASPELL 1

/* Use libpoppler for direct PDF import */
#cmakedefine HAVE_POPPLER 1

/* Use libpoppler-cairo for rendering PDF preview */
#cmakedefine HAVE_POPPLER_CAIRO 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if `fordblks' is member of `struct mallinfo'. */
#cmakedefine HAVE_STRUCT_MALLINFO_FORDBLKS 1

/* Define to 1 if `fsmblks' is member of `struct mallinfo'. */
#cmakedefine HAVE_STRUCT_MALLINFO_FSMBLKS 1

/* Define to 1 if `hblkhd' is member of `struct mallinfo'. */
#cmakedefine HAVE_STRUCT_MALLINFO_HBLKHD 1

/* Define to 1 if `uordblks' is member of `struct mallinfo'. */
#cmakedefine HAVE_STRUCT_MALLINFO_UORDBLKS

/* Define to 1 if `usmblks' is member of `struct mallinfo'. */
#cmakedefine HAVE_STRUCT_MALLINFO_USMBLKS 1

/* Build in dbus */
#cmakedefine WITH_DBUS 1

/* enable gtk spelling widget */
#cmakedefine WITH_GTKSPELL 1

/* Image/Graphics Magick++ support for bitmap effects */
#cmakedefine WITH_MAGICK 1

/* Use libjpeg */
#cmakedefine HAVE_JPEG 1

/* Build in libcdr */
#cmakedefine WITH_LIBCDR 1

/* Build using libcdr 0.1.x */
#cmakedefine WITH_LIBCDR01 1

/* Build in libvisio */
#cmakedefine WITH_LIBVISIO 1

/* Build using libvisio 0.1.x */
#cmakedefine WITH_LIBVISIO01 1

/* Build in libwpg */
#cmakedefine WITH_LIBWPG 1

/* Build in libwpg-0.2 */
#cmakedefine WITH_LIBWPG02 1

/* Build in libwpg-0.3 */
#cmakedefine WITH_LIBWPG03 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#cmakedefine WORDS_BIGENDIAN 1

/* Enable LPE Tool? */
#cmakedefine WITH_LPETOOL 0

/* Do we want experimental, unsupported, unguaranteed, etc., LivePathEffects enabled? */
#cmakedefine LPE_ENABLE_TEST_EFFECTS 0

#endif /* _CONFIG_H_ */
