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

#include <glibmm/refptr.h>

namespace Gtk {
class Builder;
class CheckButton;
class Dialog;
class Entry;
class Window;
}

namespace Inkscape {
namespace UI {
namespace Dialog {

class SaveTemplate
{

public:

    static void save_document_as_template(Gtk::Window &parentWindow);

protected:

    void on_name_changed();

private:

    Gtk::Dialog *dialog;

    Gtk::Entry *name;
    Gtk::Entry *author;
    Gtk::Entry *description;
    Gtk::Entry *keywords;

    Gtk::CheckButton *set_default_template;

    SaveTemplate(Gtk::Window &parent);
    void save_template(Gtk::Window &parent);

};
}
}
}
#endif
