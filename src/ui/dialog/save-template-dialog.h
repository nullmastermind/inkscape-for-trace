// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SEEN_UI_DIALOG_SAVE_TEMPLATE_H
#define INKSCAPE_SEEN_UI_DIALOG_SAVE_TEMPLATE_H

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/checkbutton.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

class SaveTemplate : public Gtk::Dialog
{

public:

    static void save_document_as_template(Gtk::Window &parentWindow);

protected:

    void on_name_changed();

private:

    Gtk::Grid grid;

    Gtk::Label name_label;
    Gtk::Entry name_text;

    Gtk::Label author_label;
    Gtk::Entry author_text;

    Gtk::Label description_label;
    Gtk::Entry description_text;

    Gtk::Label keywords_label;
    Gtk::Entry keywords_text;

    Gtk::CheckButton is_default_template;

    SaveTemplate();
    bool save_template(Gtk::Window &parentWindow);

};
}
}
}
#endif
