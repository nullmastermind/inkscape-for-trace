// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape - an ambitious vector drawing program
 *
 * Authors:
 * Tavmjong Bah
 *
 * (C) 2018 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef _WIN32
#include <windows.h> // SetDllDirectoryW, SetConsoleOutputCP
#include <fcntl.h> // _O_BINARY
#endif

#include "inkscape-application.h"
#include "path-prefix.h"

#include "io/resource.h"

static void set_extensions_env()
{
    // add inkscape to PATH, so the correct version is always available to extensions by simply calling "inkscape"
    gchar *program_dir = get_program_dir();
    if (program_dir) {
        gchar const *path = g_getenv("PATH");
        gchar *new_path = g_strdup_printf("%s" G_SEARCHPATH_SEPARATOR_S "%s", program_dir, path);
        g_setenv("PATH", new_path, true);
        g_free(new_path);
    }
    g_free(program_dir);

    // add various locations to PYTHONPATH so extensions find their modules
    auto extensiondir_user = get_path_ustring(Inkscape::IO::Resource::USER, Inkscape::IO::Resource::EXTENSIONS);
    auto extensiondir_system = get_path_ustring(Inkscape::IO::Resource::SYSTEM, Inkscape::IO::Resource::EXTENSIONS);

    auto pythonpath = extensiondir_user + G_SEARCHPATH_SEPARATOR + extensiondir_system;

    auto pythonpath_old = Glib::getenv("PYTHONPATH");
    if (!pythonpath_old.empty()) {
        pythonpath += G_SEARCHPATH_SEPARATOR + pythonpath_old;
    }

    pythonpath += G_SEARCHPATH_SEPARATOR + Glib::build_filename(extensiondir_system, "inkex", "deprecated-simple");

    Glib::setenv("PYTHONPATH", pythonpath);

    // Python 2.x attempts to encode output as ASCII by default when sent to a pipe.
    Glib::setenv("PYTHONIOENCODING", "UTF-8");

#ifdef _WIN32
    // add inkscape directory to DLL search path so dynamically linked extension modules find their libraries
    // should be fixed in Python 3.8 (https://github.com/python/cpython/commit/2438cdf0e932a341c7613bf4323d06b91ae9f1f1)
    gchar *installation_dir = get_program_dir();
    wchar_t *installation_dir_w = (wchar_t *)g_utf8_to_utf16(installation_dir, -1, NULL, NULL, NULL);
    SetDllDirectoryW(installation_dir_w);
    g_free(installation_dir);
    g_free(installation_dir_w);
#endif
}

#ifdef __APPLE__
static void set_macos_app_bundle_env(gchar const *program_dir)
{
    std::string bundle_contents_dir;
    bundle_contents_dir.assign(program_dir).append("/.."); // <TheApp.app>/Contents

    // use bundle identifier
    // https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/MacOSXDirectories/MacOSXDirectories.html
    auto app_support_dir = Glib::getenv("HOME") + "/Library/Application Support/org.inkscape.Inkscape";

    auto bundle_resources_dir       = bundle_contents_dir  + "/Resources";
    auto bundle_resources_etc_dir   = bundle_resources_dir + "/etc";
    auto bundle_resources_bin_dir   = bundle_resources_dir + "/bin";
    auto bundle_resources_lib_dir   = bundle_resources_dir + "/lib";
    auto bundle_resources_share_dir = bundle_resources_dir + "/share";

    // failsafe: Check if the expected content is really there, using GIO modules
    // as an indicator.
    // This is also helpful to developers as it enables the possibility to
    //      1. cmake -DCMAKE_INSTALL_PREFIX=Inkscape.app/Contents/Resources
    //      2. move binary to Inkscape.app/Contents/MacOS and set rpath
    //      3. copy Info.plist
    // to ease up on testing and get correct application behavior (like dock icon).
    if (!Glib::file_test(bundle_resources_lib_dir + "/gio/modules", Glib::FILE_TEST_EXISTS)) {
        // doesn't look like a standalone bundle
        return;
    }

    // XDG
    // https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
    Glib::setenv("XDG_DATA_HOME",   app_support_dir + "/share");
    Glib::setenv("XDG_DATA_DIRS",   bundle_resources_share_dir);
    Glib::setenv("XDG_CONFIG_HOME", app_support_dir + "/config");
    Glib::setenv("XDG_CONFIG_DIRS", bundle_resources_etc_dir + "/xdg");
    Glib::setenv("XDG_CACHE_HOME",  app_support_dir + "/cache");

    // GTK
    // https://developer.gnome.org/gtk3/stable/gtk-running.html
    Glib::setenv("GTK_EXE_PREFIX",  bundle_resources_dir);
    Glib::setenv("GTK_DATA_PREFIX", bundle_resources_dir);

    // GDK
    Glib::setenv("GDK_PIXBUF_MODULE_FILE", bundle_resources_lib_dir + "/gdk-pixbuf-2.0/2.10.0/loaders.cache");

    // Inkscape
    Glib::setenv("INKSCAPE_LOCALEDIR", bundle_resources_share_dir + "/locale");

    // fontconfig
    Glib::setenv("FONTCONFIG_PATH", bundle_resources_etc_dir + "/fonts");

    // GIO
    Glib::setenv("GIO_MODULE_DIR", bundle_resources_lib_dir + "/gio/modules");

    // GNOME introspection
    Glib::setenv("GI_TYPELIB_PATH", bundle_resources_lib_dir + "/girepository-1.0");

    // PATH
    Glib::setenv("PATH", bundle_resources_bin_dir + ":" + Glib::getenv("PATH"));

    // DYLD_LIBRARY_PATH
    // This is required to make Python GTK bindings work as they use dlopen()
    // to load libraries.
    Glib::setenv("DYLD_LIBRARY_PATH", bundle_resources_lib_dir + ":"
            + bundle_resources_lib_dir + "/gdk-pixbuf-2.0/2.10.0/loaders");
}
#endif

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    {   // Check if we're inside an application bundle and adjust environment
        // accordingly.

        gchar *program_dir = get_program_dir();
        if (g_str_has_suffix(program_dir, "Contents/MacOS")) {

            // Step 1
            // Remove macOS session identifier from command line arguments.
            // Code adopted from GIMP's app/main.c

            int new_argc = 0;
            for (int i = 0; i < argc; i++) {
                // Rewrite argv[] without "-psn_..." argument.
                if (!g_str_has_prefix(argv[i], "-psn_")) {
                    argv[new_argc] = argv[i];
                    new_argc++;
                }
            }
            if (argc > new_argc) {
                argv[new_argc] = nullptr; // glib expects null-terminated array
                argc = new_argc;
            }

            // Step 2
            // In the past, a launch script/wrapper was used to setup necessary environment
            // variables to facilitate relocatability for the application bundle. Starting
            // with Catalina, this approach is no longer feasible due to new security checks
            // that get misdirected by using a launcher. The launcher needs to go and the
            // binary needs to setup the environment itself.

            set_macos_app_bundle_env(program_dir);
        }

        g_free(program_dir);
    }
#elif defined _WIN32
    // temporarily switch console encoding to UTF8 while Inkscape runs
    // as everything else is a mess and it seems to work just fine
    const unsigned int initial_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    fflush(stdout); // empty buffer, just to be safe (see warning in documentation for _setmode)
    _setmode(_fileno(stdout), _O_BINARY); // binary mode seems required for this to work properly
#endif

    set_extensions_env();

    int ret;
    if (gtk_init_check(NULL, NULL)) {
        g_set_prgname("org.inkscape.Inkscape");
        ret = (ConcreteInkscapeApplication<Gtk::Application>::get_instance()).run(argc, argv);
    } else {
        ret = (ConcreteInkscapeApplication<Gio::Application>::get_instance()).run(argc, argv);
    }

#ifdef _WIN32
    // switch back to initial console encoding
    SetConsoleOutputCP(initial_cp);
#endif

    return ret;
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
