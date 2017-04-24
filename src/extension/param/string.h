#ifndef INK_EXTENSION_PARAMSTRING_H_SEEN
#define INK_EXTENSION_PARAMSTRING_H_SEEN

/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "parameter.h"

namespace Inkscape {
namespace Extension {

class ParamString : public Parameter {
private:
    /** \brief  Internal value.  This should point to a string that has
                been allocated in memory.  And should be free'd. */
    gchar * _value;
    /** \brief Internal value. This indicates the maximum lenth of the string. Zero meaning unlimited.
      */
    gint _max_length;
public:
    ParamString(const gchar * name,
                const gchar * text,
                const gchar * description,
                bool hidden,
                int indent,
                Inkscape::Extension::Extension * ext,
                Inkscape::XML::Node * xml);
    virtual ~ParamString(void);

    /** \brief  Returns \c _value, with a \i const to protect it. */
    const gchar *get(SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/) const { return _value; }

    const gchar * set (const gchar * in, SPDocument * doc, Inkscape::XML::Node * node);

    Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal);

    // Explicitly call superclass version to avoid method being hidden.
    virtual void string(std::list <std::string> &list) const { return Parameter::string(list); }

    virtual void string(std::string &string) const;

    void setMaxLength(int maxLenght) { _max_length = maxLenght; }
    int getMaxLength(void) { return _max_length; }
};


}  // namespace Extension
}  // namespace Inkscape

#endif /* INK_EXTENSION_PARAMSTRING_H_SEEN */

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
