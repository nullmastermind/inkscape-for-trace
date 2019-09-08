// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkview - An SVG file viewer.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */


#include "inkview-window.h"

#include <iostream>

#include "document.h"

#include "ui/monitor.h"
#include "ui/view/svg-view-widget.h"

#include "util/units.h"

InkviewWindow::InkviewWindow(const Gio::Application::type_vec_files files,
                             bool fullscreen,
                             bool recursive,
                             int timer,
                             double scale,
                             bool preload
    )
    : _files(files)
    , _fullscreen(fullscreen)
    , _recursive(recursive)
    , _timer(timer)
    , _scale(scale)
    , _preload(preload)
    , _index(-1)
    , _view(nullptr)
    , _controlwindow(nullptr)
{
    _files = create_file_list(_files);

    if (_preload) {
        preload_documents();
    }

    if (_files.empty()) {
        throw NoValidFilesException();
    }

    _documents.resize( _files.size(), nullptr); // We keep _documents and _files in sync.

    // Callbacks
    signal_key_press_event().connect(sigc::mem_fun(*this, &InkviewWindow::key_press), false);

    if (_timer) {
        Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &InkviewWindow::on_timer), _timer);
    }

    // Actions
    add_action( "show_first", sigc::mem_fun(*this, &InkviewWindow::show_first) );
    add_action( "show_prev",  sigc::mem_fun(*this, &InkviewWindow::show_prev)  );
    add_action( "show_next",  sigc::mem_fun(*this, &InkviewWindow::show_next)  );
    add_action( "show_last",  sigc::mem_fun(*this, &InkviewWindow::show_last)  );

    // ToDo: Add Pause, Resume.

    if (_fullscreen) {
        Gtk::Window::fullscreen();
    }

    // Show first file
    activate_action( "show_first" );
}

std::vector<Glib::RefPtr<Gio::File> >
InkviewWindow::create_file_list(const std::vector<Glib::RefPtr<Gio::File > >& files)
{
    std::vector<Glib::RefPtr<Gio::File> > valid_files;

    static bool first = true;

    for (auto file : files) {
        Gio::FileType type = file->query_file_type();
        switch (type) {
            case Gio::FILE_TYPE_NOT_KNOWN:
                std::cerr << "InkviewWindow: File or directory does not exist: "
                          << file->get_basename() << std::endl;
                break;

            case Gio::FILE_TYPE_REGULAR:
            {
                // Only look at SVG and SVGZ files.
                std::string basename = file->get_basename();
                std::string extension = basename.substr(basename.find_last_of(".") + 1);
                if (extension == "svg" || extension == "svgz") {
                    valid_files.push_back(file);
                }
                break;
            }

            case Gio::FILE_TYPE_DIRECTORY:
            {
                if (_recursive || first) {
                    // No easy way to get children of directory!
                    Glib::RefPtr<Gio::FileEnumerator> children = file->enumerate_children();
                    Glib::RefPtr<Gio::FileInfo> info;
                    std::vector<Glib::RefPtr<Gio::File> > input;
                    while ((info = children->next_file())) {
                        input.push_back(children->get_child(info));
                    }
                    auto new_files = create_file_list(input);
                    valid_files.insert(valid_files.end(), new_files.begin(), new_files.end());
                }
                break;
            }
            default:
                std::cerr << "InkviewWindow: Unknown file type: " << type << std::endl;
        }
        first = false;
    }

    return valid_files;
}

void
InkviewWindow::update_title()
{
    Glib::ustring title(_documents[_index]->getDocumentName());

    if (_documents.size() > 1) {
        title += Glib::ustring::compose("  (%1/%2)", _index+1, _documents.size());
    }

    set_title(title);
}

// Returns true if successfully shows document.
bool
InkviewWindow::show_document(SPDocument* document)
{
    document->ensureUpToDate();  // Crashes on some documents if this isn't called!

    // Resize window:  (Might be better to use get_monitor_geometry_at_window(this))
    Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_primary();
    int width  = MIN((int)document->getWidth().value("px")  * _scale,  monitor_geometry.get_width());
    int height = MIN((int)document->getHeight().value("px") * _scale,  monitor_geometry.get_height());
    resize (width, height);

    if (_view) {
        _view->setDocument(document);
    } else {
        _view = Gtk::manage(new Inkscape::UI::View::SVGViewWidget(document));
        add (*_view);
    }

    update_title();

    return true;
}


// Load document, if fail, remove entry from lists.
SPDocument*
InkviewWindow::load_document()
{
    SPDocument* document = _documents[_index];

    if (!document) {
        // We need to load document. ToDo: Pass Gio::File. Is get_base_name() better?
        document = SPDocument::createNewDoc (_files[_index]->get_parse_name().c_str(), true, false);
        if (document) {
            // We've successfully loaded it!
            _documents[_index] = document;
        }
    }

    if (!document) {
        // Failed to load document, remove from vectors.
        _documents.erase(std::next(_documents.begin(), _index));
        _files.erase(    std::next(    _files.begin(), _index));
    }

    return document;
}



void
InkviewWindow::preload_documents()
{
    for (auto it =_files.begin(); it != _files.end(); ) {

        SPDocument* document =
            SPDocument::createNewDoc ((*it)->get_parse_name().c_str(), true, false);
        if (document) {
            _documents.push_back(document);
            ++it;
        } else {
            it = _files.erase(it);
        }
    }
}

static std::string window_markup = R"(
<interface>
  <object class="GtkWindow" id="ControlWindow">
    <child>
      <object class="GtkButtonBox">
        <child>
          <object class="GtkButton" id="show-first">
            <property name="visible">True</property>
            <property name="can_focus">True</property>
            <child>
              <object class="GtkImage">
                <property name="visible">True</property>
                <property name="icon_name">go-first</property>
              </object>
            </child>
          </object>
        </child>
        <child>
          <object class="GtkButton" id="show-prev">
            <property name="visible">True</property>
            <property name="can_focus">True</property>
            <child>
              <object class="GtkImage">
                <property name="visible">True</property>
                <property name="icon_name">go-previous</property>
              </object>
            </child>
          </object>
        </child>
        <child>
          <object class="GtkButton" id="show-next">
            <property name="visible">True</property>
            <property name="can_focus">False</property>
            <child>
              <object class="GtkImage">
                <property name="visible">True</property>
                <property name="icon_name">go-next</property>
              </object>
            </child>
          </object>
        </child>
        <child>
          <object class="GtkButton" id="show-last">
            <property name="visible">True</property>
            <property name="can_focus">False</property>
            <child>
              <object class="GtkImage">
                <property name="visible">True</property>
                <property name="icon_name">go-last</property>
              </object>
            </child>
          </object>
        </child>
      </object>
    </child>
  </object>
</interface>
)";

void
InkviewWindow::show_control()
{
    if (!_controlwindow) {

        auto builder = Gtk::Builder::create();
        try
        {
            builder->add_from_string(window_markup);
        }
        catch (const Glib::Error& err)
        {
            std::cerr << "InkviewWindow::show_control: builder failed: " << err.what() << std::endl;
            return;
        }


        builder->get_widget("ControlWindow", _controlwindow);
        if (!_controlwindow) {
            std::cerr << "InkviewWindow::show_control: Control Window not found!" << std::endl;
            return;
        }

        // Need to give control window access to viewer window's actions.
        Glib::RefPtr<Gio::ActionGroup> viewer = get_action_group("win");
        if (viewer) {
            _controlwindow->insert_action_group("viewer", viewer);
        }

        // Gtk::Button not derived from Gtk::Actionable due to ABI issues. Must use Gtk.
        // Fixed in Gtk4. In Gtk4 this can be replaced by setting the action in the interface.
        Gtk::Button* button;
        builder->get_widget("show-first", button);
        gtk_actionable_set_action_name( GTK_ACTIONABLE(button->gobj()), "viewer.show_first");
        builder->get_widget("show-prev", button);
        gtk_actionable_set_action_name( GTK_ACTIONABLE(button->gobj()), "viewer.show_prev");
        builder->get_widget("show-next", button);
        gtk_actionable_set_action_name( GTK_ACTIONABLE(button->gobj()), "viewer.show_next");
        builder->get_widget("show-last", button);
        gtk_actionable_set_action_name( GTK_ACTIONABLE(button->gobj()), "viewer.show_last");

        _controlwindow->set_resizable(false);
        _controlwindow->set_transient_for(*this);
        _controlwindow->show_all();

    } else {
        _controlwindow->present();
    }
}

// Next document
void
InkviewWindow::show_next()
{
    ++_index;

    SPDocument* document = nullptr;

    while (_index < _documents.size() && !document) {
        document = load_document();
    }

    if (document) {
        // Show new document
        show_document(document);
    } else {
        // Failed to load new document, keep current.
        --_index;
    }
}

// Previous document
void
InkviewWindow::show_prev()
{
    SPDocument* document = nullptr;
    int old_index = _index;

    while (_index > 0 && !document) {
        --_index;
        document = load_document();
    }

    if (document) {
        // Show new document
        show_document(document);
    } else {
        // Failed to load new document, keep current.
        _index = old_index;
    }
}

// Show first document
void
InkviewWindow::show_first()
{
    _index = -1;
    show_next();
}

// Show last document
void
InkviewWindow::show_last()
{
    _index = _documents.size();
    show_prev();
}

bool
InkviewWindow::key_press(GdkEventKey* event)
{
    switch (event->keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_Home:
            show_first();
            break;

        case GDK_KEY_Down:
        case GDK_KEY_End:
            show_last();
            break;

        case GDK_KEY_F11:
            if (_fullscreen) {
                unfullscreen();
                _fullscreen = false;
            } else {
                fullscreen();
                _fullscreen = true;
            }
            break;

        case GDK_KEY_Return:
            show_control();
            break;

        case GDK_KEY_KP_Page_Down:
        case GDK_KEY_Page_Down:
        case GDK_KEY_Right:
        case GDK_KEY_space:
            show_next();
            break;

        case GDK_KEY_KP_Page_Up:
        case GDK_KEY_Page_Up:
        case GDK_KEY_Left:
        case GDK_KEY_BackSpace:
            show_prev();
            break;

        case GDK_KEY_Escape:
        case GDK_KEY_q:
        case GDK_KEY_Q:
            close();
            break;

        default:
            break;
    }
    return false;
}

bool
InkviewWindow::on_timer()
{
    show_next();

    // Stop if at end.
    if (_index >= _documents.size() - 1) {
        return false;
    }
    return true;
}

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
