// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INK_EXTENSION_PARAMINT_H_SEEN
#define INK_EXTENSION_PARAMINT_H_SEEN

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

class ParamInt : public InxParameter {
public:
    enum AppearanceMode {
        DEFAULT, FULL
    };

    ParamInt(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    /** Returns \c _value. */
    int get() const { return _value; }

    int set(int in);

    int max () { return _max; }

    int min () { return _min; }

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;

    std::string value_to_string() const override;

private:
    /** Internal value. */
    int _value = 0;

    /** limits */
    // TODO: do these defaults make sense or should we be unbounded by default?
    int _min = 0;
    int _max = 10;

    /** appearance mode **/
    AppearanceMode _mode = DEFAULT;
};

}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* INK_EXTENSION_PARAMINT_H_SEEN */

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
