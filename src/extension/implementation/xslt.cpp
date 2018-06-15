/** \file
 * Code for handling XSLT extensions.
 */
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006-2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/fileutils.h>
#include "file.h"
#include "xslt.h"
#include "../extension.h"
#include "../output.h"
#include "extension/input.h"

#include "io/resource.h"
#include <unistd.h>
#include <cstring>
#include "document.h"

#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

Inkscape::XML::Document * sp_repr_do_read (xmlDocPtr doc, const gchar * default_ns);

/* Namespaces */
namespace Inkscape {
namespace Extension {
namespace Implementation {

/* Real functions */
/**
    \return    A XSLT object
    \brief     This function creates a XSLT object and sets up the
               variables.

*/
XSLT::XSLT(void) :
    Implementation(),
    _filename(""),
    _parsedDoc(nullptr),
    _stylesheet(nullptr)
{
}

Glib::ustring XSLT::solve_reldir(Inkscape::XML::Node *reprin) {

    gchar const *s = reprin->attribute("reldir");

    if (!s) {
        Glib::ustring str = reprin->firstChild()->content();
        return str;
    }

    Glib::ustring reldir = s;
    if (reldir == "extensions") {
        using namespace Inkscape::IO::Resource;
        return get_filename(EXTENSIONS, reprin->firstChild()->content());
    } else {
        Glib::ustring str = reprin->firstChild()->content();
        return str;
    }
}

bool XSLT::check(Inkscape::Extension::Extension *module)
{
    if (load(module)) {
        unload(module);
        return true;
    } else {
        return false;
    }
}

bool XSLT::load(Inkscape::Extension::Extension *module)
{
    if (module->loaded()) { return true; }

    Inkscape::XML::Node *child_repr = module->get_repr()->firstChild();
    while (child_repr != nullptr) {
        if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "xslt")) {
            child_repr = child_repr->firstChild();
            while (child_repr != nullptr) {
                if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "file")) {
                    _filename = solve_reldir(child_repr);
                }
                child_repr = child_repr->next();
            }

            break;
        }
        child_repr = child_repr->next();
    }

    _parsedDoc = xmlParseFile(_filename.c_str());
    if (_parsedDoc == nullptr) { return false; }

    _stylesheet = xsltParseStylesheetDoc(_parsedDoc);

    return true;
}

void XSLT::unload(Inkscape::Extension::Extension *module)
{
    if (!module->loaded()) { return; }
    xsltFreeStylesheet(_stylesheet);
    // No need to use xmlfreedoc(_parsedDoc), it's handled by xsltFreeStylesheet(_stylesheet);
    return;
}

SPDocument * XSLT::open(Inkscape::Extension::Input */*module*/,
                        gchar const *filename)
{
    xmlDocPtr filein = xmlParseFile(filename);
    if (filein == nullptr) { return nullptr; }

    const char * params[1];
    params[0] = nullptr;

    xmlDocPtr result = xsltApplyStylesheet(_stylesheet, filein, params);
    xmlFreeDoc(filein);

    Inkscape::XML::Document * rdoc = sp_repr_do_read( result, SP_SVG_NS_URI);
    xmlFreeDoc(result);

    if (rdoc == nullptr) {
        return nullptr;
    }

    if (strcmp(rdoc->root()->name(), "svg:svg") != 0) {
        return nullptr;
    }

    gchar * base = nullptr;
    gchar * name = nullptr;
    gchar * s = nullptr, * p = nullptr;
    s = g_strdup(filename);
    p = strrchr(s, '/');
    if (p) {
        name = g_strdup(p + 1);
        p[1] = '\0';
        base = g_strdup(s);
    } else {
        base = nullptr;
        name = g_strdup(filename);
    }
    g_free(s);

    SPDocument * doc = SPDocument::createDoc(rdoc, filename, base, name, true, nullptr);

    g_free(base); g_free(name);

    return doc;
}

void XSLT::save(Inkscape::Extension::Output *module, SPDocument *doc, gchar const *filename)
{
    /* TODO: Should we assume filename to be in utf8 or to be a raw filename?
     * See JavaFXOutput::save for discussion. */
    g_return_if_fail(doc != nullptr);
    g_return_if_fail(filename != nullptr);

    Inkscape::XML::Node *repr = doc->getReprRoot();

    std::string tempfilename_out;
    int tempfd_out = 0;
    try {
        tempfd_out = Glib::file_open_tmp(tempfilename_out, "ink_ext_XXXXXX");
    } catch (...) {
        /// \todo Popup dialog here
        return;
    }

    if (!sp_repr_save_rebased_file(repr->document(), tempfilename_out.c_str(), SP_SVG_NS_URI,
                                   doc->getBase(), filename)) {
        throw Inkscape::Extension::Output::save_failed();
    }

    xmlDocPtr svgdoc = xmlParseFile(tempfilename_out.c_str());
    close(tempfd_out);
    if (svgdoc == nullptr) {
        return;
    }

    std::list<std::string> params;
    module->paramListString(params);
    const int max_parameters = params.size() * 2;
    const char * xslt_params[max_parameters+1] ;

    int count = 0;
    for(std::list<std::string>::iterator t=params.begin(); t != params.end(); ++t) {
        std::size_t pos = t->find("=");
        std::ostringstream parameter;
        std::ostringstream value;
        parameter << t->substr(2,pos-2);
        value << t->substr(pos+1);
        xslt_params[count++] = g_strdup_printf("%s", parameter.str().c_str());
        xslt_params[count++] = g_strdup_printf("'%s'", value.str().c_str());
    }
    xslt_params[count] = nullptr;

    xmlDocPtr newdoc = xsltApplyStylesheet(_stylesheet, svgdoc, xslt_params);
    //xmlSaveFile(filename, newdoc);
    int success = xsltSaveResultToFilename(filename, newdoc, _stylesheet, 0);

    xmlFreeDoc(newdoc);
    xmlFreeDoc(svgdoc);

    xsltCleanupGlobals();
    xmlCleanupParser();

    if (success < 1) {
        throw Inkscape::Extension::Output::save_failed();
    }

    return;
}


}  /* Implementation  */
}  /* module  */
}  /* Inkscape  */


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
