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


class InkscapeApplication
{
public:
    virtual void on_startup() = 0;
    virtual void on_startup2() = 0;
    virtual InkFileExportCmd* file_export() = 0;
    virtual int  on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options) = 0;
    virtual void on_new() = 0;
    virtual void on_quit() = 0;
    SPDocument* get_active_document();
    Inkscape::Selection* get_active_selection();

protected:
    bool _with_gui;
    bool _batch_process; // Temp
    bool _use_shell;
    InkscapeApplication();

    // Documents are owned by the application which is responsible for opening/saving/exporting. WIP
    std::vector<SPDocument*> _documents;

    InkFileExportCmd _file_export;

    // Actions from the command line or file.
    action_vector_t _command_line_actions;
};

// T can be either:
//   Gio::Application (window server is not present) or
//   Gtk::Application (window server is present).
// With Gtk::Application, one can still run "headless" by not creating any windows.
template <class T> class ConcreteInkscapeApplication : public T, public InkscapeApplication
{
public:
    ConcreteInkscapeApplication();

public:
    InkFileExportCmd* file_export() override { return &_file_export; }

protected:
    void on_startup()  override;
    void on_startup2() override;
    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;
    SPDesktop* create_window(const Glib::RefPtr<Gio::File>& file = Glib::RefPtr<Gio::File>());
    void parse_actions(const Glib::ustring& input, action_vector_t& action_vector);

private:
    // Callbacks
    int  on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options) override;

    // Actions
    void on_new() override;
    void on_quit() override;
    void on_about();
    
    void shell();
    void shell2();

    Glib::RefPtr<Gtk::Builder> _builder;

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
