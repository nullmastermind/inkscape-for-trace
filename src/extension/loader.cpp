/*
 * Loader for external plug-ins.
 *
 * Authors:
 *   Moritz Eberl <moritz@semiodesk.com>
 *
 * Copyright (C) 2016 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "loader.h"
#include "system.h"
#include <exception>
#include <string.h>
#include "dependency.h"

namespace Inkscape {
namespace Extension {

typedef Implementation::Implementation *(*_getImplementation)(void);

bool Loader::LoadDependency(Dependency *dep)
{
    GModule *module = NULL;
    module = g_module_open(dep->getName(), (GModuleFlags)0);
    if (module == NULL) {
        return false;
    }
    return true;
}

/**
 * @brief Load the actual implementation of a plugin supplied by the plugin.
 * @param doc The xml representation of the INX extension configuration.
 * @return The implementation of the extension loaded from the plugin.
 */
Implementation::Implementation *Loader::LoadImplementation(Inkscape::XML::Document *doc)
{
    try {
        Inkscape::XML::Node *repr = doc->root();
        Inkscape::XML::Node *child_repr = repr->firstChild();
        while (child_repr != NULL) {
            char const *chname = child_repr->name();
            if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
                chname += strlen(INKSCAPE_EXTENSION_NS);
            }

            if (!strcmp(chname, "dependency")) {
                Dependency dep = Dependency(child_repr);
                bool success = LoadDependency(&dep);
                if( !success ){
                    const char *res = g_module_error();
                    g_warning("Unable to load dependency %s of plugin %s.\nDetails: %s\n", dep.getName(), res);
                    return NULL;
                }
            } 

            if (!strcmp(chname, "plugin")) {
                if (const gchar *name = child_repr->attribute("name")) {
                    GModule *module = NULL;
                    _getImplementation GetImplementation = NULL;
                    gchar *path = g_build_filename(_baseDirectory, name, (char *) NULL);
                    module = g_module_open(path, G_MODULE_BIND_LOCAL);
                    g_free(path);
                    if (module == NULL) {
                        const char *res = g_module_error();
                        g_warning("Unable to load extension %s.\nDetails: %s\n", name, res);
                        return NULL;
                    }

                    if (g_module_symbol(module, "GetImplementation", (gpointer *) &GetImplementation) == FALSE) {
                        const char *res = g_module_error();
                        g_warning("Unable to load extension %s.\nDetails: %s\n", name, res);
                        return NULL;
                    }

                    Implementation::Implementation *i = GetImplementation();
                    return i;
                }
            } 

            child_repr = child_repr->next();
        }
    } catch (std::exception &e) {
        g_warning("Unable to load extension.");
    }
    return NULL;
}

} // namespace Extension
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