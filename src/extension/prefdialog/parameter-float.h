// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INK_EXTENSION_PARAMFLOAT_H_SEEN
#define INK_EXTENSION_PARAMFLOAT_H_SEEN

/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace XML {
class Node;
}

namespace Extension {

class ParamFloat : public InxParameter {
public:
    enum AppearanceMode {
        DEFAULT, FULL
    };

    ParamFloat(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    /** Returns \c _value. */
    float get() const { return _value; }

    float set(float in);

    float max () { return _max; }

    float min () { return _min; }

    float precision () { return _precision; }

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;

    std::string value_to_string() const override;

private:
    /** Internal value. */
    float _value = 0;

    /** limits */
    // TODO: do these defaults make sense or should we be unbounded by default?
    float _min = 0;
    float _max = 10;

    /** numeric precision (i.e. number of digits) */
    int _precision = 1;

    /** appearance mode **/
    AppearanceMode _mode = DEFAULT;
};

}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* INK_EXTENSION_PARAMFLOAT_H_SEEN */

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
