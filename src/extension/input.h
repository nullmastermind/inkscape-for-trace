// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#ifndef INKSCAPE_EXTENSION_INPUT_H__
#define INKSCAPE_EXTENSION_INPUT_H__

#include <exception>
#include <glib.h>
#include "extension.h"

class SPDocument;

namespace Inkscape {
namespace Extension {

class Input : public Extension {
    gchar *mimetype;             /**< What is the mime type this inputs? */
    gchar *extension;            /**< The extension of the input files */
    gchar *filetypename;         /**< A userfriendly name for the file type */
    gchar *filetypetooltip;      /**< A more detailed description of the filetype */

public:
    struct open_failed : public std::exception {
        ~open_failed() noexcept override = default;
        const char *what() const noexcept override { return "Open failed"; }
    };
    struct no_extension_found : public std::exception {
        ~no_extension_found() noexcept override = default;
        const char *what() const noexcept override { return "No suitable input extension found"; }
    };
    struct open_cancelled : public std::exception {
        ~open_cancelled() noexcept override = default;
        const char *what() const noexcept override { return "Open was cancelled"; }
    };

    Input(Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory);
    ~Input() override;

    bool check() override;

    SPDocument *  open                 (gchar const *uri);
    gchar *       get_mimetype         ();
    gchar *       get_extension        ();
    const char *  get_filetypename     (bool translated=false);
    const char *  get_filetypetooltip  (bool translated=false);
    bool          prefs                (gchar const *uri);
};

} }  /* namespace Inkscape, Extension */
#endif /* INKSCAPE_EXTENSION_INPUT_H__ */

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
