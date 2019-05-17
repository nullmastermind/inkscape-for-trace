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

    // add share/inkscape/extensions to PYTHONPATH so the inkex module is found by extensions in user folder
    auto new_pythonpath = std::string(INKSCAPE_EXTENSIONDIR);

    // add old PYTHONPATH
    gchar const *pythonpath = g_getenv("PYTHONPATH");
    if (pythonpath) {
        new_pythonpath.append(G_SEARCHPATH_SEPARATOR_S).append(pythonpath);
    }

    // add share/inkscape/extensions/inkex/deprecated-simple
    new_pythonpath.append(G_SEARCHPATH_SEPARATOR_S)
        .append(INKSCAPE_EXTENSIONDIR)
        .append(G_DIR_SEPARATOR_S)
        .append("inkex")
        .append(G_DIR_SEPARATOR_S)
        .append("deprecated-simple");

    g_setenv("PYTHONPATH", new_pythonpath.c_str(), true);

    // Python 2.x attempts to encode output as ASCII by default when sent to a pipe.
    g_setenv("PYTHONIOENCODING", "UTF-8", true);

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
