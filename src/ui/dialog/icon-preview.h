// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A simple dialog for previewing icon representation.
 */
/* Authors:
 *   Jon A. Cruz
 *   Bob Jamison
 *   Other dudes from The Inkscape Organization
 *
 * Copyright (C) 2004,2005 The Inkscape Organization
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_ICON_PREVIEW_H
#define SEEN_ICON_PREVIEW_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/toggletoolbutton.h>

#include "ui/dialog/dialog-base.h"

class SPObject;
namespace Glib {
class Timer;
}

namespace Inkscape {
class Drawing;
namespace UI {
namespace Dialog {


/**
 * A panel that displays an icon preview
 */
class IconPreviewPanel : public DialogBase
{
public:
    IconPreviewPanel();
    //IconPreviewPanel(Glib::ustring const &label);
    ~IconPreviewPanel() override;

    static IconPreviewPanel& getInstance();

    void update() override;
    void refreshPreview();
    void modeToggled();

private:
    IconPreviewPanel(IconPreviewPanel const &) = delete; // no copy
    IconPreviewPanel &operator=(IconPreviewPanel const &) = delete; // no assign


    SPDesktop *desktop;
    SPDocument *document;
    Drawing *drawing;
    unsigned int visionkey;
    Glib::Timer *timer;
    Glib::Timer *renderTimer;
    bool pending;
    gdouble minDelay;

    Gtk::Box        iconBox;
    Gtk::Paned      splitter;
    Glib::ustring targetId;
    int hot;
    int numEntries;
    int* sizes;

    Gtk::Image      magnified;
    Gtk::Label      magLabel;

    Gtk::ToggleButton     *selectionButton;

    guchar** pixMem;
    Gtk::Image** images;
    Glib::ustring** labels;
    Gtk::ToggleToolButton** buttons;
    sigc::connection docModConn;


    void setDocument( SPDocument *document );
    void on_button_clicked(int which);
    void renderPreview( SPObject* obj );
    void updateMagnify();
    void queueRefresh();
    bool refreshCB();
};

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape



#endif // SEEN_ICON_PREVIEW_H

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
