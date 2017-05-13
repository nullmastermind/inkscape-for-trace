/*
 * Inkscape - an ambitious vector drawing program
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   Davide Puricelli <evo@debian.org>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Masatake YAMATO  <jet@gyve.org>
 *   F.J.Franklin <F.J.Franklin@sheffield.ac.uk>
 *   Michael Meeks <michael@helixcode.com>
 *   Chema Celorio <chema@celorio.com>
 *   Pawel Palucha
 * ... and various people who have worked with various projects
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Inkscape authors:
 *   Johan Ceuppens
 *
 * Copyright (C) 2004 Inkscape authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstring>
#include <locale.h>


#include <gtkmm/applicationwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/image.h>
#include <gtkmm/main.h>

#include <libxml/tree.h>
#include <gdk/gdkkeysyms.h>

#include "inkgc/gc-core.h"
#include "preferences.h"

#include <glibmm.h>
#include <glibmm/i18n.h>

#include "document.h"
#include "svg-view.h"
#include "svg-view-widget.h"

#include "io/sys.h"
#include "util/units.h"
#ifdef ENABLE_NLS
#include "helper/gettext.h"
#endif


#include "inkscape.h"

#include "ui/icon-names.h"

class SPSlideShow;

static int sp_svgview_main_delete (GtkWidget *widget,
                                   GdkEvent *event,
                                   struct SPSlideShow *ss);

static int sp_svgview_main_key_press (GtkWidget *widget, 
                                      GdkEventKey *event,
                                      struct SPSlideShow *ss);

/**
 * The main application window for the slideshow
 */
class SPSlideShow : public Gtk::ApplicationWindow {
private:
    std::vector<Glib::ustring>  _slides;  ///< List of filenames for each slide
    int                         _current; ///< Index of the currently displayed slide
    SPDocument                 *_doc;     ///< The currently displayed slide
    int                         _timer;
    GtkWidget                  *_view;
    Gtk::Window                *_ctrlwin; ///< Window containing slideshow control buttons

public:
    /// Current state of application (full-screen or windowed)
    bool is_fullscreen;

    /// Update the window title with current document name
    void update_title()
    {
        Glib::ustring title(_doc->getName());
        if (_slides.size() > 1) {
            title += Glib::ustring::compose("  (%1/%2)", _current+1, _slides.size());
        }

        set_title(title);
    }

    SPSlideShow(std::vector<Glib::ustring> const &slides);
    
    void set_timer(int timer) {_timer = timer;}
    void control_show();
    void show_next();
    void show_prev();
    void goto_first();
    void goto_last();

    static int ctrlwin_delete (GtkWidget *widget,
                               GdkEvent  *event,
                               void      *data);
protected:
    void waiting_cursor();
    void normal_cursor();
    void set_document(SPDocument *doc,
                      int         current);
};

SPSlideShow::SPSlideShow(std::vector<Glib::ustring> const &slides)
    :
        _slides(slides),
        _current(0),
        _doc(SPDocument::createNewDoc(_slides[0].c_str(), true, false)),
        _view(NULL),
        is_fullscreen(false),
        _timer(0),
        _ctrlwin(NULL)
{
    update_title();

    auto default_screen = Gdk::Screen::get_default();

    set_default_size(MIN ((int)_doc->getWidth().value("px"),  default_screen->get_width()  - 64),
            MIN ((int)_doc->getHeight().value("px"), default_screen->get_height() - 64));

    g_signal_connect (G_OBJECT (gobj()), "delete_event",    (GCallback) sp_svgview_main_delete,    this);
    g_signal_connect (G_OBJECT (gobj()), "key_press_event", (GCallback) sp_svgview_main_key_press, this);

    _doc->ensureUpToDate();
    _view = sp_svg_view_widget_new (_doc);
    _doc->doUnref ();
    SP_SVG_VIEW_WIDGET(_view)->setResize( false, _doc->getWidth().value("px"), _doc->getHeight().value("px") );
    gtk_widget_show (_view);
    add(*Glib::wrap(_view));

    show();
}


static int sp_svgview_main_delete (GtkWidget */*widget*/,
                                   GdkEvent */*event*/,
                                   struct SPSlideShow */*ss*/)
{
    Gtk::Main::quit();
    return FALSE;
}

static int sp_svgview_main_key_press (GtkWidget */*widget*/, 
                                      GdkEventKey *event,
                                      struct SPSlideShow *ss)
{
    switch (event->keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_Home:
            ss->goto_first();
            break;
        case GDK_KEY_Down:
        case GDK_KEY_End:
            ss->goto_last();
            break;
        case GDK_KEY_F11:
            if (ss->is_fullscreen) {
                ss->unfullscreen();
                ss->is_fullscreen = false;
            } else {
                ss->fullscreen();
                ss->is_fullscreen = true;
            }
            break;
        case GDK_KEY_Return:
            ss->control_show();
        break;
        case GDK_KEY_KP_Page_Down:
        case GDK_KEY_Page_Down:
        case GDK_KEY_Right:
        case GDK_KEY_space:
            ss->show_next();
        break;
        case GDK_KEY_KP_Page_Up:
        case GDK_KEY_Page_Up:
        case GDK_KEY_Left:
        case GDK_KEY_BackSpace:
            ss->show_prev();
            break;
        case GDK_KEY_Escape:
        case GDK_KEY_q:
        case GDK_KEY_Q:
            Gtk::Main::quit();
            break;
        default:
            break;
    }

    ss->update_title();
    return TRUE;
}

/// List of all input filenames
static Glib::OptionGroup::vecustrings filenames;

/// Input timer option
static int timer = 0;

/**
 * \brief Set of command-line options for Inkview
 */
class InkviewOptionsGroup : public Glib::OptionGroup
{
public:
    InkviewOptionsGroup()
        :
            Glib::OptionGroup(N_("Inkscape Options"),
                              N_("Default program options")),
            _entry_timer(),
            _entry_args()
    {
        // Entry for the "timer" option
        _entry_timer.set_short_name('t');
        _entry_timer.set_long_name("timer");
        _entry_timer.set_arg_description(N_("NUM"));
        _entry_timer.set_description(N_("Reset timer:"));
        add_entry(_entry_timer, timer);

        // Entry for the remaining non-option arguments
        _entry_args.set_short_name('\0');
        _entry_args.set_long_name(G_OPTION_REMAINING);
        _entry_args.set_arg_description(N_("FILES …"));

        add_entry(_entry_args, filenames);
    }

private:
    Glib::OptionEntry _entry_timer;
    Glib::OptionEntry _entry_args;
};


/** get a list of valid SVG files from a list of strings */
std::vector<Glib::ustring> get_valid_files(std::vector<Glib::ustring> filenames, bool recursive = false)
{
    std::vector<Glib::ustring> valid_files;

    for(auto file : filenames)
    {
        if (!Inkscape::IO::file_test( file.c_str(), G_FILE_TEST_EXISTS )) {
            g_printerr("%s: %s\n", _("File or folder does not exist"), file.c_str());
        } else {
            if (Inkscape::IO::file_test( file.c_str(), G_FILE_TEST_IS_DIR )) {
                if (recursive) {
                    std::vector<Glib::ustring> new_filenames;
                    Glib::Dir directory(file);
                    for (auto new_file: directory) {
                        Glib::ustring extension = new_file.substr( new_file.find_last_of(".") + 1 );
                        if (!extension.compare("svg") || !extension.compare("svgz")) {
                            new_filenames.push_back(Glib::build_filename(file, new_file));
                        }
                    }
                    std::vector<Glib::ustring> new_files = get_valid_files(new_filenames);
                    valid_files.insert(valid_files.end(), new_files.begin(), new_files.end());
                }
            } else {
                auto doc = SPDocument::createNewDoc(file.c_str(), TRUE, false);
                if(doc) {
                    /* Append to list */
                    valid_files.push_back(file);
                } else {
                    g_printerr("%s: %s\n", _("Could not open file"), file.c_str());
                }
            }
        }
    }
    
    return valid_files;
}

#ifdef WIN32
// minimal print handler (just prints the string to stdout)
void g_print_no_convert(const gchar *buf) { fputs(buf, stdout); }
void g_printerr_no_convert(const gchar *buf) { fputs(buf, stderr); }
#endif

int main (int argc, char **argv)
{
#ifdef WIN32
    // Ugly hack to make g_print emit UTF-8 encoded characters. Otherwise glib will *always*
    // perform character conversion to the system's ANSI code page making UTF-8 output impossible.
    g_set_print_handler(g_print_no_convert);
    g_set_printerr_handler(g_print_no_convert);
#endif
#ifdef ENABLE_NLS
    Inkscape::initialize_gettext();
#endif

    Glib::OptionContext opt(N_("Open SVG files"));
    opt.set_translation_domain(GETTEXT_PACKAGE);
    
    InkviewOptionsGroup grp;
    grp.set_translation_domain(GETTEXT_PACKAGE);
    
    opt.set_main_group(grp);

    Gtk::Main main_instance (argc, argv, opt);

    LIBXML_TEST_VERSION

    Inkscape::GC::init();
    Inkscape::Preferences::get(); // ensure preferences are initialized

    Inkscape::Application::create(argv[0], true);

    if(filenames.empty())
    {
        g_print("%s", opt.get_help().c_str());
        exit(EXIT_FAILURE);
    }

    std::vector<Glib::ustring> valid_files = get_valid_files(filenames, true);
    if(valid_files.empty()) {
       return 1; /* none of the slides loadable */
    }
    
    SPSlideShow ss(valid_files);
    ss.set_timer(timer);
    main_instance.run();

    return 0;
}

int SPSlideShow::ctrlwin_delete (GtkWidget */*widget*/,
                                 GdkEvent  */*event*/,
                                 void      *data)
{
    auto ss = reinterpret_cast<SPSlideShow *>(data);
    if(ss->_ctrlwin) delete ss->_ctrlwin;

    ss->_ctrlwin = NULL;
    return FALSE;
}

/**
 * @brief Show the control buttons (next, previous etc) for the application
 */
void SPSlideShow::control_show()
{
    if (!_ctrlwin) {
        _ctrlwin = new Gtk::Window();
        _ctrlwin->set_resizable(false);
        _ctrlwin->set_transient_for(*this);
        g_signal_connect(G_OBJECT (_ctrlwin->gobj()), "key_press_event", (GCallback) sp_svgview_main_key_press, this);
        g_signal_connect(G_OBJECT (_ctrlwin->gobj()), "delete_event",    (GCallback) SPSlideShow::ctrlwin_delete, this);
        auto t = Gtk::manage(new Gtk::ButtonBox());
        _ctrlwin->add(*t);

        auto btn_go_first = Gtk::manage(new Gtk::Button());
        auto img_go_first = Gtk::manage(new Gtk::Image());
        img_go_first->set_from_icon_name(INKSCAPE_ICON("go-first"), Gtk::ICON_SIZE_BUTTON);
        btn_go_first->set_image(*img_go_first);
        t->add(*btn_go_first);
        btn_go_first->signal_clicked().connect(sigc::mem_fun(*this, &SPSlideShow::goto_first));
        
        auto btn_go_prev = Gtk::manage(new Gtk::Button());
        auto img_go_prev = Gtk::manage(new Gtk::Image());
        img_go_prev->set_from_icon_name(INKSCAPE_ICON("go-previous"), Gtk::ICON_SIZE_BUTTON);
        btn_go_prev->set_image(*img_go_prev);
        t->add(*btn_go_prev);
        btn_go_prev->signal_clicked().connect(sigc::mem_fun(*this, &SPSlideShow::show_prev));
        
        auto btn_go_next = Gtk::manage(new Gtk::Button());
        auto img_go_next = Gtk::manage(new Gtk::Image());
        img_go_next->set_from_icon_name(INKSCAPE_ICON("go-next"), Gtk::ICON_SIZE_BUTTON);
        btn_go_next->set_image(*img_go_next);
        t->add(*btn_go_next);
        btn_go_next->signal_clicked().connect(sigc::mem_fun(*this, &SPSlideShow::show_next));
        
        auto btn_go_last = Gtk::manage(new Gtk::Button());
        auto img_go_last = Gtk::manage(new Gtk::Image());
        img_go_last->set_from_icon_name(INKSCAPE_ICON("go-last"), Gtk::ICON_SIZE_BUTTON);
        btn_go_last->set_image(*img_go_last);
        t->add(*btn_go_last);
        btn_go_last->signal_clicked().connect(sigc::mem_fun(*this, &SPSlideShow::goto_last));

        _ctrlwin->show_all();
    } else {
        _ctrlwin->present();
    }
}

void SPSlideShow::waiting_cursor()
{
    auto display = Gdk::Display::get_default();
    auto waiting = Gdk::Cursor::create(display, Gdk::WATCH);
    get_window()->set_cursor(waiting);
    
    if (_ctrlwin) {
        _ctrlwin->get_window()->set_cursor(waiting);
    }
    while(Gtk::Main::events_pending()) {
        Gtk::Main::iteration();
    }
}

void SPSlideShow::normal_cursor()
{
    get_window()->set_cursor();
    if (_ctrlwin) {
        _ctrlwin->get_window()->set_cursor();
    }
}

void SPSlideShow::set_document(SPDocument *doc,
                               int         current)
{
    if (doc && doc != _doc) {
        doc->ensureUpToDate();
        reinterpret_cast<SPSVGView*>(SP_VIEW_WIDGET_VIEW (_view))->setDocument (doc);
        _doc = doc;
        _current = current;
    }
}

/**
 * @brief Show the next file in the slideshow
 */
void SPSlideShow::show_next()
{
    waiting_cursor();

    SPDocument *doc = NULL;
    while (!doc && (_current < _slides.size() - 1)) {
        doc = SPDocument::createNewDoc ((_slides[++_current]).c_str(), TRUE, false);
    }

    set_document(doc, _current);
    normal_cursor();
}

/**
 * @brief Show the previous file in the slideshow
 */
void SPSlideShow::show_prev()
{
    waiting_cursor();

    SPDocument *doc = NULL;
    while (!doc && (_current > 0)) {
        doc = SPDocument::createNewDoc ((_slides[--_current]).c_str(), TRUE, false);
    }

    set_document(doc, _current);
    normal_cursor();
}

/**
 * @brief Switch to first slide in slideshow
 */
void SPSlideShow::goto_first()
{
    waiting_cursor();

    SPDocument *doc = NULL;
    int current = 0;
    while ( !doc && (current < _slides.size() - 1)) {
        doc = SPDocument::createNewDoc((_slides[current++]).c_str(), TRUE, false);
    }

    set_document(doc, current - 1);

    normal_cursor();
}

/**
 * @brief Switch to last slide in slideshow
 */
void SPSlideShow::goto_last()
{
    waiting_cursor();

    SPDocument *doc = NULL;
    int current = _slides.size() - 1;
    while (!doc && (current >= 0)) {
        doc = SPDocument::createNewDoc((_slides[current--]).c_str(), TRUE, false);
    }

    set_document(doc, current + 1);

    normal_cursor();
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
