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

#include <vector>

#include "parameter.h"

namespace Gtk {
class Widget;
}

namespace Glib {
class ustring;
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
    private:
        /** A table to store the parameters for this page.
          * This only gets created if there are parameters on this page */
        std::vector<InxParameter *> parameters;
    public:
        ParamNotebookPage(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);
        ~ParamNotebookPage() override;

        Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal) override;
        void paramString (std::list <std::string> &list);
        gchar *get_text () {return _text;};
        InxParameter *get_param (const gchar *name) override;
    }; /* class ParamNotebookPage */

    /** A table to store the pages with parameters for this notebook.
      * This only gets created if there are pages in this notebook */
    std::vector<ParamNotebookPage*> pages;

public:
    ParamNotebook(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);
    ~ParamNotebook() override;

    Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal) override;

    /**
     * A function to get the currentpage and the parameters in a string form.
     * @return A string with the 'value' and all the parameters on all pages as command line arguments.
     */
    void string (std::list <std::string> &list) const override;

    // Explicitly call superclass version to avoid method being hidden.
    void string(std::string &string) const override {return InxParameter::string(string);}


    InxParameter *get_param (const gchar *name) override;

    const Glib::ustring& get (const SPDocument * /*doc*/, const Inkscape::XML::Node * /*node*/) { return _value; }
    const Glib::ustring& set (const int in, SPDocument *doc, Inkscape::XML::Node *node);
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
