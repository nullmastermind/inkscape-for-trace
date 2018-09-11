// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Inkscape About box
 *
 * The standard Gnome::UI::About class doesn't include a place to stuff
 * a renderable View that holds the classic Inkscape "about.svg".
 */
/* Author:
 *   Kees Cook <kees@outflux.net>
 *
 * Copyright (C) 2005 Kees Cook
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_ABOUTBOX_H
#define INKSCAPE_UI_DIALOG_ABOUTBOX_H

#include <gtkmm/aboutdialog.h>

namespace Gtk {
class AspectFrame;
}

namespace Inkscape {
namespace UI {
namespace Dialog {

class AboutBox : public Gtk::AboutDialog {

public:

    static void show_about();
    static void hide_about();

private:

    AboutBox();

    /** A widget containing an SVG "splash screen"
     *  image to display in the content area of the dialo
     */
    Gtk::AspectFrame *_splash_widget;
    
    void initStrings();
    void build_splash_widget();

    void on_response(int response_id) override;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_ABOUTBOX_H

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
