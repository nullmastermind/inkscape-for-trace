// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "input.h"

#include "timer.h"

#include "implementation/implementation.h"

#include "prefdialog/prefdialog.h"

#include "xml/repr.h"


/* Inkscape::Extension::Input */

namespace Inkscape {
namespace Extension {

/**
    \return   None
    \brief    Builds a SPModuleInput object from a XML description
    \param    module  The module to be initialized
    \param    repr    The XML description in a Inkscape::XML::Node tree

    Okay, so you want to build a SPModuleInput object.

    This function first takes and does the build of the parent class,
    which is SPModule.  Then, it looks for the <input> section of the
    XML description.  Under there should be several fields which
    describe the input module to excruciating detail.  Those are parsed,
    copied, and put into the structure that is passed in as module.
    Overall, there are many levels of indentation, just to handle the
    levels of indentation in the XML file.
*/
Input::Input (Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory)
    : Extension(in_repr, in_imp, base_directory)
{
    mimetype = nullptr;
    extension = nullptr;
    filetypename = nullptr;
    filetypetooltip = nullptr;

    if (repr != nullptr) {
        Inkscape::XML::Node * child_repr;

        child_repr = repr->firstChild();

        while (child_repr != nullptr) {
            if (!strcmp(child_repr->name(), INKSCAPE_EXTENSION_NS "input")) {
                child_repr = child_repr->firstChild();
                while (child_repr != nullptr) {
                    char const * chname = child_repr->name();
					if (!strncmp(chname, INKSCAPE_EXTENSION_NS_NC, strlen(INKSCAPE_EXTENSION_NS_NC))) {
						chname += strlen(INKSCAPE_EXTENSION_NS);
					}
                    if (chname[0] == '_') /* Allow _ for translation of tags */
                        chname++;
                    if (!strcmp(chname, "extension")) {
                        g_free (extension);
                        extension = g_strdup(child_repr->firstChild()->content());
                    }
                    if (!strcmp(chname, "mimetype")) {
                        g_free (mimetype);
                        mimetype = g_strdup(child_repr->firstChild()->content());
                    }
                    if (!strcmp(chname, "filetypename")) {
                        g_free (filetypename);
                        filetypename = g_strdup(child_repr->firstChild()->content());
                    }
                    if (!strcmp(chname, "filetypetooltip")) {
                        g_free (filetypetooltip);
                        filetypetooltip = g_strdup(child_repr->firstChild()->content());
                    }

                    child_repr = child_repr->next();
                }

                break;
            }

            child_repr = child_repr->next();
        }

    }

    return;
}

/**
    \return  None
    \brief   Destroys an Input extension
*/
Input::~Input ()
{
    g_free(mimetype);
    g_free(extension);
    g_free(filetypename);
    g_free(filetypetooltip);
    return;
}

/**
    \return  Whether this extension checks out
    \brief   Validate this extension

    This function checks to make sure that the input extension has
    a filename extension and a MIME type.  Then it calls the parent
    class' check function which also checks out the implementation.
*/
bool
Input::check ()
{
    if (extension == nullptr)
        return FALSE;
    if (mimetype == nullptr)
        return FALSE;

    return Extension::check();
}

/**
    \return  A new document
    \brief   This function creates a document from a file
    \param   uri  The filename to create the document from

    This function acts as the first step in creating a new document
    from a file.  The first thing that this does is make sure that the
    file actually exists.  If it doesn't, a NULL is returned.  If the
    file exits, then it is opened using the implementation of this extension.
*/
SPDocument *
Input::open (const gchar *uri)
{
    if (!loaded()) {
        set_state(Extension::STATE_LOADED);
    }
    if (!loaded()) {
        return nullptr;
    }
    timer->touch();

    SPDocument *const doc = imp->open(this, uri);

    return doc;
}

/**
    \return  IETF mime-type for the extension
    \brief   Get the mime-type that describes this extension
*/
gchar *
Input::get_mimetype()
{
    return mimetype;
}

/**
    \return  Filename extension for the extension
    \brief   Get the filename extension for this extension
*/
gchar *
Input::get_extension()
{
    return extension;
}

/**
    \return  The name of the filetype supported
    \brief   Get the name of the filetype supported
*/
const char *
Input::get_filetypename(bool translated)
{
    const char *name;

    if (filetypename)
        name = filetypename;
    else
        name = get_name();

    if (name && translated && filetypename) {
        return get_translation(name);
    } else {
        return name;
    }
}

/**
    \return  Tooltip giving more information on the filetype
    \brief   Get the tooltip for more information on the filetype
*/
const char *
Input::get_filetypetooltip(bool translated)
{
    if (filetypetooltip && translated) {
        return get_translation(filetypetooltip);
    } else {
        return filetypetooltip;
    }
}

/**
    \return  A dialog to get settings for this extension
    \brief   Create a dialog for preference for this extension

    Calls the implementation to get the preferences.
*/
bool
Input::prefs (const gchar *uri)
{
    if (!loaded()) {
        set_state(Extension::STATE_LOADED);
    }
    if (!loaded()) {
        return false;
    }

    Gtk::Widget * controls;
    controls = imp->prefs_input(this, uri);
    if (controls == nullptr) {
        // std::cout << "No preferences for Input" << std::endl;
        return true;
    }

    Glib::ustring name = this->get_name();
    PrefDialog *dialog = new PrefDialog(name, controls);
    int response = dialog->run();
    dialog->hide();

    delete dialog;

    return (response == Gtk::RESPONSE_OK);
}

} }  /* namespace Inkscape, Extension */

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
