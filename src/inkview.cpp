/*
 * Inkscape - an ambitious vector drawing program
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Davide Puricelli <evo@debian.org>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Masatake YAMATO  <jet@gyve.org>
 *   F.J.Franklin <F.J.Franklin@sheffield.ac.uk>
 *   Michael Meeks <michael@helixcode.com>
 *   Chema Celorio <chema@celorio.com>
 *   Pawel Palucha
 * ... and various people who have worked with various projects
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Inkscape authors:
 *   Johan Ceuppens
 *
 * Copyright (C) 2004 Inkscape authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtkmm/main.h>

#include <libxml/tree.h>

#include <glibmm.h>
#include <glibmm/i18n.h>

#include "document.h"
#include "inkscape.h"
#include "preferences.h"
#ifdef ENABLE_NLS
#include "helper/gettext.h"
#endif
#include "inkgc/gc-core.h"
#include "io/sys.h"
#include "svg-view-slideshow.h"





/**
 * \brief Set of command-line options for Inkview
 */
class InkviewOptionsGroup : public Glib::OptionGroup
{
public:
    /// List of all input filenames
    Glib::OptionGroup::vecustrings filenames;

    /// Input timer option
    int timer = 0;

    InkviewOptionsGroup() : Glib::OptionGroup(N_("Inkscape Options"),
                                              N_("Default program options"))
    {
        // Entry for the "timer" option
        Glib::OptionEntry _entry_timer;
        _entry_timer.set_short_name('t');
        _entry_timer.set_long_name("timer");
        _entry_timer.set_arg_description(N_("NUM"));
        _entry_timer.set_description(N_("Change image every NUM seconds"));
        add_entry(_entry_timer, timer);

        // Entry for the remaining non-option arguments
        Glib::OptionEntry _entry_args;
        _entry_args.set_long_name(G_OPTION_REMAINING);
        _entry_args.set_arg_description(N_("FILES/FOLDERS …"));

        add_entry(_entry_args, filenames);
    }
};


/** get a list of valid SVG files from a list of strings */
std::vector<Glib::ustring> get_valid_files(std::vector<Glib::ustring> filenames, bool recursive = false)
{
    std::vector<Glib::ustring> valid_files;

    for(auto file : filenames)
    {
        if (!Inkscape::IO::file_test( file.c_str(), G_FILE_TEST_EXISTS )) {
            g_printerr("%s: %s\n", _("File or folder does not exist"), file.c_str());
        } else {
            if (Inkscape::IO::file_test( file.c_str(), G_FILE_TEST_IS_DIR )) {
                if (recursive) {
                    std::vector<Glib::ustring> new_filenames;
                    Glib::Dir directory(file);
                    for (auto new_file: directory) {
                        Glib::ustring extension = new_file.substr( new_file.find_last_of(".") + 1 );
                        if (!extension.compare("svg") || !extension.compare("svgz")) {
                            new_filenames.push_back(Glib::build_filename(file, new_file));
                        }
                    }
                    std::vector<Glib::ustring> new_files = get_valid_files(new_filenames);
                    valid_files.insert(valid_files.end(), new_files.begin(), new_files.end());
                }
            } else {
                auto doc = SPDocument::createNewDoc(file.c_str(), TRUE, false);
                if(doc) {
                    /* Append to list */
                    valid_files.push_back(file);
                } else {
                    g_printerr("%s: %s\n", _("Could not open file"), file.c_str());
                }
            }
        }
    }

    return valid_files;
}

#ifdef WIN32
// minimal print handler (just prints the string to stdout)
void g_print_no_convert(const gchar *buf) { fputs(buf, stdout); }
void g_printerr_no_convert(const gchar *buf) { fputs(buf, stderr); }
#endif

int main (int argc, char **argv)
{
#ifdef WIN32
    // Ugly hack to make g_print emit UTF-8 encoded characters. Otherwise glib will *always*
    // perform character conversion to the system's ANSI code page making UTF-8 output impossible.
    g_set_print_handler(g_print_no_convert);
    g_set_printerr_handler(g_print_no_convert);
#endif
#ifdef ENABLE_NLS
    Inkscape::initialize_gettext();
#endif

    Glib::OptionContext context(N_("- display SVG files"));
    context.set_summary(N_(
        "Quickly browse through a collection of .svg(z) files\n"
        "or show them as a slide show."));
    context.set_description(N_(
        "Example:\n"
        "  inkview -t 3 file1.svg file2.svgz series*.svg more_files"));
    context.set_translation_domain(GETTEXT_PACKAGE);

    InkviewOptionsGroup options;
    options.set_translation_domain(GETTEXT_PACKAGE);

    context.set_main_group(options);

    Gtk::Main main_instance (argc, argv, context);

    LIBXML_TEST_VERSION

    Inkscape::GC::init();
    Inkscape::Preferences::get(); // ensure preferences are initialized

    Inkscape::Application::create(argv[0], true);

    if(options.filenames.empty())
    {
        g_print("%s", context.get_help().c_str());
        exit(EXIT_FAILURE);
    }

    std::vector<Glib::ustring> valid_files = get_valid_files(options.filenames, true);
    if(valid_files.empty()) {
       return 1; /* none of the slides loadable */
    }

    SPSlideShow ss(valid_files, options.timer);
    main_instance.run();

    return 0;
}

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
