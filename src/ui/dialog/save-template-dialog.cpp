// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "save-template-dialog.h"
#include "file.h"
#include "io/resource.h"

#include <glibmm/i18n.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

SaveTemplate::SaveTemplate() :
    Gtk::Dialog(_("Save Document as Template")),
    name_label(_("Name: "), Gtk::ALIGN_START),
    author_label(_("Author: "), Gtk::ALIGN_START),
    description_label(_("Description: "), Gtk::ALIGN_START),
    keywords_label(_("Keywords: "), Gtk::ALIGN_START),
    is_default_template(_("Set as default template"))
{
    resize(400, 200);

    name_text.set_hexpand(true);

    grid.attach(name_label, 0, 0, 1, 1);
    grid.attach(name_text, 1, 0, 1, 1);

    grid.attach(author_label, 0, 1, 1, 1);
    grid.attach(author_text, 1, 1, 1, 1);

    grid.attach(description_label, 0, 2, 1, 1);
    grid.attach(description_text, 1, 2, 1, 1);

    grid.attach(keywords_label, 0, 3, 1, 1);
    grid.attach(keywords_text, 1, 3, 1, 1);

    auto content_area = get_content_area();

    content_area->set_spacing(5);

    content_area->add(grid);
    content_area->add(is_default_template);

    name_text.signal_changed().connect( sigc::mem_fun(*this,
              &SaveTemplate::on_name_changed) );

    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    add_button("Save", Gtk::RESPONSE_OK);

    set_response_sensitive(Gtk::RESPONSE_OK, false);
    set_default_response(Gtk::RESPONSE_CANCEL);

    show_all();
}

void SaveTemplate::on_name_changed() {

    if (name_text.get_text_length() == 0) {

        set_response_sensitive(Gtk::RESPONSE_OK, false);
    } else {

        set_response_sensitive(Gtk::RESPONSE_OK, true);
    }
}

bool SaveTemplate::save_template(Gtk::Window &parentWindow) {

    return sp_file_save_template(parentWindow, name_text.get_text(),
        author_text.get_text(), description_text.get_text(),
        keywords_text.get_text(), is_default_template.get_active());
}

void SaveTemplate::save_document_as_template(Gtk::Window &parentWindow) {

    SaveTemplate dialog;

    auto operation_done = false;

    while (operation_done == false) {

        auto user_response = dialog.run();

        if (user_response == Gtk::RESPONSE_OK)
            operation_done = dialog.save_template(parentWindow);
         else
            operation_done = true;
    }
}

}
}
}
