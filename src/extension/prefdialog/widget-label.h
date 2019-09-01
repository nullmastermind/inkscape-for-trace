// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Description widget for extensions
 *//*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2005-2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INK_EXTENSION_WIDGET_LABEL_H
#define SEEN_INK_EXTENSION_WIDGET_LABEL_H

#include "widget.h"

#include <glibmm/ustring.h>

namespace Gtk {
	class Widget;
}

namespace Inkscape {
namespace Xml {
	class Node;
}

namespace Extension {

/** \brief  A label widget */
class WidgetLabel : public InxWidget {
public:
    enum AppearanceMode {
        DEFAULT, HEADER, URL
    };

    WidgetLabel(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;
private:
    /** \brief  Internal value. */
    Glib::ustring _value;

    /** appearance mode **/
    AppearanceMode _mode = DEFAULT;
};

}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* SEEN_INK_EXTENSION_WIDGET_LABEL_H */

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
