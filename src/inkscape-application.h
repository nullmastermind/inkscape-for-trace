// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * The main Inkscape application.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INKSCAPE_APPLICATION_H
#define INKSCAPE_APPLICATION_H

/*
 * The main Inkscape application.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <gtkmm.h>

#include "document.h"
#include "selection.h"

#include "helper/action.h"
#include "io/file-export-cmd.h"   // File export (non-verb)

typedef std::vector<std::pair<std::string, Glib::VariantBase> > action_vector_t;

class InkscapeApplication : public Gtk::Application
{
protected:
    InkscapeApplication();

public:
    static Glib::RefPtr<InkscapeApplication> create();

    SPDocument* get_active_document();
    Inkscape::Selection* get_active_selection();

    InkFileExportCmd* file_export() { return &_file_export; }

protected:
    void on_startup()  override;
    void on_startup2();
    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

private:
    SPDesktop* create_window(const Glib::RefPtr<Gio::File>& file = Glib::RefPtr<Gio::File>());
    void parse_actions(const Glib::ustring& input, action_vector_t& action_vector);
    void shell();
    void shell2();

private:
    // Callbacks
    int  on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options);

    // Actions
    void on_new();
    void on_quit();
    void on_about();

    Glib::RefPtr<Gtk::Builder> _builder;

    bool _with_gui;
    bool _use_shell;
    InkFileExportCmd _file_export;

    // Documents are owned by the application which is responsible for opening/saving/exporting. WIP
    std::vector<SPDocument*> _documents;

    // Actions from the command line or file.
    action_vector_t _command_line_actions;
};

#endif // INKSCAPE_APPLICATION_H

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
