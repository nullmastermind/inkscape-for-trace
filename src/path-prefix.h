// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 * Separate the inkscape paths from the prefix code, as that is kind of
 * a separate package (binreloc)
 * 	http://autopackage.org/downloads.html
 *
 * Since the directories set up by autoconf end up in config.h, we can't
 * _change_ them, since config.h isn't protected by a set of
 * one-time-include directives and is repeatedly re-included by some
 * chains of .h files.  As a result, nothing should refer to those
 * define'd directories, and instead should use only the paths defined here.
 *
 */
#ifndef SEEN_PATH_PREFIX_H
#define SEEN_PATH_PREFIX_H

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "prefix.h"

#ifdef ENABLE_BINRELOC // TODO: Should we drop binreloc in favor of OS-specific relocation code
                       //       in append_inkscape_datadir() like we already do for win32?
/* The way that we're building now is with a shared library between Inkscape
   and Inkview, and the code will find the path to the library then. But we
   don't really want that. This prefix then pulls things out of the lib directory
   and back into the root install dir. */
#  define INKSCAPE_LIBPREFIX      "/../.."
#  define INKSCAPE_DATADIR_REAL   BR_DATADIR( INKSCAPE_LIBPREFIX "/share")
#  define INKSCAPE_SYSTEMDIR      BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape")
#  define INKSCAPE_ATTRRELDIR     BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/attributes" )
#  define INKSCAPE_DOCDIR         BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/doc" )
#  define INKSCAPE_EXAMPLESDIR    BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/examples" )
#  define INKSCAPE_EXTENSIONDIR   BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/extensions" )
#  define INKSCAPE_FILTERDIR      BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/filters" )
#  define INKSCAPE_FONTSDIR       BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/fonts" )
#  define INKSCAPE_KEYSDIR        BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/keys" )
#  define INKSCAPE_ICONSDIR       BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/icons" )
#  define INKSCAPE_PIXMAPSDIR     BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/pixmaps" )
#  define INKSCAPE_MARKERSDIR     BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/markers" )
#  define INKSCAPE_PAINTDIR       BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/paint" )
#  define INKSCAPE_PALETTESDIR    BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/palettes" )
#  define INKSCAPE_SCREENSDIR     BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/screens" )
#  define INKSCAPE_SYMBOLSDIR     BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/symbols" )
#  define INKSCAPE_THEMEDIR       BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/themes" )
#  define INKSCAPE_TUTORIALSDIR   BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/tutorials" )
#  define INKSCAPE_TEMPLATESDIR   BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/templates" )
#  define INKSCAPE_UIDIR          BR_DATADIR( INKSCAPE_LIBPREFIX "/share/inkscape/ui" )
//CREATE V0.1 support
#    define CREATE_PAINTDIR       BR_DATADIR( INKSCAPE_LIBPREFIX "/share/create/paint" )
#    define CREATE_PALETTESDIR    BR_DATADIR( INKSCAPE_LIBPREFIX "/share/create/swatches" )
#elif defined ENABLE_OSX_APP_LOCATIONS // TODO: Is ENABLE_OSX_APP_LOCATIONS still in use?
#    define INKSCAPE_DATADIR_REAL "Contents/Resources/share"
#    define INKSCAPE_SYSTEMDIR    "Contents/Resources/share/inkscape"
#    define INKSCAPE_ATTRRELDIR   "Contents/Resources/share/inkscape/attributes"
#    define INKSCAPE_DOCDIR       "Contents/Resources/share/inkscape/doc"
#    define INKSCAPE_EXAMPLESDIR  "Contents/Resources/share/inkscape/examples"
#    define INKSCAPE_EXTENSIONDIR "Contents/Resources/share/inkscape/extensions"
#    define INKSCAPE_FILTERDIR    "Contents/Resources/share/inkscape/filters"
#    define INKSCAPE_FONTSDIR     "Contents/Resources/share/inkscape/fonts"
#    define INKSCAPE_KEYSDIR      "Contents/Resources/share/inkscape/keys"
#    define INKSCAPE_ICONSDIR     "Contents/Resources/share/inkscape/icons"
#    define INKSCAPE_PIXMAPSDIR   "Contents/Resources/share/inkscape/pixmaps"
#    define INKSCAPE_MARKERSDIR   "Contents/Resources/share/inkscape/markers"
#    define INKSCAPE_PAINTDIR     "Contents/Resources/share/inkscape/paint"
#    define INKSCAPE_PALETTESDIR  "Contents/Resources/share/inkscape/palettes"
#    define INKSCAPE_SCREENSDIR   "Contents/Resources/share/inkscape/screens"
#    define INKSCAPE_SYMBOLSDIR   "Contents/Resources/share/inkscape/symbols"
#    define INKSCAPE_THEMEDIR     "Contents/Resources/share/inkscape/themes"
#    define INKSCAPE_TUTORIALSDIR "Contents/Resources/share/inkscape/tutorials"
#    define INKSCAPE_TEMPLATESDIR "Contents/Resources/share/inkscape/templates"
#    define INKSCAPE_UIDIR        "Contents/Resources/share/inkscape/ui"
//CREATE V0.1 support
#    define CREATE_PAINTDIR      "/Library/Application Support/create/paint"
#    define CREATE_PALETTESDIR   "/Library/Application Support/create/swatches"
#else
#    define INKSCAPE_DATADIR_REAL append_inkscape_datadir()
#    define INKSCAPE_SYSTEMDIR    append_inkscape_datadir("inkscape")
#    define INKSCAPE_ATTRRELDIR   append_inkscape_datadir("inkscape/attributes")
#    define INKSCAPE_BINDDIR      append_inkscape_datadir("inkscape/bind")
#    define INKSCAPE_DOCDIR       append_inkscape_datadir("inkscape/doc")
#    define INKSCAPE_EXAMPLESDIR  append_inkscape_datadir("inkscape/examples")
#    define INKSCAPE_EXTENSIONDIR append_inkscape_datadir("inkscape/extensions")
#    define INKSCAPE_FILTERDIR    append_inkscape_datadir("inkscape/filters")
#    define INKSCAPE_FONTSDIR     append_inkscape_datadir("inkscape/fonts")
#    define INKSCAPE_KEYSDIR      append_inkscape_datadir("inkscape/keys")
#    define INKSCAPE_ICONSDIR     append_inkscape_datadir("inkscape/icons")
#    define INKSCAPE_PIXMAPSDIR   append_inkscape_datadir("inkscape/pixmaps")
#    define INKSCAPE_MARKERSDIR   append_inkscape_datadir("inkscape/markers")
#    define INKSCAPE_PAINTDIR     append_inkscape_datadir("inkscape/paint")
#    define INKSCAPE_PALETTESDIR  append_inkscape_datadir("inkscape/palettes")
#    define INKSCAPE_SCREENSDIR   append_inkscape_datadir("inkscape/screens")
#    define INKSCAPE_SYMBOLSDIR   append_inkscape_datadir("inkscape/symbols")
#    define INKSCAPE_THEMEDIR     append_inkscape_datadir("inkscape/themes")
#    define INKSCAPE_TUTORIALSDIR append_inkscape_datadir("inkscape/tutorials")
#    define INKSCAPE_TEMPLATESDIR append_inkscape_datadir("inkscape/templates")
#    define INKSCAPE_UIDIR        append_inkscape_datadir("inkscape/ui")
//CREATE V0.1 support
#    define CREATE_PAINTDIR       append_inkscape_datadir("create/paint")
#    define CREATE_PALETTESDIR    append_inkscape_datadir("create/swatches")
#endif


char *append_inkscape_datadir(const char *relative_path=nullptr);
char *get_program_name();
char *get_program_dir();


#endif /* _PATH_PREFIX_H_ */
