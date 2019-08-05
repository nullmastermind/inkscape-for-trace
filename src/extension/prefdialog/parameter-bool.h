// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INK_EXTENSION_PARAMBOOL_H
#define SEEN_INK_EXTENSION_PARAMBOOL_H
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"

class SPDocument;

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace XML {
class Node;
}

namespace Extension {

/**
 * A boolean parameter.
 */
class ParamBool : public InxParameter {
public:
    ParamBool(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    /**
     * Returns the current state/value.
     */
    bool get(const SPDocument *doc, const Inkscape::XML::Node *node) const;

    /**
     * A function to set the state/value.
     * This function sets the internal value, but it also sets the value
     * in the preferences structure.  To put it in the right place, \c PREF_DIR
     * and \c pref_name() are used.
     *
     * @param  in   The value to set to
     * @param  doc  A document that should be used to set the value.
     * @param  node The node where the value may be placed
     */
    bool set(bool in, SPDocument *doc, Inkscape::XML::Node *node);

    /**
     * Creates a bool check button for a bool parameter.
     * Builds a hbox with a label and a check button in it.
     */
    Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal) override;

    /**
     * Appends 'true' or 'false'.
     * @todo investigate. Returning a value that can then be appended would probably work better/safer.
     */
    std::string value_to_string() const override;

private:
    /** Internal value. */
    bool _value = true;
};

}  // namespace Extension
}  // namespace Inkscape

#endif // SEEN_INK_EXTENSION_PARAMBOOL_H

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
