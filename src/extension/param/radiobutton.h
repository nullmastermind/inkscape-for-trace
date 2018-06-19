#ifndef INK_EXTENSION_PARAMRADIOBUTTON_H_SEEN
#define INK_EXTENSION_PARAMRADIOBUTTON_H_SEEN

/** \file
 * Radiobutton parameter for extensions.
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

#include "parameter.h"

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace Extension {

class Extension;



// \brief  A class to represent a radiobutton parameter of an extension
class ParamRadioButton : public Parameter {
public:
    enum AppearanceMode {
        FULL, COMPACT, MINIMAL
    };

    ParamRadioButton(const gchar * name,
                     const gchar * text,
                     const gchar * description,
                     bool hidden,
                     int indent,
                     Inkscape::Extension::Extension * ext,
                     Inkscape::XML::Node * xml,
                     AppearanceMode mode);
    ~ParamRadioButton() override;

    Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal) override;

    // Explicitly call superclass version to avoid method being hidden.
    void string(std::list <std::string> &list) const override { return Parameter::string(list); }

    void string(std::string &string) const override;

    Glib::ustring value_from_label(const Glib::ustring label);

    const gchar *get(const SPDocument * /*doc*/, const Inkscape::XML::Node * /*node*/) const { return _value; }

    const gchar *set(const gchar *in, SPDocument *doc, Inkscape::XML::Node *node);

private:
    /** \brief  Internal value.  This should point to a string that has
                been allocated in memory.  And should be free'd.
                It is the value of the current selected string */
    gchar * _value;
    AppearanceMode _mode;

    /* For internal use only.
     	Note that value and text MUST be non-NULL. This is ensured by newing only at one location in the code where non-NULL checks are made. */
    class optionentry {
    public:
        optionentry (Glib::ustring * val, Glib::ustring * txt) {
            value = val;
            text = txt;
        }
        ~optionentry() {
            delete value;
            delete text;
        }
        Glib::ustring * value;
        Glib::ustring * text;
    };

    std::vector<optionentry*> choices; /**< A table to store the choice strings  */

}; /* class ParamRadioButton */





}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* INK_EXTENSION_PARAMRADIOBUTTON_H_SEEN */

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
