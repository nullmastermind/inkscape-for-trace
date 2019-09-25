// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INK_EXTENSION_PARAMNOTEBOOK_H_SEEN
#define INK_EXTENSION_PARAMNOTEBOOK_H_SEEN

/** \file
 * Notebook parameter for extensions.
 */

/*
 * Author:
 *   Johan Engelen <johan@shouraizou.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2006 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"

#include <vector>

#include <glibmm/ustring.h>


namespace Gtk {
class Widget;
}


namespace Inkscape {
namespace Extension {

class Extension;


/** A class to represent a notebook parameter of an extension. */
class ParamNotebook : public InxParameter {
private:
    /** Internal value. */
    Glib::ustring _value;

    /**
     * A class to represent the pages of a notebook parameter of an extension.
     */
    class ParamNotebookPage : public InxParameter {
        friend class ParamNotebook;
    public:
        ParamNotebookPage(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

        Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;

        // ParamNotebookPage is not a real parameter (it has no value), so make sure it does not return one
        std::string value_to_string() const override { return ""; };
    }; /* class ParamNotebookPage */

public:
    ParamNotebook(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;

    std::string value_to_string() const override;

    const Glib::ustring& get() { return _value; }
    const Glib::ustring& set(const int in);
}; /* class ParamNotebook */





}  // namespace Extension
}  // namespace Inkscape

#endif /* INK_EXTENSION_PARAMNOTEBOOK_H_SEEN */

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
