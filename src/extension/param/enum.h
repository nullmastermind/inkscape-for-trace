#ifndef INK_EXTENSION_PARAMENUM_H_SEEN
#define INK_EXTENSION_PARAMENUM_H_SEEN

/** \file
 * Enumeration parameter for extensions.
 */

/*
 * Authors:
 *   Johan Engelen <johan@shouraizou.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2006-2007 Johan Engelen
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <vector>

#include "parameter.h"
#include "document.h"

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace Extension {

class Extension;


// \brief  A class to represent a notebookparameter of an extension
class ParamComboBox : public Parameter {
private:
    /** \brief  Internal value.  This should point to a string that has
                been allocated in memory.  And should be free'd.
                It is the value of the current selected string */
    gchar * _value;

    /* For internal use only. 
     * Note that value and text MUST be non-NULL. 
     * This is ensured by newing only at one location in the code where non-NULL checks are made. 
     */
    class enumentry {
    public:
        enumentry (Glib::ustring &val, Glib::ustring &text) :
            value(val),
            text(text)
        {}

        Glib::ustring value;
        Glib::ustring text;
    };

    std::vector<enumentry *> choices; /**< A table to store the choice strings  */

public:
    ParamComboBox(const gchar * name,
                  const gchar * text,
                  const gchar * description,
                  bool hidden,
                  int indent,
                  Inkscape::Extension::Extension * ext,
                  Inkscape::XML::Node * xml);
    ~ParamComboBox() override;

    Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal) override;

    // Explicitly call superclass version to avoid method being hidden.
    void string(std::list <std::string> &list) const override { return Parameter::string(list); }

    void string(std::string &string) const override;

    gchar const *get(SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/) const { return _value; }

    const gchar * set (const gchar * in, SPDocument * doc, Inkscape::XML::Node * node);

    /**
     * @returns true if text is part of this enum
     */
    bool contains(const gchar * text, SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/) const;

    void changed ();
}; /* class ParamComboBox */





}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* INK_EXTENSION_PARAMENUM_H_SEEN */

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
