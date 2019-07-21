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

namespace Inkscape {
namespace Extension {

class Extension;


/** A class to represent a notebookparameter of an extension. */
class ParamNotebook : public Parameter {
private:
    /**
     * Internal value.  This should point to a string that has
     * been allocated in memory.  And should be free'd.
     * It is the name of the current page.
     */
    gchar * _value;

    /**
     * A class to represent the pages of a notebookparameter of an extension.
     */
    class ParamNotebookPage : public Parameter {
    private:
        std::vector<Parameter *> parameters; /**< A table to store the parameters for this page.
                              This only gets created if there are parameters on this
                              page */

    public:
        static ParamNotebookPage * makepage (Inkscape::XML::Node * in_repr, Inkscape::Extension::Extension * in_ext);

        ParamNotebookPage(const gchar * name,
                      const gchar * text,
                      const gchar * description,
                      bool hidden,
                      Inkscape::Extension::Extension * ext,
                      Inkscape::XML::Node * xml);
        ~ParamNotebookPage() override;

        Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal) override;
        void paramString (std::list <std::string> &list);
        gchar * get_text () {return _text;};
        Parameter * get_param (const gchar * name) override;
    }; /* class ParamNotebookPage */


    std::vector<ParamNotebookPage*> pages; /**< A table to store the pages with parameters for this notebook.
                              This only gets created if there are pages in this
                              notebook */
public:
    ParamNotebook(const gchar * name,
                  const gchar * text,
                  const gchar * description,
                  bool hidden,
                  int indent,
                  Inkscape::Extension::Extension * ext,
                  Inkscape::XML::Node * xml);
    ~ParamNotebook() override;

    Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal) override;

    /**
     * A function to get the currentpage and the parameters in a string form.
     * @return A string with the 'value' and all the parameters on all pages as command line arguments.
     */
    void string (std::list <std::string> &list) const override;

    // Explicitly call superclass version to avoid method being hidden.
    void string(std::string &string) const override {return Parameter::string(string);}


    Parameter * get_param (const gchar * name) override;

    const gchar * get (const SPDocument * /*doc*/, const Inkscape::XML::Node * /*node*/) { return _value; }
    const gchar * set (const int in, SPDocument * doc, Inkscape::XML::Node * node);
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
