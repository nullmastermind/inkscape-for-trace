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
#include <windows.h> // provides SetDllDirectoryW
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

int main(int argc, char *argv[])
{
    set_extensions_env();

    if (gtk_init_check(NULL, NULL))
        return (ConcreteInkscapeApplication<Gtk::Application>::get_instance()).run(argc, argv);
    else
        return (ConcreteInkscapeApplication<Gio::Application>::get_instance()).run(argc, argv);
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
