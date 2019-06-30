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

class InkscapeWindow;
class SPDocument;
class SPDesktop;

class InkscapeApplication
{
public:
    virtual void on_startup() = 0;
    virtual void on_startup2() = 0;
    virtual InkFileExportCmd* file_export() = 0;
    virtual int  on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options) = 0;
    virtual void on_new() = 0;
    virtual void on_quit() = 0;

    // Gio::Actions need to know what document, selection, view to work on.
    // In headless mode, these are set for each file processed.
    // With GUI, these are set everytime the cursor enters an InkscapeWindow.
    SPDocument*           get_active_document() { return _active_document; };
    void                  set_active_document(SPDocument* document) { _active_document = document; };

    Inkscape::Selection*  get_active_selection() { return _active_selection; }
    void                  set_active_selection(Inkscape::Selection* selection)
                                                               {_active_selection = selection;};

    // A view should track selection and canvas to document transform matrix. This is partially
    // redundant with the selection functions above. Maybe we should get rid of view altogether.
    Inkscape::UI::View::View* get_active_view() { return _active_view; }
    void                  set_active_view(Inkscape::UI::View::View* view) { _active_view = view; }

    /****** Document ******/
    /* Except for document_fix(), these should not require a GUI! */
    void                  document_add(SPDocument* document);

    SPDocument*           document_new(const std::string &Template);
    SPDocument*           document_open(const Glib::RefPtr<Gio::File>& file);
    SPDocument*           document_open(const std::string& data);
    bool                  document_swap(InkscapeWindow* window, SPDocument* document);
    bool                  document_revert(SPDocument* document);
    void                  document_close(SPDocument* document);
    unsigned              document_window_count(SPDocument* document);

    void                  document_fix(InkscapeWindow* window);

    /******* Window *******/
    InkscapeWindow*       window_open(SPDocument* document);
    void                  window_close(InkscapeWindow* window);

    // Update all windows connected to a document.
    void                  windows_update(SPDocument* document);

    /******* Debug ********/
    void                  dump();

    // These are needed to cast Glib::RefPtr<Gtk::Application> to Glib::RefPtr<InkscapeApplication>,
    // Presumably, Gtk/Gio::Application takes care of ref counting in ConcreteInkscapeApplication
    // so we just provide dummies (and there is only one application in the application!).
    // void reference()   { /*printf("reference()\n"  );*/ }
    // void unreference() { /*printf("unreference()\n");*/ }

protected:
    bool _with_gui;
    bool _batch_process; // Temp
    bool _use_shell;
    bool _use_pipe;
    int _pdf_page;
    int _pdf_poppler;
    InkscapeApplication();

    // Documents are owned by the application which is responsible for opening/saving/exporting. WIP
    // std::vector<SPDocument*> _documents;   For a true headless version
    std::map<SPDocument*, std::vector<InkscapeWindow*> > _documents;

    // We keep track of these things so we don't need a window to find them (for headless operation).
    SPDocument*               _active_document;
    Inkscape::Selection*      _active_selection;
    Inkscape::UI::View::View* _active_view;

    InkFileExportCmd _file_export;

    // Actions from the command line or file.
    action_vector_t _command_line_actions;
};

// T can be either:
//   Gio::Application (window server is not present, required for CI testing) or
//   Gtk::Application (window server is present).
// With Gtk::Application, one can still run "headless" by not creating any windows.
template <class T> class ConcreteInkscapeApplication : public T, public InkscapeApplication
{
public:
    static ConcreteInkscapeApplication<T>& get_instance();

private:
    ConcreteInkscapeApplication();

public:
    InkFileExportCmd* file_export() override { return &_file_export; }
    SPDesktop* create_window(const Glib::RefPtr<Gio::File>& file = Glib::RefPtr<Gio::File>(),
                             bool add_to_recent = true, bool replace_empty = true);
    bool       destroy_window(InkscapeWindow* window);
    void       destroy_all();

protected:
    void on_startup()  override;
    void on_startup2() override;
    void on_activate() override;
    void process(SPDocument* document, std::string output_path);
    void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;
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

    void _start_main_option_section(const Glib::ustring& section_name);

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
