// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __INK_EXTENSION_PARAMDESCRIPTION_H__
#define __INK_EXTENSION_PARAMDESCRIPTION_H__

/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"

class SPDocument;

namespace Gtk {
	class Widget;
}

namespace Inkscape {
namespace Xml {
	class Node;
}

namespace Extension {

/** \brief  A description parameter */
class ParamDescription : public InxParameter {
public:
    enum AppearanceMode {
        DEFAULT, HEADER, URL
    };

    ParamDescription(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal) override;
private:
    /** \brief  Internal value. */
    Glib::ustring _value;

    /** appearance mode **/
    AppearanceMode _mode = DEFAULT;
};

}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* __INK_EXTENSION_PARAMDESCRIPTION_H__ */

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
