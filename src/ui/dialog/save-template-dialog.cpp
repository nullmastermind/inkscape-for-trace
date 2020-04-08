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
#include <gtkmm/builder.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/window.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

SaveTemplate::SaveTemplate(Gtk::Window &parent) {

    std::string gladefile = get_filename_string(Inkscape::IO::Resource::UIS, "dialog-save-template.glade");
    Glib::RefPtr<Gtk::Builder> builder;
    try {
        builder = Gtk::Builder::create_from_file(gladefile);
    } catch (const Glib::Error &ex) {
        g_warning("GtkBuilder file loading failed for save template dialog");
        return;
    }

    builder->get_widget("dialog", dialog);
    builder->get_widget("name", name);
    builder->get_widget("author", author);
    builder->get_widget("description", description);
    builder->get_widget("keywords", keywords);
    builder->get_widget("set-default", set_default_template);

    name->signal_changed().connect(sigc::mem_fun(*this, &SaveTemplate::on_name_changed));

    dialog->add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    dialog->add_button(_("Save"), Gtk::RESPONSE_OK);

    dialog->set_response_sensitive(Gtk::RESPONSE_OK, false);
    dialog->set_default_response(Gtk::RESPONSE_CANCEL);

    dialog->set_transient_for(parent);
    dialog->show_all();
}

void SaveTemplate::on_name_changed() {

    bool has_text = name->get_text_length() != 0;
    dialog->set_response_sensitive(Gtk::RESPONSE_OK, has_text);
}

void SaveTemplate::save_template(Gtk::Window &parent) {

    sp_file_save_template(parent, name->get_text(), author->get_text(), description->get_text(),
        keywords->get_text(), set_default_template->get_active());
}

void SaveTemplate::save_document_as_template(Gtk::Window &parent) {

    SaveTemplate dialog(parent);
    int response = dialog.dialog->run();

    switch (response) {
    case Gtk::RESPONSE_OK:
        dialog.save_template(parent);
        break;
    default:
        break;
    }

    dialog.dialog->close();
}

}
}
}
