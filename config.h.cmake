#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Define WIN32 when on windows */
#ifndef WIN32
#cmakedefine WIN32
#endif

/* Use binreloc thread support? */
#cmakedefine BR_PTHREADS 1

/* Use AutoPackage? */
#cmakedefine ENABLE_BINRELOC 1

/* define to 1 if you have lcms version 1.x */
#cmakedefine HAVE_LIBLCMS1 1

/* define to 1 if you have lcms version 2.x */
#cmakedefine HAVE_LIBLCMS2 1

/* always defined to indicate that i18n is enabled */
#cmakedefine ENABLE_NLS 1

/* Build with OSX .app data dir paths? */
#cmakedefine ENABLE_OSX_APP_LOCATIONS 1

/* Translation domain used */
#define GETTEXT_PACKAGE "${PROJECT_NAME}"

/* Define to 1 if you have the `bind_textdomain_codeset' function. */
#cmakedefine HAVE_BIND_TEXTDOMAIN_CODESET 1

/* Whether the Cairo PDF backend is available */
#cmakedefine HAVE_CAIRO_PDF 1
#cmakedefine PANGO_ENABLE_ENGINE 1
#cmakedefine RENDER_WITH_PANGO_CAIRO 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `floor' function. */
#cmakedefine HAVE_FLOOR 1

/* Define to 1 if you have the `fpsetmask' function. */
#cmakedefine HAVE_FPSETMASK 1

/* Define to 1 if you have the `gettimeofday' function. */
#cmakedefine HAVE_GETTIMEOFDAY 1

/* Build with Gtkmm 3.0.x or higher */
#cmakedefine WITH_GTKMM_3_0 1

/* Build with Gtkmm 3.10.x or higher */
#cmakedefine WITH_GTKMM_3_10 1

/* Build with Gtkmm 3.12.x or higher */
#cmakedefine WITH_GTKMM_3_12 1

/* Build with Gtkmm 3.16.x or higher */
#cmakedefine WITH_GTKMM_3_16 1

/* Build with Gtkmm 3.22.x or higher */
#cmakedefine WITH_GTKMM_3_22 1

/* Build with GDL 3.6 or higher */
#cmakedefine WITH_GDL_3_6 1

/* Define to 1 if you have the <ieeefp.h> header file. */
#cmakedefine HAVE_IEEEFP_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the `mallinfo' function. */
#cmakedefine HAVE_MALLINFO 1

/* Define to 1 if you have the <malloc.h> header file. */
#cmakedefine HAVE_MALLOC_H 1

/* Define to 1 if you have the `memmove' function. */
#cmakedefine HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#cmakedefine HAVE_MEMSET 1

/* Use OpenMP (via cmake) */
#cmakedefine HAVE_OPENMP 1

/* Use aspell for built-in spellchecker */
#cmakedefine HAVE_ASPELL 1

/* Use libpoppler for direct PDF import */
#cmakedefine HAVE_POPPLER 1

/* Use libpoppler-cairo for rendering PDF preview */
#cmakedefine HAVE_POPPLER_CAIRO 1

/* Use libpoppler-glib and Cairo-SVG for PDF import */
#cmakedefine HAVE_POPPLER_GLIB 1

/* Use color space API from Poppler >= 0.26.0 */
#cmakedefine POPPLER_EVEN_NEWER_COLOR_SPACE_API 1

/* Use color space API from Poppler >= 0.29.0 */
#cmakedefine POPPLER_EVEN_NEWER_NEW_COLOR_SPACE_API 1

/* Use object API from Poppler >= 0.58.0 */
#cmakedefine POPPLER_NEW_OBJECT_API 1

/* Define to 1 if you have the `pow' function. */
#cmakedefine HAVE_POW 1

/* Define to 1 if you have the `sqrt' function. */
#cmakedefine HAVE_SQRT 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the `strpbrk' function. */
#cmakedefine HAVE_STRPBRK 1

/* Define to 1 if you have the `strrchr' function. */
#cmakedefine HAVE_STRRCHR 1

/* Define to 1 if you have the `strspn' function. */
#cmakedefine HAVE_STRSPN 1

/* Define to 1 if you have the `strstr' function. */
#cmakedefine HAVE_STRSTR 1

/* Define to 1 if you have the `strtoul' function. */
#cmakedefine HAVE_STRTOUL 1

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

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Base data directory -- only path-prefix.h should use it! */
#define INKSCAPE_DATADIR "${INKSCAPE_DATADIR}"

/* Base library directory -- only path-prefix.h should use it! */
#define INKSCAPE_LIBDIR "${INKSCAPE_LIBDIR}"

/* Name of package */
#define PACKAGE "${PROJECT_NAME}"

/* Localization directory */
#define PACKAGE_LOCALE_DIR "${PACKAGE_LOCALE_DIR}"

/* Define to the full name of this package. */
#define PACKAGE_NAME "${PROJECT_NAME}"

/* Build in dbus */
#cmakedefine WITH_DBUS 1

/* enable gtk spelling widget */
#cmakedefine WITH_GTKSPELL 1

/* Image Magick++ support for bitmap effects */
#cmakedefine WITH_IMAGE_MAGICK 1

/* Use libjpeg */
#cmakedefine HAVE_JPEG 1

/* Build in libcdr */
#cmakedefine WITH_LIBCDR 1

/* Build using libcdr 0.0.x */
#cmakedefine WITH_LIBCDR00 1

/* Build using libcdr 0.1.x */
#cmakedefine WITH_LIBCDR01 1

/* Build in libvisio */
#cmakedefine WITH_LIBVISIO 1

/* Build using libvisio 0.0.x */
#cmakedefine WITH_LIBVISIO00 1

/* Build using libvisio 0.1.x */
#cmakedefine WITH_LIBVISIO01 1

/* Build in libwpg */
#cmakedefine WITH_LIBWPG 1

/* Build in libwpg-0.1 */
#cmakedefine WITH_LIBWPG01 1

/* Build in libwpg-0.2 */
#cmakedefine WITH_LIBWPG02 1

/* Build in libwpg-0.3 */
#cmakedefine WITH_LIBWPG03 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#cmakedefine WORDS_BIGENDIAN 1

/* Do we want experimental, unsupported, unguaranteed, etc., LivePathEffects enabled? */
#cmakedefine LPE_ENABLE_TEST_EFFECTS 1

/* Local variables to store GTKMM version
#cmakedefine INKSCAPE_GTKMM_MAJOR_VERSION @INKSCAPE_GTKMM_MAJOR_VERSION@
#cmakedefine INKSCAPE_GTKMM_MINOR_VERSION @INKSCAPE_GTKMM_MINOR_VERSION@
#cmakedefine INKSCAPE_GTKMM_MICRO_VERSION @INKSCAPE_GTKMM_MICRO_VERSION@

/**
 * Check GtkMM version
 *
 * This is adapted from the upstream Gtk+ macro for use with GtkMM
 *
 * @todo Perhaps this should be in its own header?  However, this is likely to
 *       be used very frequently, so It would be annoying to have to add another
 *	 header inclusion in many files
 *
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
  (INKSCAPE_GTKMM_MAJOR_VERSION > (major) ||                                     \
  (INKSCAPE_GTKMM_MAJOR_VERSION == (major) && INKSCAPE_GTKMM_MINOR_VERSION > (minor)) || \
  (INKSCAPE_GTKMM_MAJOR_VERSION == (major) && INKSCAPE_GTKMM_MINOR_VERSION == (minor) && \
  INKSCAPE_GTKMM_MICRO_VERSION >= (micro)))

#endif /* _CONFIG_H_ */


