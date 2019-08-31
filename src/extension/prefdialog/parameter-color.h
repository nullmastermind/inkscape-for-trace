// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INK_EXTENSION_PARAMCOLOR_H__
#define SEEN_INK_EXTENSION_PARAMCOLOR_H__
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"
#include "ui/selected-color.h"

namespace Gtk {
class Widget;
class ColorButton;
}

namespace Inkscape {
namespace XML {
class Node;
}

namespace Extension {

class ParamColor : public InxParameter {
public:
    enum AppearanceMode {
        DEFAULT, COLOR_BUTTON
    };

    ParamColor(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);
    ~ParamColor() override;

    /** Returns \c _value, with a \i const to protect it. */
    unsigned int get() const { return _color.value(); }

    unsigned int set(unsigned int in);

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;

    std::string value_to_string() const override;

    sigc::signal<void> *_changeSignal;

private:
    void _onColorChanged();
    void _onColorButtonChanged();

    /** Internal value of this parameter */
    Inkscape::UI::SelectedColor _color;

    sigc::connection _color_changed;
    sigc::connection _color_released;

    Gtk::ColorButton *_color_button;

    /** appearance mode **/
    AppearanceMode _mode = DEFAULT;
}; // class ParamColor

}  // namespace Extension
}  // namespace Inkscape

#endif // SEEN_INK_EXTENSION_PARAMCOLOR_H__

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
