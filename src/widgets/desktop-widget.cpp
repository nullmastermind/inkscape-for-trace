// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Desktop widget implementation
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *   John Bintz <jcoswell@coswellproductions.org>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2006 John Bintz
 * Copyright (C) 2004 MenTaLguY
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <2geom/rect.h>

#include "attributes.h"
#include "cms-system.h"
#include "conn-avoid-ref.h"
#include "desktop-events.h"
#include "desktop-widget.h"
#include "desktop.h"
#include "document-undo.h"
#include "ege-color-prof-tracker.h"
#include "file.h"
#include "inkscape-version.h"
#include "verbs.h"

#include "display/canvas-arena.h"
#include "display/canvas-axonomgrid.h"
#include "display/guideline.h"
#include "display/sp-canvas.h"

#include "extension/db.h"

#include "helper/action.h"

#include "object/sp-image.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"

#include "ui/dialog/dialog-manager.h"
#include "ui/dialog/swatches.h"
#include "ui/icon-loader.h"
#include "ui/icon-names.h"
#include "ui/tools/box3d-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/button.h"
#include "ui/widget/dock.h"
#include "ui/widget/ink-select-one-action.h"
#include "ui/widget/layer-selector.h"
#include "ui/widget/selected-style.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"

// TEMP
#include "ui/desktop/menubar.h"

#include "util/ege-appear-time-tracker.h"
#include "util/units.h"

// We're in the "widgets" directory, so no need to explicitly prefix these:
#include "gimp/ruler.h"
#include "spinbutton-events.h"
#include "spw-utilities.h"
#include "toolbox.h"
#include "widget-sizes.h"


using Inkscape::DocumentUndo;
using Inkscape::UI::Widget::UnitTracker;
using Inkscape::UI::UXManager;
using Inkscape::UI::ToolboxFactory;
using ege::AppearTimeTracker;
using Inkscape::Util::unit_table;


//---------------------------------------------------------------------
/* SPDesktopWidget */

static void sp_desktop_widget_class_init (SPDesktopWidgetClass *klass);

static void sp_desktop_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void sp_desktop_widget_realize (GtkWidget *widget);

static void sp_desktop_widget_adjustment_value_changed (GtkAdjustment *adj, SPDesktopWidget *dtw);

static gdouble sp_dtw_zoom_value_to_display (gdouble value);
static gdouble sp_dtw_zoom_display_to_value (gdouble value);
static void sp_dtw_zoom_menu_handler (SPDesktop *dt, gdouble factor);

SPViewWidgetClass *dtw_parent_class;

class CMSPrefWatcher {
public:
    CMSPrefWatcher() :
        _dpw(*this),
        _spw(*this),
        _tracker(ege_color_prof_tracker_new(nullptr))
    {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        g_signal_connect( G_OBJECT(_tracker), "modified", G_CALLBACK(hook), this );
        prefs->addObserver(_dpw);
        prefs->addObserver(_spw);
    }
    virtual ~CMSPrefWatcher() = default;

    //virtual void notify(PrefValue &);
    void add( SPDesktopWidget* dtw ) {
        _widget_list.push_back(dtw);
    }
    void remove( SPDesktopWidget* dtw ) {
        _widget_list.remove(dtw);
    }

private:
    static void hook(EgeColorProfTracker *tracker, gint b, CMSPrefWatcher *watcher);

    class DisplayProfileWatcher : public Inkscape::Preferences::Observer {
    public:
        DisplayProfileWatcher(CMSPrefWatcher &pw) : Observer("/options/displayprofile"), _pw(pw) {}
        void notify(Inkscape::Preferences::Entry const &/*val*/) override {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            _pw._setCmsSensitive(!prefs->getString("/options/displayprofile/uri").empty());
            _pw._refreshAll();
        }
    private:
        CMSPrefWatcher &_pw;
    };

    DisplayProfileWatcher _dpw;

    class SoftProofWatcher : public Inkscape::Preferences::Observer {
    public:
        SoftProofWatcher(CMSPrefWatcher &pw) : Observer("/options/softproof"), _pw(pw) {}
        void notify(Inkscape::Preferences::Entry const &) override {
            _pw._refreshAll();
        }
    private:
        CMSPrefWatcher &_pw;
    };

    SoftProofWatcher _spw;

    void _refreshAll();
    void _setCmsSensitive(bool value);

    std::list<SPDesktopWidget*> _widget_list;
    EgeColorProfTracker *_tracker;

    friend class DisplayProfileWatcher;
    friend class SoftproofWatcher;
};

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
void CMSPrefWatcher::hook(EgeColorProfTracker * /*tracker*/, gint monitor, CMSPrefWatcher * /*watcher*/)
{
    unsigned char* buf = nullptr;
    guint len = 0;

    ege_color_prof_tracker_get_profile_for( monitor, reinterpret_cast<gpointer*>(&buf), &len );
    Glib::ustring id = Inkscape::CMSSystem::setDisplayPer( buf, len, monitor );
}
#else
void CMSPrefWatcher::hook(EgeColorProfTracker * /*tracker*/, gint /*monitor*/, CMSPrefWatcher * /*watcher*/)
{
}
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

/// @todo Use conditional compilation in saner places. The whole PrefWatcher
/// object is unnecessary if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2) is not defined.
void CMSPrefWatcher::_refreshAll()
{
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    for (auto & it : _widget_list) {
        it->requestCanvasUpdate();
    }
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
}

void CMSPrefWatcher::_setCmsSensitive(bool enabled)
{
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    for ( auto dtw : _widget_list ) {
        auto cms_adj = dtw->get_cms_adjust();
        if ( cms_adj->get_sensitive() != enabled ) {
            dtw->cms_adjust_set_sensitive(enabled);
        }
    }
#else
    (void) enabled;
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
}

static CMSPrefWatcher* watcher = nullptr;

void
SPDesktopWidget::setMessage (Inkscape::MessageType type, const gchar *message)
{
    _select_status->set_markup(message ? message : "");

    // make sure the important messages are displayed immediately!
    if (type == Inkscape::IMMEDIATE_MESSAGE && _select_status->get_is_drawable()) {
        _select_status->queue_draw();
    }

    _select_status->set_tooltip_text(_select_status->get_text());
}

Geom::Point
SPDesktopWidget::window_get_pointer()
{
    int x, y;
    auto window = Glib::wrap(GTK_WIDGET(_canvas))->get_window();
    auto display = window->get_display();

#if GTK_CHECK_VERSION(3,20,0)
    auto seat = display->get_default_seat();
    auto device = seat->get_pointer();
#else
    auto dm = display->get_device_manager();
    auto device = dm->get_client_pointer();
#endif
    Gdk::ModifierType m;
    window->get_device_position(device, x, y, m);

    return Geom::Point(x, y);
}

static GTimer *overallTimer = nullptr;

/**
 * Registers SPDesktopWidget class and returns its type number.
 */
GType SPDesktopWidget::getType()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPDesktopWidgetClass),
            nullptr, // base_init
            nullptr, // base_finalize
            (GClassInitFunc)sp_desktop_widget_class_init,
            nullptr, // class_finalize
            nullptr, // class_data
            sizeof(SPDesktopWidget),
            0, // n_preallocs
            (GInstanceInitFunc)SPDesktopWidget::init,
            nullptr // value_table
        };
        type = g_type_register_static(SP_TYPE_VIEW_WIDGET, "SPDesktopWidget", &info, static_cast<GTypeFlags>(0));
        // Begin a timer to watch for the first desktop to appear on-screen
        overallTimer = g_timer_new();
    }
    return type;
}

/**
 * SPDesktopWidget vtable initialization
 */
static void
sp_desktop_widget_class_init (SPDesktopWidgetClass *klass)
{
    dtw_parent_class = SP_VIEW_WIDGET_CLASS(g_type_class_peek_parent(klass));

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->dispose = SPDesktopWidget::dispose;

    widget_class->size_allocate = sp_desktop_widget_size_allocate;
    widget_class->realize = sp_desktop_widget_realize;
}

/**
 * Callback for changes in size of the canvas table (i.e. the container for
 * the canvas, the rulers etc).
 *
 * This adjusts the range of the rulers when the dock container is adjusted
 * (fixes lp:950552)
 */
void
SPDesktopWidget::canvas_tbl_size_allocate(Gtk::Allocation& /*allocation*/)
{
    update_rulers();
}

/**
 * Callback for SPDesktopWidget object initialization.
 */
void SPDesktopWidget::init( SPDesktopWidget *dtw )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    new (&dtw->modified_connection) sigc::connection();

    dtw->_ruler_clicked = false;
    dtw->_ruler_dragged = false;
    dtw->_active_guide = nullptr;
    dtw->_xp = 0;
    dtw->_yp = 0;
    dtw->window = nullptr;
    dtw->desktop = nullptr;
    dtw->_interaction_disabled_counter = 0;

    /* Main table */
    dtw->_vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    dtw->_vbox->set_name("DesktopMainTable");
    gtk_container_add( GTK_CONTAINER(dtw), GTK_WIDGET(dtw->_vbox->gobj()) );

    /* Status bar */
    dtw->_statusbar = Gtk::manage(new Gtk::Box());
    dtw->_statusbar->set_name("DesktopStatusBar");
    dtw->_vbox->pack_end(*dtw->_statusbar, false, true);

    /* Swatches panel */
    {
        dtw->_panels = new Inkscape::UI::Dialog::SwatchesPanel("/embedded/swatches");
        dtw->_panels->set_vexpand(false);
        dtw->_vbox->pack_end(*dtw->_panels, false, true);
    }

    /* DesktopHBox (Vertical toolboxes, canvas) */
    dtw->_hbox = Gtk::manage(new Gtk::Box());
    dtw->_hbox->set_name("DesktopHbox");
    dtw->_vbox->pack_end(*dtw->_hbox, true, true);

    /* Toolboxes */
    dtw->aux_toolbox = ToolboxFactory::createAuxToolbox();
    dtw->_vbox->pack_end(*Glib::wrap(dtw->aux_toolbox), false, true);

    dtw->snap_toolbox = ToolboxFactory::createSnapToolbox();
    ToolboxFactory::setOrientation( dtw->snap_toolbox, GTK_ORIENTATION_VERTICAL );
    dtw->_hbox->pack_end(*Glib::wrap(dtw->snap_toolbox), false, true);

    dtw->commands_toolbox = ToolboxFactory::createCommandsToolbox();
    dtw->_vbox->pack_end(*Glib::wrap(dtw->commands_toolbox), false, true);

    dtw->tool_toolbox = ToolboxFactory::createToolToolbox();
    ToolboxFactory::setOrientation( dtw->tool_toolbox, GTK_ORIENTATION_VERTICAL );
    dtw->_hbox->pack_start(*Glib::wrap(dtw->tool_toolbox), false, true);

    /* Canvas table wrapper */
    auto tbl_wrapper = Gtk::manage(new Gtk::Grid()); // Is this widget really needed? No!
    tbl_wrapper->set_name("CanvasTableWrapper");
    dtw->_hbox->pack_start(*tbl_wrapper, true, true, 1);

    /* Canvas table */
    dtw->_canvas_tbl = Gtk::manage(new Gtk::Grid());
    dtw->_canvas_tbl->set_name("CanvasTable");
    // Added to table wrapper later either directly or via paned window shared with dock.



    // Lock guides button
    dtw->_guides_lock = Gtk::manage(new Inkscape::UI::Widget::Button(GTK_ICON_SIZE_MENU,
                                                                     Inkscape::UI::Widget::BUTTON_TYPE_TOGGLE,
                                                                     nullptr,
                                                                     INKSCAPE_ICON("object-locked"),
                                                                     _("Toggle lock of all guides in the document")));

    auto guides_lock_style_provider = Gtk::CssProvider::create();
    guides_lock_style_provider->load_from_data("GtkWidget { padding-left: 0; padding-right: 0; padding-top: 0; padding-bottom: 0; }");
    dtw->_guides_lock->set_name("LockGuides");
    auto context = dtw->_guides_lock->get_style_context();
    context->add_provider(guides_lock_style_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    dtw->_guides_lock->signal_toggled().connect(sigc::mem_fun(dtw, &SPDesktopWidget::update_guides_lock));
    dtw->_canvas_tbl->attach(*dtw->_guides_lock, 0, 0, 1, 1);

    /* Horizontal ruler */
    dtw->_hruler = Glib::wrap(sp_ruler_new(GTK_ORIENTATION_HORIZONTAL));
    dtw->_hruler->set_name("HorizontalRuler");
    dtw->_hruler_box = Gtk::manage(new Gtk::EventBox());
    Inkscape::Util::Unit const *pt = unit_table.getUnit("pt");
    sp_ruler_set_unit(SP_RULER(dtw->_hruler->gobj()), pt);
    dtw->_hruler_box->set_tooltip_text(gettext(pt->name_plural.c_str()));
    dtw->_hruler_box->add(*dtw->_hruler);

    dtw->_hruler_box->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*dtw, &SPDesktopWidget::on_ruler_box_button_press_event), dtw->_hruler_box, true));
    dtw->_hruler_box->signal_button_release_event().connect(sigc::bind(sigc::mem_fun(*dtw, &SPDesktopWidget::on_ruler_box_button_release_event), dtw->_hruler_box, true));
    dtw->_hruler_box->signal_motion_notify_event().connect(sigc::bind(sigc::mem_fun(*dtw, &SPDesktopWidget::on_ruler_box_motion_notify_event), dtw->_hruler_box, true));

    dtw->_canvas_tbl->attach(*dtw->_hruler_box, 1, 0, 1, 1);

    /* Vertical ruler */
    dtw->_vruler = Glib::wrap(sp_ruler_new(GTK_ORIENTATION_VERTICAL));
    dtw->_vruler->set_name("VerticalRuler");
    dtw->_vruler_box = Gtk::manage(new Gtk::EventBox());
    sp_ruler_set_unit (SP_RULER (dtw->_vruler->gobj()), pt);
    dtw->_vruler_box->set_tooltip_text(gettext(pt->name_plural.c_str()));
    dtw->_vruler_box->add(*dtw->_vruler);

    dtw->_vruler_box->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(*dtw, &SPDesktopWidget::on_ruler_box_button_press_event), dtw->_vruler_box, false));
    dtw->_vruler_box->signal_button_release_event().connect(sigc::bind(sigc::mem_fun(*dtw, &SPDesktopWidget::on_ruler_box_button_release_event), dtw->_vruler_box, false));
    dtw->_vruler_box->signal_motion_notify_event().connect(sigc::bind(sigc::mem_fun(*dtw, &SPDesktopWidget::on_ruler_box_motion_notify_event), dtw->_vruler_box, false));

    dtw->_canvas_tbl->attach(*dtw->_vruler_box, 0, 1, 1, 1);

    // Horizontal scrollbar
    dtw->_hadj = Gtk::Adjustment::create(0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0);
    dtw->_hscrollbar = Gtk::manage(new Gtk::Scrollbar(dtw->_hadj));
    dtw->_hscrollbar->set_name("HorizontalScrollbar");
    dtw->_canvas_tbl->attach(*dtw->_hscrollbar, 1, 2, 1, 1);

    // By packing the sticky zoom button and vertical scrollbar in a box it allows the canvas to
    // expand fully to the top if the rulers are hidden.
    // (Otherwise, the canvas is pushed down by the height of the sticky zoom button.)

    // Vertical Scrollbar box
    dtw->_vscrollbar_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    dtw->_canvas_tbl->attach(*dtw->_vscrollbar_box, 2, 0, 1, 2);

    // Sticky zoom button
    dtw->_sticky_zoom = Gtk::manage(new Inkscape::UI::Widget::Button(GTK_ICON_SIZE_MENU,
                                                                     Inkscape::UI::Widget::BUTTON_TYPE_TOGGLE,
                                                                     nullptr,
                                                                     INKSCAPE_ICON("zoom-original"),
                                                                     _("Zoom drawing if window size changes")));
    dtw->_sticky_zoom->set_name("StickyZoom");
    dtw->_sticky_zoom->set_active(prefs->getBool("/options/stickyzoom/value"));
    dtw->_sticky_zoom->signal_toggled().connect(sigc::mem_fun(dtw, &SPDesktopWidget::sticky_zoom_toggled));
    dtw->_vscrollbar_box->pack_start(*dtw->_sticky_zoom, false, false);

    // Vertical scrollbar
    dtw->_vadj = Gtk::Adjustment::create(0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0);
    dtw->_vscrollbar = Gtk::manage(new Gtk::Scrollbar(dtw->_vadj, Gtk::ORIENTATION_VERTICAL));
    dtw->_vscrollbar->set_name("VerticalScrollbar");
    dtw->_vscrollbar_box->pack_start(*dtw->_vscrollbar, true, true, 0);

    gchar const* tip = "";
    Inkscape::Verb* verb = Inkscape::Verb::get( SP_VERB_VIEW_CMS_TOGGLE );
    if ( verb ) {
        SPAction *act = verb->get_action( Inkscape::ActionContext( dtw->viewwidget.view ) );
        if ( act && act->tip ) {
            tip = act->tip;
        }
    }
    dtw->_cms_adjust = Gtk::manage(new Inkscape::UI::Widget::Button(GTK_ICON_SIZE_MENU,
                                                                    Inkscape::UI::Widget::BUTTON_TYPE_TOGGLE,
                                                                    nullptr,
                                                                    INKSCAPE_ICON("color-management"),
                                                                    tip ));
    dtw->_cms_adjust->set_name("CMS_Adjust");

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    {
        Glib::ustring current = prefs->getString("/options/displayprofile/uri");
        bool enabled = current.length() > 0;
        dtw->cms_adjust_set_sensitive(enabled );
        if ( enabled ) {
            bool active = prefs->getBool("/options/displayprofile/enable");
            if ( active ) {
                dtw->_cms_adjust->toggle_set_down(true);
            }
        }
    }
    g_signal_connect_after( G_OBJECT(dtw->_cms_adjust->gobj()), "clicked", G_CALLBACK(SPDesktopWidget::cms_adjust_toggled), dtw );
#else
    dtw->cms_adjust_set_sensitive(false);
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

    dtw->_canvas_tbl->attach(*dtw->_cms_adjust, 2, 2, 1, 1);
    {
        if (!watcher) {
            watcher = new CMSPrefWatcher();
        }
        watcher->add(dtw);
    }
    /* Canvas */
    dtw->_canvas = SP_CANVAS(SPCanvas::createAA());
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    dtw->_canvas->_enable_cms_display_adj = prefs->getBool("/options/displayprofile/enable");
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    gtk_widget_set_can_focus (GTK_WIDGET (dtw->_canvas), TRUE);

    sp_ruler_add_track_widget (SP_RULER(dtw->_hruler->gobj()), GTK_WIDGET(dtw->_canvas));
    sp_ruler_add_track_widget (SP_RULER(dtw->_vruler->gobj()), GTK_WIDGET(dtw->_canvas));
    auto css_provider  = gtk_css_provider_new();
    auto style_context = gtk_widget_get_style_context(GTK_WIDGET(dtw->_canvas));

    gtk_css_provider_load_from_data(css_provider,
                                    "SPCanvas {\n"
                                    " background-color: white;\n"
                                    "}\n",
                                    -1, nullptr);

    gtk_style_context_add_provider(style_context,
                                   GTK_STYLE_PROVIDER(css_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_signal_connect (G_OBJECT (dtw->_canvas), "event", G_CALLBACK (SPDesktopWidget::event), dtw);

    gtk_widget_set_hexpand(GTK_WIDGET(dtw->_canvas), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(dtw->_canvas), TRUE);
    dtw->_canvas_tbl->attach(*Glib::wrap(GTK_WIDGET(dtw->_canvas)), 1, 1, 1, 1);

    /* Dock */
    bool create_dock =
        prefs->getIntLimited("/options/dialogtype/value", Inkscape::UI::Dialog::FLOATING, 0, 1) ==
        Inkscape::UI::Dialog::DOCK;

    if (create_dock) {
        dtw->_dock = new Inkscape::UI::Widget::Dock();
        auto paned = new Gtk::Paned();
        paned->set_name("Canvas_and_Dock");

        paned->pack1(*dtw->_canvas_tbl);
        paned->pack2(dtw->_dock->getWidget(), Gtk::FILL);

        /* Prevent the paned from catching F6 and F8 by unsetting the default callbacks */
        if (GtkPanedClass *paned_class = GTK_PANED_CLASS (G_OBJECT_GET_CLASS (paned->gobj()))) {
            paned_class->cycle_child_focus = nullptr;
            paned_class->cycle_handle_focus = nullptr;
        }

        paned->set_hexpand(true);
        paned->set_vexpand(true);
        tbl_wrapper->attach(*paned, 1, 1, 1, 1);
    } else {
        dtw->_canvas_tbl->set_hexpand(true);
        dtw->_canvas_tbl->set_vexpand(true);
        gtk_grid_attach(GTK_GRID(tbl_wrapper), GTK_WIDGET (dtw->_canvas_tbl->gobj()), 1, 1, 1, 1);
    }

    // connect scrollbar signals
    dtw->_hadj->signal_value_changed().connect(sigc::mem_fun(dtw, &SPDesktopWidget::on_adjustment_value_changed));
    dtw->_vadj->signal_value_changed().connect(sigc::mem_fun(dtw, &SPDesktopWidget::on_adjustment_value_changed));

    // --------------- Status Tool Bar ------------------//

    // Selected Style (Fill/Stroke/Opacity)
    dtw->_selected_style = new Inkscape::UI::Widget::SelectedStyle(true);
    dtw->_statusbar->pack_start(*dtw->_selected_style, false, false);

    // Separator
    dtw->_statusbar->pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL)),
		                false, false);

    // Layer Selector
    dtw->layer_selector = new Inkscape::UI::Widget::LayerSelector(nullptr);
    // FIXME: need to unreference on container destruction to avoid leak
    dtw->layer_selector->reference();
    dtw->_statusbar->pack_start(*dtw->layer_selector, false, false, 1);

    // Select Status
    dtw->_select_status = Gtk::manage(new Gtk::Label());
    dtw->_select_status->set_name("SelectStatus");
    dtw->_select_status->set_ellipsize(Pango::ELLIPSIZE_END);
#if GTK_CHECK_VERSION(3,10,0)
    dtw->_select_status->set_line_wrap(true);
    dtw->_select_status->set_lines(2);
#endif

    dtw->_select_status->set_halign(Gtk::ALIGN_START);
    dtw->_select_status->set_size_request(1, -1);

    // Display the initial welcome message in the statusbar
    dtw->_select_status->set_markup(_("<b>Welcome to Inkscape!</b> Use shape or freehand tools to create objects; use selector (arrow) to move or transform them."));

    dtw->_statusbar->pack_start(*dtw->_select_status, true, true);


    // Zoom status spinbutton ---------------
    auto zoom_adj = Gtk::Adjustment::create(100.0, log(SP_DESKTOP_ZOOM_MIN)/log(2), log(SP_DESKTOP_ZOOM_MAX)/log(2), 0.1);
    dtw->_zoom_status = Gtk::manage(new Gtk::SpinButton(zoom_adj));

    dtw->_zoom_status->set_data("dtw", dtw->_canvas);
    dtw->_zoom_status->set_tooltip_text(_("Zoom"));
    dtw->_zoom_status->set_size_request(STATUS_ZOOM_WIDTH, -1);
    dtw->_zoom_status->set_width_chars(6);
    dtw->_zoom_status->set_numeric(false);
    dtw->_zoom_status->set_update_policy(Gtk::UPDATE_ALWAYS);

    // Callbacks
    dtw->_zoom_status_input_connection  = dtw->_zoom_status->signal_input().connect(sigc::mem_fun(dtw, &SPDesktopWidget::zoom_input));
    dtw->_zoom_status_output_connection = dtw->_zoom_status->signal_output().connect(sigc::mem_fun(dtw, &SPDesktopWidget::zoom_output));
    g_signal_connect (G_OBJECT (dtw->_zoom_status->gobj()), "focus-in-event", G_CALLBACK (spinbutton_focus_in), dtw->_zoom_status->gobj());
    g_signal_connect (G_OBJECT (dtw->_zoom_status->gobj()), "key-press-event", G_CALLBACK (spinbutton_keypress), dtw->_zoom_status->gobj());
    dtw->_zoom_status_value_changed_connection = dtw->_zoom_status->signal_value_changed().connect(sigc::mem_fun(dtw, &SPDesktopWidget::zoom_value_changed));
    dtw->_zoom_status_populate_popup_connection = dtw->_zoom_status->signal_populate_popup().connect(sigc::mem_fun(dtw, &SPDesktopWidget::zoom_populate_popup));

    // Style
    auto css_provider_spinbutton = Gtk::CssProvider::create();
    css_provider_spinbutton->load_from_data("* { padding-left: 2px; padding-right: 2px; padding-top: 0px; padding-bottom: 0px;}");  // Shouldn't this be in a style sheet? Used also by rotate.

    dtw->_zoom_status->set_name("ZoomStatus");
    auto context_zoom = dtw->_zoom_status->get_style_context();
    context_zoom->add_provider(css_provider_spinbutton, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Rotate status spinbutton ---------------
    auto rotation_adj = Gtk::Adjustment::create(0, -360.0, 360.0, 1.0);
    dtw->_rotation_status = Gtk::manage(new Gtk::SpinButton(rotation_adj));
    dtw->_rotation_status->set_data("dtw", dtw->_canvas);
    dtw->_rotation_status->set_tooltip_text(_("Rotation. (Also Ctrl+Shift+Scroll)"));
    dtw->_rotation_status->set_size_request(STATUS_ROTATION_WIDTH, -1);
    dtw->_rotation_status->set_width_chars(7);
    dtw->_rotation_status->set_numeric(false);
    dtw->_rotation_status->set_digits(2);
    dtw->_rotation_status->set_increments(1.0, 15.0);
    dtw->_rotation_status->set_update_policy(Gtk::UPDATE_ALWAYS);

    // Callbacks
    dtw->_rotation_status_input_connection  = dtw->_rotation_status->signal_input().connect(sigc::mem_fun(dtw, &SPDesktopWidget::rotation_input));
    dtw->_rotation_status_output_connection = dtw->_rotation_status->signal_output().connect(sigc::mem_fun(dtw, &SPDesktopWidget::rotation_output));
    g_signal_connect (G_OBJECT (dtw->_rotation_status->gobj()), "focus-in-event", G_CALLBACK (spinbutton_focus_in), dtw->_rotation_status->gobj());
    g_signal_connect (G_OBJECT (dtw->_rotation_status->gobj()), "key-press-event", G_CALLBACK (spinbutton_keypress), dtw->_rotation_status->gobj());
    dtw->_rotation_status_value_changed_connection = dtw->_rotation_status->signal_value_changed().connect(sigc::mem_fun(dtw, &SPDesktopWidget::rotation_value_changed));
    dtw->_rotation_status_populate_popup_connection = dtw->_rotation_status->signal_populate_popup().connect(sigc::mem_fun(dtw, &SPDesktopWidget::rotation_populate_popup));

    // Style
    dtw->_rotation_status->set_name("RotationStatus");
    auto context_rotation = dtw->_rotation_status->get_style_context();
    context_rotation->add_provider(css_provider_spinbutton, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);


    // Cursor coordinates
    dtw->_coord_status = Gtk::manage(new Gtk::Grid());
    dtw->_coord_status->set_name("CoordinateAndZStatus");
    dtw->_coord_status->set_row_spacing(0);
    dtw->_coord_status->set_column_spacing(2);
    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
    sep->set_name("CoordinateSeparator");
    dtw->_coord_status->attach(*sep, 0, 0, 1, 2);

    dtw->_coord_status->set_tooltip_text(_("Cursor coordinates"));
    auto label_x = Gtk::manage(new Gtk::Label(_("X:")));
    auto label_y = Gtk::manage(new Gtk::Label(_("Y:")));
    label_x->set_halign(Gtk::ALIGN_START);
    label_y->set_halign(Gtk::ALIGN_START);
    dtw->_coord_status->attach(*label_x, 1, 0, 1, 1);
    dtw->_coord_status->attach(*label_y, 1, 1, 1, 1);
    dtw->_coord_status_x = Gtk::manage(new Gtk::Label());
    dtw->_coord_status_y = Gtk::manage(new Gtk::Label());
    dtw->_coord_status_x->set_name("CoordinateStatusX");
    dtw->_coord_status_y->set_name("CoordinateStatusY");
    dtw->_coord_status_x->set_markup("   0.00 ");
    dtw->_coord_status_y->set_markup("   0.00 ");

    auto label_z = Gtk::manage(new Gtk::Label(_("Z:")));
    label_z->set_name("ZLabel");
    auto label_r = Gtk::manage(new Gtk::Label(_("R:")));
    label_r->set_name("RLabel");
    dtw->_coord_status_x->set_halign(Gtk::ALIGN_END);
    dtw->_coord_status_y->set_halign(Gtk::ALIGN_END);
    dtw->_coord_status->attach(*dtw->_coord_status_x, 2, 0, 1, 1);
    dtw->_coord_status->attach(*dtw->_coord_status_y, 2, 1, 1, 1);
    dtw->_coord_status->attach(*label_z, 3, 0, 1, 2);
    dtw->_coord_status->attach(*label_r, 5, 0, 1, 2);
    dtw->_coord_status->attach(*dtw->_zoom_status, 4, 0, 1, 2);
    dtw->_coord_status->attach(*dtw->_rotation_status, 6, 0, 1, 2);

    sp_set_font_size_smaller(GTK_WIDGET(dtw->_coord_status->gobj()));

    dtw->_statusbar->pack_end(*dtw->_coord_status, false, false);

    // --------------- Color Management ---------------- //
    dtw->_tracker = ege_color_prof_tracker_new(GTK_WIDGET(dtw->layer_selector->gobj()));
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    bool fromDisplay = prefs->getBool( "/options/displayprofile/from_display");
    if ( fromDisplay ) {
        Glib::ustring id = Inkscape::CMSSystem::getDisplayId( 0 );

        bool enabled = false;
        dtw->_canvas->_cms_key = id;
        enabled = !dtw->_canvas->_cms_key.empty();
        dtw->cms_adjust_set_sensitive(enabled);
    }
    g_signal_connect( G_OBJECT(dtw->_tracker), "changed", G_CALLBACK(SPDesktopWidget::color_profile_event), dtw );
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

    // ------------------ Finish Up -------------------- //
    dtw->_vbox->show_all();

    gtk_widget_grab_focus (GTK_WIDGET(dtw->_canvas));

    // If this is the first desktop created, report the time it takes to show up
    if ( overallTimer ) {
        if ( prefs->getBool("/dialogs/debug/trackAppear", false) ) {
            // Time tracker takes ownership of the timer.
            AppearTimeTracker *tracker = new AppearTimeTracker(overallTimer, GTK_WIDGET(dtw), "first SPDesktopWidget");
            tracker->setAutodelete(true);
        } else {
            g_timer_destroy(overallTimer);
        }
        overallTimer = nullptr;
    }
    // Ensure that ruler ranges are updated correctly whenever the canvas table
    // is resized
    dtw->_canvas_tbl_size_allocate_connection = dtw->_canvas_tbl->signal_size_allocate().connect(sigc::mem_fun(dtw, &SPDesktopWidget::canvas_tbl_size_allocate));
}

/**
 * Called before SPDesktopWidget destruction.
 */
void
SPDesktopWidget::dispose(GObject *object)
{
    SPDesktopWidget *dtw = SP_DESKTOP_WIDGET (object);

    if (dtw == nullptr) {
        return;
    }

    UXManager::getInstance()->delTrack(dtw);

    if (dtw->desktop) {
        if ( watcher ) {
            watcher->remove(dtw);
        }

        // Zoom
        dtw->_zoom_status_input_connection.disconnect();
        dtw->_zoom_status_output_connection.disconnect();
        g_signal_handlers_disconnect_matched (G_OBJECT (dtw->_zoom_status->gobj()), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, dtw->_zoom_status);
        dtw->_zoom_status_value_changed_connection.disconnect();
        dtw->_zoom_status_populate_popup_connection.disconnect();

        // Rotation
        dtw->_rotation_status_input_connection.disconnect();
        dtw->_rotation_status_output_connection.disconnect();
        g_signal_handlers_disconnect_matched (G_OBJECT (dtw->_rotation_status->gobj()), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, dtw->_rotation_status);
        dtw->_rotation_status_value_changed_connection.disconnect();
        dtw->_rotation_status_populate_popup_connection.disconnect();

        // Canvas
        g_signal_handlers_disconnect_by_func (G_OBJECT (dtw->_canvas), (gpointer) G_CALLBACK (SPDesktopWidget::event), dtw);
        dtw->_canvas_tbl_size_allocate_connection.disconnect();

        dtw->layer_selector->setDesktop(nullptr);
        dtw->layer_selector->unreference();
        INKSCAPE.remove_desktop(dtw->desktop); // clears selection and event_context
        dtw->modified_connection.disconnect();
        dtw->desktop->destroy();
        Inkscape::GC::release (dtw->desktop);
        dtw->desktop = nullptr;
    }

    dtw->modified_connection.~connection();

    if (G_OBJECT_CLASS (dtw_parent_class)->dispose) {
        (* G_OBJECT_CLASS (dtw_parent_class)->dispose) (object);
    }
}

/**
 * Set the title in the desktop-window (if desktop has an own window).
 *
 * The title has form file name: desktop number - Inkscape.
 * The desktop number is only shown if it's 2 or higher,
 */
void
SPDesktopWidget::updateTitle(gchar const* uri)
{
    if (window) {

        SPDocument *doc = this->desktop->doc();

        std::string Name;
        if (doc->isModifiedSinceSave()) {
            Name += "*";
        }

        Name += uri;

        if (desktop->number > 1) {
            Name += ": ";
            Name += std::to_string(desktop->number);
        }
        Name += " (";

        if (desktop->getMode() == Inkscape::RENDERMODE_OUTLINE) {
            Name += N_("outline");
        } else if (desktop->getMode() == Inkscape::RENDERMODE_NO_FILTERS) {
            Name += N_("no filters");
        } else if (desktop->getMode() == Inkscape::RENDERMODE_VISIBLE_HAIRLINES) {
            Name += N_("visible hairlines");
        }

        if (desktop->getColorMode() != Inkscape::COLORMODE_NORMAL &&
            desktop->getMode()      != Inkscape::RENDERMODE_NORMAL) {
                Name += ", ";
        }

        if (desktop->getColorMode() == Inkscape::COLORMODE_GRAYSCALE) {
            Name += N_("grayscale");
        } else if (desktop->getColorMode() == Inkscape::COLORMODE_PRINT_COLORS_PREVIEW) {
            Name += N_("print colors preview");
        }

        if (*Name.rbegin() == '(') {  // Can not use C++11 .back() or .pop_back() with ustring!
            Name.erase(Name.size() - 2);
        } else {
            Name += ")";
        }

        Name += " - Inkscape";

        // Name += " (";
        // Name += Inkscape::version_string;
        // Name += ")";

        window->set_title (Name);
    }
}

Inkscape::UI::Widget::Dock*
SPDesktopWidget::getDock()
{
    return _dock;
}

/**
 * Callback to allocate space for desktop widget.
 */
static void
sp_desktop_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    SPDesktopWidget *dtw = SP_DESKTOP_WIDGET (widget);
    GtkAllocation widg_allocation;
    gtk_widget_get_allocation(widget, &widg_allocation);

    if ((allocation->x == widg_allocation.x) &&
        (allocation->y == widg_allocation.y) &&
        (allocation->width == widg_allocation.width) &&
        (allocation->height == widg_allocation.height)) {
        if (GTK_WIDGET_CLASS (dtw_parent_class)->size_allocate)
            GTK_WIDGET_CLASS (dtw_parent_class)->size_allocate (widget, allocation);
        return;
    }

    if (gtk_widget_get_realized (widget)) {
        Geom::Rect const area = dtw->desktop->get_display_area();
        Geom::Rect const d_canvas = dtw->desktop->getCanvas()->getViewbox();
        Geom::Point midpoint = dtw->desktop->w2d(d_canvas.midpoint());

        double zoom = dtw->desktop->current_zoom();

        if (GTK_WIDGET_CLASS(dtw_parent_class)->size_allocate) {
            GTK_WIDGET_CLASS(dtw_parent_class)->size_allocate (widget, allocation);
        }

        if (dtw->get_sticky_zoom_active()) {
            /* Find new visible area */
            Geom::Rect newarea = dtw->desktop->get_display_area();
            /* Calculate adjusted zoom */
            double oldshortside = MIN(   area.width(),    area.height());
            double newshortside = MIN(newarea.width(), newarea.height());
            zoom *= newshortside / oldshortside;
        }
        dtw->desktop->zoom_absolute_center_point (midpoint, zoom);

        // TODO - Should call show_dialogs() from sp_namedview_window_from_document only.
        // But delaying the call to here solves dock sizing issues on OS X, (see #171579)
        dtw->desktop->show_dialogs();

    } else {
        if (GTK_WIDGET_CLASS (dtw_parent_class)->size_allocate) {
            GTK_WIDGET_CLASS (dtw_parent_class)->size_allocate (widget, allocation);
        }
//            this->size_allocate (widget, allocation);
    }
}

/**
 * Callback to realize desktop widget.
 */
static void
sp_desktop_widget_realize (GtkWidget *widget)
{

    SPDesktopWidget *dtw = SP_DESKTOP_WIDGET (widget);

    if (GTK_WIDGET_CLASS (dtw_parent_class)->realize)
        (* GTK_WIDGET_CLASS (dtw_parent_class)->realize) (widget);

    Geom::Rect d = Geom::Rect::from_xywh(Geom::Point(0,0), (dtw->desktop->doc())->getDimensions());

    if (d.width() < 1.0 || d.height() < 1.0) return;

    dtw->desktop->set_display_area (d, 10);

    dtw->updateNamedview();
    gchar *gtkThemeName;
    gboolean gtkApplicationPreferDarkTheme;
    GtkSettings *settings = gtk_settings_get_default();
    Gtk::Window *window = SP_ACTIVE_DESKTOP->getToplevel();
    if (settings && window) {
        g_object_get(settings, "gtk-theme-name", &gtkThemeName, NULL);
        g_object_get(settings, "gtk-application-prefer-dark-theme", &gtkApplicationPreferDarkTheme, NULL);
        bool dark = gtkApplicationPreferDarkTheme || Glib::ustring(gtkThemeName).find(":dark") != -1;
        if (!dark) {
            Glib::RefPtr<Gtk::StyleContext> stylecontext = window->get_style_context();
            Gdk::RGBA rgba;
            bool background_set = stylecontext->lookup_color("theme_bg_color", rgba);
            if (background_set && rgba.get_red() + rgba.get_green() + rgba.get_blue() < 1.0) {
                dark = true;
            }
        }
        if (dark) {
            window->get_style_context()->add_class("dark");
            window->get_style_context()->remove_class("bright");
        } else {
            window->get_style_context()->add_class("bright");
            window->get_style_context()->remove_class("dark");
        }
    }
}

/* This is just to provide access to common functionality from sp_desktop_widget_realize() above
   as well as from SPDesktop::change_document() */
void SPDesktopWidget::updateNamedview()
{
    // Listen on namedview modification
    // originally (prior to the sigc++ conversion) the signal was simply
    // connected twice rather than disconnecting the first connection
    modified_connection.disconnect();

    modified_connection = desktop->namedview->connectModified(sigc::mem_fun(*this, &SPDesktopWidget::namedviewModified));
    namedviewModified(desktop->namedview, SP_OBJECT_MODIFIED_FLAG);

    updateTitle( desktop->doc()->getName() );
}

/**
 * Callback to handle desktop widget event.
 */
gint
SPDesktopWidget::event(GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw)
{
    if (event->type == GDK_BUTTON_PRESS) {
        // defocus any spinbuttons
        gtk_widget_grab_focus (GTK_WIDGET(dtw->_canvas));
    }

    if ((event->type == GDK_BUTTON_PRESS) && (event->button.button == 3)) {
        if (event->button.state & GDK_SHIFT_MASK) {
            sp_canvas_arena_set_sticky (SP_CANVAS_ARENA (dtw->desktop->drawing), TRUE);
        } else {
            sp_canvas_arena_set_sticky (SP_CANVAS_ARENA (dtw->desktop->drawing), FALSE);
        }
    }

    if (GTK_WIDGET_CLASS (dtw_parent_class)->event) {
        return (* GTK_WIDGET_CLASS (dtw_parent_class)->event) (widget, event);
    } else {
        // The key press/release events need to be passed to desktop handler explicitly,
        // because otherwise the event contexts only receive key events when the mouse cursor
        // is over the canvas. This redirection is only done for key events and only if there's no
        // current item on the canvas, because item events and all mouse events are caught
        // and passed on by the canvas acetate (I think). --bb
        if ((event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)
                && !dtw->_canvas->_current_item) {
            return sp_desktop_root_handler (nullptr, event, dtw->desktop);
        }
    }

    return FALSE;
}


#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
void
SPDesktopWidget::color_profile_event(EgeColorProfTracker */*tracker*/, SPDesktopWidget *dtw)
{
    // Handle profile changes
    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(dtw));
    GdkWindow *window = gtk_widget_get_window(gtk_widget_get_toplevel(GTK_WIDGET(dtw)));

    // In old Gtk+ versions, we can directly find the ID number for a monitor.
    // In Gtk+ >= 3.22, however, we need to figure out the ID
# if GTK_CHECK_VERSION(3,22,0)
    auto display = gdk_display_get_default();
    auto monitor = gdk_display_get_monitor_at_window(display, window);

    int n_monitors = gdk_display_get_n_monitors(display);

    int monitorNum = -1;

    // Now loop through the set of monitors and figure out whether this monitor matches
    for (int i_monitor = 0; i_monitor < n_monitors; ++i_monitor) {
        auto monitor_at_index = gdk_display_get_monitor(display, i_monitor);
        if(monitor_at_index == monitor) monitorNum = i_monitor;
    }
# else // GTK_CHECK_VERSION(3,22,0)
    gint monitorNum = gdk_screen_get_monitor_at_window(screen, window);
# endif // GTK_CHECK_VERSION(3,22,0)

    Glib::ustring id = Inkscape::CMSSystem::getDisplayId( monitorNum );
    bool enabled = false;
    dtw->_canvas->_cms_key = id;
    dtw->requestCanvasUpdate();
    enabled = !dtw->_canvas->_cms_key.empty();
    dtw->cms_adjust_set_sensitive(enabled);
}
#else // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
void sp_dtw_color_profile_event(EgeColorProfTracker */*tracker*/, SPDesktopWidget * /*dtw*/)
{
}
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

void
SPDesktopWidget::update_guides_lock()
{
    bool down = _guides_lock->get_active();

    auto doc  = desktop->getDocument();
    auto nv   = desktop->getNamedView();
    auto repr = nv->getRepr();

    if ( down != nv->lockguides ) {
        nv->lockguides = down;
        sp_namedview_guides_toggle_lock(doc, nv);
        if (down) {
            setMessage (Inkscape::NORMAL_MESSAGE, _("Locked all guides"));
        } else {
            setMessage (Inkscape::NORMAL_MESSAGE, _("Unlocked all guides"));
        }
    }
}

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
void
SPDesktopWidget::cms_adjust_toggled( GtkWidget */*button*/, gpointer data )
{
    SPDesktopWidget *dtw = SP_DESKTOP_WIDGET(data);

    bool down = dtw->_cms_adjust->get_active();
    if ( down != dtw->_canvas->_enable_cms_display_adj ) {
        dtw->_canvas->_enable_cms_display_adj = down;
        dtw->desktop->redrawDesktop();
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool("/options/displayprofile/enable", down);
        if (down) {
            dtw->setMessage (Inkscape::NORMAL_MESSAGE, _("Color-managed display is <b>enabled</b> in this window"));
        } else {
            dtw->setMessage (Inkscape::NORMAL_MESSAGE, _("Color-managed display is <b>disabled</b> in this window"));
        }
    }
}
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

void
SPDesktopWidget::cms_adjust_set_sensitive(bool enabled)
{
    Inkscape::Verb* verb = Inkscape::Verb::get( SP_VERB_VIEW_CMS_TOGGLE );
    if ( verb ) {
        SPAction *act = verb->get_action( Inkscape::ActionContext(viewwidget.view) );
        if ( act ) {
            sp_action_set_sensitive( act, enabled );
        }
    }
    _cms_adjust->set_sensitive(enabled);
}

void
sp_dtw_desktop_activate (SPDesktopWidget */*dtw*/)
{
    /* update active desktop indicator */
}

void
sp_dtw_desktop_deactivate (SPDesktopWidget */*dtw*/)
{
    /* update inactive desktop indicator */
}

/**
 *  Shuts down the desktop object for the view being closed.  It checks
 *  to see if the document has been edited, and if so prompts the user
 *  to save, discard, or cancel.  Returns TRUE if the shutdown operation
 *  is cancelled or if the save is cancelled or fails, FALSE otherwise.
 */
bool
SPDesktopWidget::shutdown()
{
    g_assert(desktop != nullptr);

    if (INKSCAPE.sole_desktop_for_document(*desktop)) {
        SPDocument *doc = desktop->doc();
        if (doc->isModifiedSinceSave()) {
            Gtk::Window *toplevel_window = Glib::wrap(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(this))));
            Glib::ustring message = g_markup_printf_escaped(
                _("<span weight=\"bold\" size=\"larger\">Save changes to document \"%s\" before closing?</span>\n\n"
                  "If you close without saving, your changes will be discarded."),
                doc->getName());
            Gtk::MessageDialog dialog = Gtk::MessageDialog(*toplevel_window, message, true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);
            dialog.property_destroy_with_parent() = true;

            // fix for bug lp:168809
            Gtk::Container *ma = dialog.get_message_area();
            std::vector<Gtk::Widget*> ma_labels = ma->get_children();
            ma_labels[0]->set_can_focus(false);

            Gtk::Button close_button(_("Close _without saving"), true);
            close_button.show();
            dialog.add_action_widget(close_button, Gtk::RESPONSE_NO);

            dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
            dialog.add_button(_("_Save"), Gtk::RESPONSE_YES);
            dialog.set_default_response(Gtk::RESPONSE_YES);

            gint response = dialog.run();

            switch (response) {
            case GTK_RESPONSE_YES:
            {
                doc->doRef();
                sp_namedview_document_from_window(desktop);
                if (sp_file_save_document(*window, doc)) {
                    doc->doUnref();
                } else { // save dialog cancelled or save failed
                    doc->doUnref();
                    return TRUE;
                }

                break;
            }
            case GTK_RESPONSE_NO:
                break;
            default: // cancel pressed, or dialog was closed
                return TRUE;
                break;
            }
        }
        /* Code to check data loss */
        bool allow_data_loss = FALSE;
        while (doc->getReprRoot()->attribute("inkscape:dataloss") != nullptr && allow_data_loss == FALSE) {
            Gtk::Window *toplevel_window = Glib::wrap(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(this))));
            Glib::ustring message = g_markup_printf_escaped(
                _("<span weight=\"bold\" size=\"larger\">The file \"%s\" was saved with a format that may cause data loss!</span>\n\n"
                  "Do you want to save this file as Inkscape SVG?"),
                doc->getName() ? doc->getName() : "Unnamed");
            Gtk::MessageDialog dialog = Gtk::MessageDialog(*toplevel_window, message, true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);
            dialog.property_destroy_with_parent() = true;

            // fix for bug lp:168809
            Gtk::Container *ma = dialog.get_message_area();
            std::vector<Gtk::Widget*> ma_labels = ma->get_children();
            ma_labels[0]->set_can_focus(false);

            Gtk::Button close_button(_("Close _without saving"), true);
            close_button.show();
            dialog.add_action_widget(close_button, Gtk::RESPONSE_NO);

            dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);

            Gtk::Button save_button(_("_Save as Inkscape SVG"), true);
            save_button.set_can_default(true);
            save_button.show();
            dialog.add_action_widget(save_button, Gtk::RESPONSE_YES);
            dialog.set_default_response(Gtk::RESPONSE_YES);

            gint response = dialog.run();

            switch (response) {
            case GTK_RESPONSE_YES:
            {
                doc->doRef();

                if (sp_file_save_dialog(*window, doc, Inkscape::Extension::FILE_SAVE_METHOD_INKSCAPE_SVG)) {
                    doc->doUnref();
                } else { // save dialog cancelled or save failed
                    doc->doUnref();
                    return TRUE;
                }

                break;
            }
            case GTK_RESPONSE_NO:
                allow_data_loss = TRUE;
                break;
            default: // cancel pressed, or dialog was closed
                return TRUE;
                break;
            }
        }
    }

    /* Save window geometry to prefs for use as a default.
     * Use depends on setting of "options.savewindowgeometry".
     * But we save the info here regardless of the setting.
     */
    storeDesktopPosition();

    return FALSE;
}

/**
 * \store dessktop position
 */
void SPDesktopWidget::storeDesktopPosition()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool maxed = desktop->is_maximized();
    bool full = desktop->is_fullscreen();
    prefs->setBool("/desktop/geometry/fullscreen", full);
    prefs->setBool("/desktop/geometry/maximized", maxed);
    gint w, h, x, y;
    desktop->getWindowGeometry(x, y, w, h);
    // Don't save geom for maximized windows.  It
    // just tells you the current maximized size, which is not
    // as useful as whatever value it had previously.
    if (!maxed && !full) {
        prefs->setInt("/desktop/geometry/width", w);
        prefs->setInt("/desktop/geometry/height", h);
        prefs->setInt("/desktop/geometry/x", x);
        prefs->setInt("/desktop/geometry/y", y);
    }
}

/**
 * \pre this->desktop->main != 0
 */
void
SPDesktopWidget::requestCanvasUpdate() {
    // ^^ also this->desktop != 0
    g_return_if_fail(this->desktop != nullptr);
    g_return_if_fail(this->desktop->main != nullptr);
    gtk_widget_queue_draw (GTK_WIDGET (SP_CANVAS_ITEM (this->desktop->main)->canvas));
}

void
SPDesktopWidget::requestCanvasUpdateAndWait() {
    requestCanvasUpdate();

    while (gtk_events_pending())
      gtk_main_iteration_do(FALSE);

}

void
SPDesktopWidget::enableInteraction()
{
  g_return_if_fail(_interaction_disabled_counter > 0);

  _interaction_disabled_counter--;

  if (_interaction_disabled_counter == 0) {
    gtk_widget_set_sensitive(GTK_WIDGET(this), TRUE);
  }
}

void
SPDesktopWidget::disableInteraction()
{
  if (_interaction_disabled_counter == 0) {
    gtk_widget_set_sensitive(GTK_WIDGET(this), FALSE);
  }

  _interaction_disabled_counter++;
}

void
SPDesktopWidget::setCoordinateStatus(Geom::Point p)
{
    gchar *cstr;
    cstr = g_strdup_printf("%7.2f", _dt2r * p[Geom::X]);
    _coord_status_x->set_markup(cstr);
    g_free(cstr);

    cstr = g_strdup_printf("%7.2f", _dt2r * p[Geom::Y]);
    _coord_status_y->set_markup(cstr);
    g_free(cstr);
}

void
SPDesktopWidget::letZoomGrabFocus()
{
    if (_zoom_status) _zoom_status->grab_focus();
}

void
SPDesktopWidget::getWindowGeometry (gint &x, gint &y, gint &w, gint &h)
{
    if (window)
    {
        window->get_size (w, h);
        window->get_position (x, y);
    }
}

void
SPDesktopWidget::setWindowPosition (Geom::Point p)
{
    if (window)
    {
        window->move (gint(round(p[Geom::X])), gint(round(p[Geom::Y])));
    }
}

void
SPDesktopWidget::setWindowSize (gint w, gint h)
{
    if (window)
    {
        window->set_default_size (w, h);
        window->resize (w, h);
    }
}

/**
 * \note transientizing does not work on windows; when you minimize a document
 * and then open it back, only its transient emerges and you cannot access
 * the document window. The document window must be restored by rightclicking
 * the taskbar button and pressing "Restore"
 */
void
SPDesktopWidget::setWindowTransient (void *p, int transient_policy)
{
    if (window)
    {
        GtkWindow *w = GTK_WINDOW(window->gobj());
        gtk_window_set_transient_for (GTK_WINDOW(p), w);

        /*
         * This enables "aggressive" transientization,
         * i.e. dialogs always emerging on top when you switch documents. Note
         * however that this breaks "click to raise" policy of a window
         * manager because the switched-to document will be raised at once
         * (so that its transients also could raise)
         */
        if (transient_policy == 2)
            // without this, a transient window not always emerges on top
            gtk_window_present (w);
    }
}

void
SPDesktopWidget::presentWindow()
{
    GtkWindow *w =GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(this)));
    if (w)
        gtk_window_present (w);
}

bool SPDesktopWidget::showInfoDialog( Glib::ustring const &message )
{
    bool result = false;
    Gtk::Window *window = Glib::wrap(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(this))));
    if (window)
    {
        Gtk::MessageDialog dialog(*window, message, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK);
        dialog.property_destroy_with_parent() = true;
        dialog.set_name("InfoDialog");
        dialog.set_title(_("Note:")); // probably want to take this as a parameter.
        dialog.run();
    }
    return result;
}

bool SPDesktopWidget::warnDialog (Glib::ustring const &text)
{
    Gtk::MessageDialog dialog (*window, text, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);
    gint response = dialog.run();
    if (response == Gtk::RESPONSE_OK)
        return true;
    else
        return false;
}

void
SPDesktopWidget::iconify()
{
    GtkWindow *topw = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(_canvas)));
    if (GTK_IS_WINDOW(topw)) {
        if (desktop->is_iconified()) {
            gtk_window_deiconify(topw);
        } else {
            gtk_window_iconify(topw);
        }
    }
}

void
SPDesktopWidget::maximize()
{
    GtkWindow *topw = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(_canvas)));
    if (GTK_IS_WINDOW(topw)) {
        if (desktop->is_maximized()) {
            gtk_window_unmaximize(topw);
        } else {
            // Save geometry to prefs before maximizing so that
            // something useful is stored there, because GTK doesn't maintain
            // a separate non-maximized size.
            if (!desktop->is_iconified() && !desktop->is_fullscreen())
            {
                Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                gint w = -1;
                gint h, x, y;
                getWindowGeometry(x, y, w, h);
                g_assert(w != -1);
                prefs->setInt("/desktop/geometry/width", w);
                prefs->setInt("/desktop/geometry/height", h);
                prefs->setInt("/desktop/geometry/x", x);
                prefs->setInt("/desktop/geometry/y", y);
            }
            gtk_window_maximize(topw);
        }
    }
}

void
SPDesktopWidget::fullscreen()
{
    GtkWindow *topw = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(_canvas)));
    if (GTK_IS_WINDOW(topw)) {
        if (desktop->is_fullscreen()) {
            gtk_window_unfullscreen(topw);
            // widget layout is triggered by the resulting window_state_event
        } else {
            // Save geometry to prefs before maximizing so that
            // something useful is stored there, because GTK doesn't maintain
            // a separate non-maximized size.
            if (!desktop->is_iconified() && !desktop->is_maximized())
            {
                Inkscape::Preferences *prefs = Inkscape::Preferences::get();
                gint w, h, x, y;
                getWindowGeometry(x, y, w, h);
                prefs->setInt("/desktop/geometry/width", w);
                prefs->setInt("/desktop/geometry/height", h);
                prefs->setInt("/desktop/geometry/x", x);
                prefs->setInt("/desktop/geometry/y", y);
            }
            gtk_window_fullscreen(topw);
            // widget layout is triggered by the resulting window_state_event
        }
    }
}

/**
 * Hide whatever the user does not want to see in the window
 */
void SPDesktopWidget::layoutWidgets()
{
    SPDesktopWidget *dtw = this;
    Glib::ustring pref_root;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (dtw->desktop->is_focusMode()) {
        pref_root = "/focus/";
    } else if (dtw->desktop->is_fullscreen()) {
        pref_root = "/fullscreen/";
    } else {
        pref_root = "/window/";
    }

    if (!prefs->getBool(pref_root + "commands/state", true)) {
        gtk_widget_hide (dtw->commands_toolbox);
    } else {
        gtk_widget_show_all (dtw->commands_toolbox);
    }

    if (!prefs->getBool(pref_root + "snaptoolbox/state", true)) {
        gtk_widget_hide (dtw->snap_toolbox);
    } else {
        gtk_widget_show_all (dtw->snap_toolbox);
    }

    if (!prefs->getBool(pref_root + "toppanel/state", true)) {
        gtk_widget_hide (dtw->aux_toolbox);
    } else {
        // we cannot just show_all because that will show all tools' panels;
        // this is a function from toolbox.cpp that shows only the current tool's panel
        ToolboxFactory::showAuxToolbox(dtw->aux_toolbox);
    }

    if (!prefs->getBool(pref_root + "toolbox/state", true)) {
        gtk_widget_hide (dtw->tool_toolbox);
    } else {
        gtk_widget_show_all (dtw->tool_toolbox);
    }

    if (!prefs->getBool(pref_root + "statusbar/state", true)) {
        dtw->_statusbar->hide();
    } else {
        dtw->_statusbar->show_all();
    }

    if (!prefs->getBool(pref_root + "panels/state", true)) {
        dtw->_panels->hide();
    } else {
        dtw->_panels->show_all();
    }

    if (!prefs->getBool(pref_root + "scrollbars/state", true)) {
        dtw->_hscrollbar->hide();
        dtw->_vscrollbar_box->hide();
        dtw->_cms_adjust->hide();
    } else {
        dtw->_hscrollbar->show_all();
        dtw->_vscrollbar_box->show_all();
        dtw->_cms_adjust->show_all();
    }

    if (!prefs->getBool(pref_root + "rulers/state", true)) {
        dtw->_guides_lock->hide();
        dtw->_hruler->hide();
        dtw->_vruler->hide();
    } else {
        dtw->_guides_lock->show_all();
        dtw->_hruler->show_all();
        dtw->_vruler->show_all();
    }
}

void
SPDesktopWidget::setToolboxFocusTo (const gchar* label)
{
    // First try looking for a named widget
    auto hb = sp_search_by_name_recursive(Glib::wrap(aux_toolbox), label);

    // Fallback to looking for a named data member (deprecated)
    if (!hb) {
        hb = Glib::wrap(GTK_WIDGET(sp_search_by_data_recursive(aux_toolbox, (gpointer) label)));
    }

    if (hb)
    {
        hb->grab_focus();
    }
}

void
SPDesktopWidget::setToolboxAdjustmentValue (gchar const *id, double value)
{
    // First try looking for a named widget
    auto hb = sp_search_by_name_recursive(Glib::wrap(aux_toolbox), id);

    // Fallback to looking for a named data member (deprecated)
    if (!hb) {
        hb = Glib::wrap(GTK_WIDGET(sp_search_by_data_recursive(aux_toolbox, (gpointer)id)));
    }

    if (hb) {
        auto sb = dynamic_cast<Inkscape::UI::Widget::SpinButtonToolItem *>(hb);
        auto a = sb->get_adjustment();

        if(a) a->set_value(value);
    }

    else g_warning ("Could not find GtkAdjustment for %s\n", id);
}

void
SPDesktopWidget::setToolboxSelectOneValue (gchar const *id, int value)
{
    gpointer hb = sp_search_by_data_recursive(aux_toolbox, (gpointer) id);
    if (static_cast<InkSelectOneAction*>(hb)) {
        static_cast<InkSelectOneAction*>(hb)->set_active( value );
    }
}


bool
SPDesktopWidget::isToolboxButtonActive (const gchar* id)
{
    bool isActive = false;
    gpointer thing = sp_search_by_data_recursive(aux_toolbox, (gpointer) id);
    if ( !thing ) {
        //g_message( "Unable to locate item for {%s}", id );
    } else if ( GTK_IS_TOGGLE_BUTTON(thing) ) {
        GtkToggleButton *b = GTK_TOGGLE_BUTTON(thing);
        isActive = gtk_toggle_button_get_active( b ) != 0;
    } else if ( GTK_IS_TOGGLE_ACTION(thing) ) {
        GtkToggleAction* act = GTK_TOGGLE_ACTION(thing);
        isActive = gtk_toggle_action_get_active( act ) != 0;
    } else {
        //g_message( "Item for {%s} is of an unsupported type", id );
    }

    return isActive;
}

void SPDesktopWidget::setToolboxPosition(Glib::ustring const& id, GtkPositionType pos)
{
    // Note - later on these won't be individual member variables.
    GtkWidget* toolbox = nullptr;
    if (id == "ToolToolbar") {
        toolbox = tool_toolbox;
    } else if (id == "AuxToolbar") {
        toolbox = aux_toolbox;
    } else if (id == "CommandsToolbar") {
        toolbox = commands_toolbox;
    } else if (id == "SnapToolbar") {
        toolbox = snap_toolbox;
    }


    if (toolbox) {
        switch(pos) {
            case GTK_POS_TOP:
            case GTK_POS_BOTTOM:
                if ( gtk_widget_is_ancestor(toolbox, GTK_WIDGET(_hbox->gobj())) ) {
                    // Removing a widget can reduce ref count to zero
                    g_object_ref(G_OBJECT(toolbox));
                    _hbox->remove(*Glib::wrap(toolbox));
                    _vbox->add(*Glib::wrap(toolbox));
                    g_object_unref(G_OBJECT(toolbox));

                    // Function doesn't seem to be in Gtkmm wrapper yet
                    gtk_box_set_child_packing(_vbox->gobj(), toolbox, FALSE, TRUE, 0, GTK_PACK_START);
                }
                ToolboxFactory::setOrientation(toolbox, GTK_ORIENTATION_HORIZONTAL);
                break;
            case GTK_POS_LEFT:
            case GTK_POS_RIGHT:
                if ( !gtk_widget_is_ancestor(toolbox, GTK_WIDGET(_hbox->gobj())) ) {
                    g_object_ref(G_OBJECT(toolbox));
                    _vbox->remove(*Glib::wrap(toolbox));
                    _hbox->add(*Glib::wrap(toolbox));
                    g_object_unref(G_OBJECT(toolbox));

                    // Function doesn't seem to be in Gtkmm wrapper yet
                    gtk_box_set_child_packing(_hbox->gobj(), toolbox, FALSE, TRUE, 0, GTK_PACK_START);
                    if (pos == GTK_POS_LEFT) {
                        _hbox->reorder_child(*Glib::wrap(toolbox), 0 );
                    }
                }
                ToolboxFactory::setOrientation(toolbox, GTK_ORIENTATION_VERTICAL);
                break;
        }
    }
}


SPDesktopWidget *sp_desktop_widget_new(SPDocument *document)
{
    SPDesktopWidget* dtw = SPDesktopWidget::createInstance(document);
    return dtw;
}

SPDesktopWidget* SPDesktopWidget::createInstance(SPDocument *document)
{
    SPDesktopWidget *dtw = static_cast<SPDesktopWidget*>(g_object_new(SP_TYPE_DESKTOP_WIDGET, nullptr));

    SPNamedView *namedview = sp_document_namedview(document, nullptr);

    dtw->_dt2r = 1. / namedview->display_units->factor;

    dtw->_ruler_origin = Geom::Point(0,0); //namedview->gridorigin;   Why was the grid origin used here?

    dtw->desktop = new SPDesktop();
    dtw->stub = new SPDesktopWidget::WidgetStub (dtw);
    dtw->desktop->init (namedview, dtw->_canvas, dtw->stub);
    INKSCAPE.add_desktop (dtw->desktop);

    // Add the shape geometry to libavoid for autorouting connectors.
    // This needs desktop set for its spacing preferences.
    init_avoided_shape_geometry(dtw->desktop);

    dtw->_selected_style->setDesktop(dtw->desktop);

    /* Once desktop is set, we can update rulers */
    dtw->update_rulers();

    sp_view_widget_set_view (SP_VIEW_WIDGET (dtw), dtw->desktop);

    /* Listen on namedview modification */
    dtw->modified_connection = namedview->connectModified(sigc::mem_fun(*dtw, &SPDesktopWidget::namedviewModified));

    dtw->layer_selector->setDesktop(dtw->desktop);

    // TEMP
    dtw->_menubar = build_menubar(dtw->desktop);
    dtw->_menubar->set_name("MenuBar");
    dtw->_menubar->show_all();
    dtw->_vbox->pack_start(*dtw->_menubar, false, false);

    dtw->layoutWidgets();

    std::vector<GtkWidget *> toolboxes;
    toolboxes.push_back(dtw->tool_toolbox);
    toolboxes.push_back(dtw->aux_toolbox);
    toolboxes.push_back(dtw->commands_toolbox);
    toolboxes.push_back(dtw->snap_toolbox);

    dtw->_panels->setDesktop( dtw->desktop );

    UXManager::getInstance()->addTrack(dtw);
    UXManager::getInstance()->connectToDesktop( toolboxes, dtw->desktop );

    return dtw;
}


void
SPDesktopWidget::update_rulers()
{
    Geom::Rect viewbox = desktop->get_display_area();

    double lower_x = _dt2r * (viewbox.left()  - _ruler_origin[Geom::X]);
    double upper_x = _dt2r * (viewbox.right() - _ruler_origin[Geom::X]);
    sp_ruler_set_range(SP_RULER(_hruler->gobj()),
	      	       lower_x,
		       upper_x,
		       (upper_x - lower_x));

    double lower_y = _dt2r * (viewbox.bottom() - _ruler_origin[Geom::Y]);
    double upper_y = _dt2r * (viewbox.top()    - _ruler_origin[Geom::Y]);
    if (desktop->is_yaxisdown()) {
        std::swap(lower_y, upper_y);
    }
    sp_ruler_set_range(SP_RULER(_vruler->gobj()),
                       lower_y,
		       upper_y,
		       (upper_y - lower_y));
}


void SPDesktopWidget::namedviewModified(SPObject *obj, guint flags)
{
    SPNamedView *nv=SP_NAMEDVIEW(obj);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        _dt2r = 1. / nv->display_units->factor;
        _ruler_origin = Geom::Point(0,0); //nv->gridorigin;   Why was the grid origin used here?

        sp_ruler_set_unit(SP_RULER(_vruler->gobj()), nv->getDisplayUnit());
        sp_ruler_set_unit(SP_RULER(_hruler->gobj()), nv->getDisplayUnit());

        /* This loops through all the grandchildren of aux toolbox,
         * and for each that it finds, it performs an sp_search_by_data_recursive(),
         * looking for widgets that hold some "tracker" data (this is used by
         * all toolboxes to refer to the unit selector). The default document units
         * is then selected within these unit selectors.
         *
         * Of course it would be nice to be able to refer to the toolbox and the
         * unit selector directly by name, but I don't yet see a way to do that.
         *
         * This should solve: https://bugs.launchpad.net/inkscape/+bug/362995
         */
        if (GTK_IS_CONTAINER(aux_toolbox)) {
            std::vector<Gtk::Widget*> ch = Glib::wrap(GTK_CONTAINER(aux_toolbox))->get_children();
            for (auto i:ch) {
                if (GTK_IS_CONTAINER(i->gobj())) {
                    std::vector<Gtk::Widget*> grch = dynamic_cast<Gtk::Container*>(i)->get_children();
                    for (auto j:grch) {

                        if (!GTK_IS_WIDGET(j->gobj())) // wasn't a widget
                            continue;

                        // Don't apply to text toolbar. We want to be able to
                        // use different units for text. (Bug 1562217)
                        const Glib::ustring name = j->get_name();
                        if ( name == "TextToolbar")
                            continue;

                        gpointer t = sp_search_by_data_recursive(GTK_WIDGET(j->gobj()), (gpointer) "tracker");
                        if (t == nullptr) // didn't find any tracker data
                            continue;

                        UnitTracker *tracker = reinterpret_cast<UnitTracker*>( t );
                        if (tracker == nullptr) // it's null when inkscape is first opened
                            continue;

                        tracker->setActiveUnit( nv->display_units );
                    } // grandchildren
                } // if child is a container
            } // children
        } // if aux_toolbox is a container

        _hruler_box->set_tooltip_text(gettext(nv->display_units->name_plural.c_str()));
        _vruler_box->set_tooltip_text(gettext(nv->display_units->name_plural.c_str()));

        update_rulers();
        ToolboxFactory::updateSnapToolbox(this->desktop, nullptr, this->snap_toolbox);
    }
}

void
SPDesktopWidget::on_adjustment_value_changed()
{
    if (update)
        return;

    update = 1;

    // Do not call canvas->scrollTo directly... messes up 'offset'.
    desktop->scroll_absolute( Geom::Point(_hadj->get_value(),
                                          _vadj->get_value()), false);

    update = 0;
}

/* we make the desktop window with focus active, signal is connected in interface.c */
bool SPDesktopWidget::onFocusInEvent(GdkEventFocus*)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/bitmapautoreload/value", true)) {
        std::vector<SPObject *> imageList = (desktop->doc())->getResourceList("image");
        for (std::vector<SPObject *>::const_iterator it = imageList.begin(); it != imageList.end(); ++it) {
            SPImage* image = SP_IMAGE(*it);
            image->refresh_if_outdated();
        }
    }

    INKSCAPE.activate_desktop (desktop);

    return false;
}

// ------------------------ Zoom ------------------------
static gdouble
sp_dtw_zoom_value_to_display (gdouble value)
{
    return floor (10 * (pow (2, value) * 100.0 + 0.05)) / 10;
}

static gdouble
sp_dtw_zoom_display_to_value (gdouble value)
{
    return  log (value / 100.0) / log (2);
}

int
SPDesktopWidget::zoom_input(double *new_val)
{
    gchar *b = g_strdup(_zoom_status->get_text().c_str());

    gchar *comma = g_strstr_len (b, -1, ",");
    if (comma) {
        *comma = '.';
    }

    char *oldlocale = g_strdup (setlocale(LC_NUMERIC, nullptr));
    setlocale (LC_NUMERIC, "C");
    gdouble new_typed = atof (b);
    setlocale (LC_NUMERIC, oldlocale);
    g_free (oldlocale);
    g_free (b);

    *new_val = sp_dtw_zoom_display_to_value (new_typed);
    return TRUE;
}

bool
SPDesktopWidget::zoom_output()
{
    gchar b[64];
    double val = sp_dtw_zoom_value_to_display (_zoom_status->get_value());
    if (val < 10) {
        g_snprintf (b, 64, "%4.1f%%", val);
    } else {
        g_snprintf (b, 64, "%4.0f%%", val);
    }
    _zoom_status->set_text(b);
    return true;
}

void
SPDesktopWidget::zoom_value_changed()
{
    double const zoom_factor = pow (2, _zoom_status->get_value());

    // Zoom around center of window
    Geom::Rect const d_canvas = desktop->getCanvas()->getViewbox();
    Geom::Point midpoint = desktop->w2d(d_canvas.midpoint());
    _zoom_status_value_changed_connection.block();
    desktop->zoom_absolute_center_point (midpoint, zoom_factor);
    _zoom_status_value_changed_connection.unblock();

    spinbutton_defocus(GTK_WIDGET(_zoom_status->gobj()));
}

void
SPDesktopWidget::zoom_menu_handler(double factor)
{
    Geom::Rect const d = desktop->get_display_area();
    desktop->zoom_absolute_center_point (d.midpoint(), factor);
}

void
SPDesktopWidget::zoom_populate_popup(Gtk::Menu *menu)
{
    for ( auto iter : menu->get_children()) {
        menu->remove(*iter);
    }

    auto item_1000 = Gtk::manage(new Gtk::MenuItem("1000%"));
    auto item_500  = Gtk::manage(new Gtk::MenuItem("500%"));
    auto item_200  = Gtk::manage(new Gtk::MenuItem("200%"));
    auto item_100  = Gtk::manage(new Gtk::MenuItem("100%"));
    auto item_50   = Gtk::manage(new Gtk::MenuItem( "50%"));
    auto item_25   = Gtk::manage(new Gtk::MenuItem( "25%"));
    auto item_10   = Gtk::manage(new Gtk::MenuItem( "10%"));

    item_1000->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler), 10.00));
    item_500->signal_activate().connect( sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler),  5.00));
    item_200->signal_activate().connect( sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler),  2.00));
    item_100->signal_activate().connect( sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler),  1.00));
    item_50->signal_activate().connect(  sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler),  0.50));
    item_25->signal_activate().connect(  sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler),  0.25));
    item_10->signal_activate().connect(  sigc::bind(sigc::mem_fun(this, &SPDesktopWidget::zoom_menu_handler),  0.10));

    menu->append(*item_1000);
    menu->append(*item_500);
    menu->append(*item_200);
    menu->append(*item_100);
    menu->append(*item_50);
    menu->append(*item_25);
    menu->append(*item_10);

    auto sep = Gtk::manage(new Gtk::SeparatorMenuItem());
    menu->append(*sep);

    auto item_page = Gtk::manage(new Gtk::MenuItem(_("Page")));
    item_page->signal_activate().connect(sigc::mem_fun(desktop, &SPDesktop::zoom_page));
    menu->append(*item_page);

    auto item_drawing = Gtk::manage(new Gtk::MenuItem(_("Drawing")));
    item_drawing->signal_activate().connect(sigc::mem_fun(desktop, &SPDesktop::zoom_drawing));
    menu->append(*item_drawing);

    auto item_selection = Gtk::manage(new Gtk::MenuItem(_("Selection")));
    item_selection->signal_activate().connect(sigc::mem_fun(desktop, &SPDesktop::zoom_selection));
    menu->append(*item_selection);

    menu->show_all();
}


void
SPDesktopWidget::sticky_zoom_toggled()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/options/stickyzoom/value", _sticky_zoom->get_active());
}


void
SPDesktopWidget::update_zoom()
{
    _zoom_status_value_changed_connection.block();
    _zoom_status->set_value(log(desktop->current_zoom()) / log(2));
    _zoom_status->queue_draw();
    _zoom_status_value_changed_connection.unblock();
}


// ---------------------- Rotation ------------------------
int
SPDesktopWidget::rotation_input(double *new_val)
{
    auto *b = g_strdup(_rotation_status->get_text().c_str());

    gchar *comma = g_strstr_len (b, -1, ",");
    if (comma) {
        *comma = '.';
    }

    char *oldlocale = g_strdup (setlocale(LC_NUMERIC, nullptr));
    setlocale (LC_NUMERIC, "C");
    gdouble new_value = atof (b);
    setlocale (LC_NUMERIC, oldlocale);
    g_free (oldlocale);
    g_free (b);

    *new_val = new_value;
    return true;
}

bool
SPDesktopWidget::rotation_output()
{
    gchar b[64];
    double val = _rotation_status->get_value();

    if (val < -180) val += 360;
    if (val >  180) val -= 360;

    g_snprintf (b, 64, "%7.2f°", val);

    _rotation_status->set_text(b);
    return true;
}

void
SPDesktopWidget::rotation_value_changed()
{
    double const rotate_factor = M_PI / 180.0 * _rotation_status->get_value();
    // std::cout << "SPDesktopWidget::rotation_value_changed: "
    //           << _rotation_status->get_value()
    //           << "  (" << rotate_factor << ")" <<std::endl;

    // Rotate around center of window
    Geom::Rect const d_canvas = desktop->getCanvas()->getViewbox();
    _rotation_status_value_changed_connection.block();
    Geom::Point midpoint = desktop->w2d(d_canvas.midpoint());
    desktop->rotate_absolute_center_point (midpoint, rotate_factor);
    _rotation_status_value_changed_connection.unblock();

    spinbutton_defocus(GTK_WIDGET(_rotation_status->gobj()));
}

void
SPDesktopWidget::rotation_populate_popup(Gtk::Menu *menu)
{
    for ( auto iter : menu->get_children()) {
        menu->remove(*iter);
    }

    auto item_m135 = Gtk::manage(new Gtk::MenuItem("-135°"));
    auto item_m90  = Gtk::manage(new Gtk::MenuItem( "-90°"));
    auto item_m45  = Gtk::manage(new Gtk::MenuItem( "-45°"));
    auto item_0    = Gtk::manage(new Gtk::MenuItem(   "0°"));
    auto item_p45  = Gtk::manage(new Gtk::MenuItem(  "45°"));
    auto item_p90  = Gtk::manage(new Gtk::MenuItem(  "90°"));
    auto item_p135 = Gtk::manage(new Gtk::MenuItem( "135°"));
    auto item_p180 = Gtk::manage(new Gtk::MenuItem( "180°"));

    item_m135->signal_activate().connect(sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value), -135));
    item_m90->signal_activate().connect( sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value), -90));
    item_m45->signal_activate().connect( sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value), -45));
    item_0->signal_activate().connect(   sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value),   0));
    item_p45->signal_activate().connect( sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value),  45));
    item_p90->signal_activate().connect( sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value),  90));
    item_p135->signal_activate().connect(sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value), 135));
    item_p180->signal_activate().connect(sigc::bind(sigc::mem_fun(_rotation_status, &Gtk::SpinButton::set_value), 180));

    menu->append(*item_m135);
    menu->append(*item_m90);
    menu->append(*item_m45);
    menu->append(*item_0);
    menu->append(*item_p45);
    menu->append(*item_p90);
    menu->append(*item_p135);
    menu->append(*item_p180);

    menu->show_all();
}


void
SPDesktopWidget::update_rotation()
{
    _rotation_status_value_changed_connection.block();
    _rotation_status->set_value(desktop->current_rotation() / M_PI * 180.0);
    _rotation_status->queue_draw();
    _rotation_status_value_changed_connection.unblock();

}


// --------------- Rulers/Scrollbars/Etc. -----------------
void
SPDesktopWidget::toggle_rulers()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (_guides_lock->get_visible()) {
        _guides_lock->hide();
        _hruler->hide();
        _vruler->hide();
        prefs->setBool(desktop->is_fullscreen() ? "/fullscreen/rulers/state" : "/window/rulers/state", false);
    } else {
        _guides_lock->show_all();
        _hruler->show_all();
        _vruler->show_all();
        prefs->setBool(desktop->is_fullscreen() ? "/fullscreen/rulers/state" : "/window/rulers/state", true);
    }
}

void
SPDesktopWidget::toggle_scrollbars()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (_hscrollbar->get_visible()) {
        _hscrollbar->hide();
        _vscrollbar_box->hide();
        _cms_adjust->hide();
        prefs->setBool(desktop->is_fullscreen() ? "/fullscreen/scrollbars/state" : "/window/scrollbars/state", false);
    } else {
        _hscrollbar->show_all();
        _vscrollbar_box->show_all();
        _cms_adjust->show_all();
        prefs->setBool(desktop->is_fullscreen() ? "/fullscreen/scrollbars/state" : "/window/scrollbars/state", true);
    }
}

bool
SPDesktopWidget::get_color_prof_adj_enabled() const
{
    return _cms_adjust->get_sensitive() && _cms_adjust->get_active();
}

void
SPDesktopWidget::toggle_color_prof_adj()
{
    if (_cms_adjust->get_sensitive()) {
        if (_cms_adjust->get_active()) {
            _cms_adjust->toggle_set_down(false);
        } else {
            _cms_adjust->toggle_set_down(true);
        }
    }
}

static void
set_adjustment (Glib::RefPtr<Gtk::Adjustment> &adj, double l, double u, double ps, double si, double pi)
{
    if ((l != adj->get_lower()) ||
        (u != adj->get_upper()) ||
        (ps != adj->get_page_size()) ||
        (si != adj->get_step_increment()) ||
        (pi != adj->get_page_increment())) {
	    adj->set_lower(l);
	    adj->set_upper(u);
	    adj->set_page_size(ps);
	    adj->set_step_increment(si);
	    adj->set_page_increment(pi);
    }
}

void
SPDesktopWidget::update_scrollbars(double scale)
{
    if (update) return;
    update = 1;

    /* The desktop region we always show unconditionally */
    SPDocument *doc = desktop->doc();
    Geom::Rect darea ( Geom::Point(-doc->getWidth().value("px"), -doc->getHeight().value("px")),
                     Geom::Point(2 * doc->getWidth().value("px"), 2 * doc->getHeight().value("px"))  );

    Geom::OptRect deskarea;
    if (Inkscape::Preferences::get()->getInt("/tools/bounding_box") == 0) {
        deskarea = darea | doc->getRoot()->desktopVisualBounds();
    } else {
        deskarea = darea | doc->getRoot()->desktopGeometricBounds();
    }

    /* Canvas region we always show unconditionally */
    double const y_dir = desktop->yaxisdir();
    Geom::Rect carea( Geom::Point(deskarea->left() * scale - 64, (deskarea->top() * scale + 64) * y_dir),
                    Geom::Point(deskarea->right() * scale + 64, (deskarea->bottom() * scale - 64) * y_dir)  );

    Geom::Rect viewbox = _canvas->getViewbox();

    /* Viewbox is always included into scrollable region */
    carea = Geom::unify(carea, viewbox);

    set_adjustment(_hadj, carea.min()[Geom::X], carea.max()[Geom::X],
                   viewbox.dimensions()[Geom::X],
                   0.1 * viewbox.dimensions()[Geom::X],
                   viewbox.dimensions()[Geom::X]);
    _hadj->set_value(viewbox.min()[Geom::X]);

    set_adjustment(_vadj, carea.min()[Geom::Y], carea.max()[Geom::Y],
                   viewbox.dimensions()[Geom::Y],
                   0.1 * viewbox.dimensions()[Geom::Y],
                   viewbox.dimensions()[Geom::Y]);
    _vadj->set_value(viewbox.min()[Geom::Y]);

    update = 0;
}

bool
SPDesktopWidget::get_sticky_zoom_active() const
{
    return _sticky_zoom->get_active();
}

double
SPDesktopWidget::get_hruler_thickness() const
{
    auto allocation = _hruler->get_allocation();
    return allocation.get_height();
}

double
SPDesktopWidget::get_vruler_thickness() const
{
    auto allocation = _vruler->get_allocation();
    return allocation.get_width();
}

gint
SPDesktopWidget::ruler_event(GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw, bool horiz)
{
    switch (event->type) {
    case GDK_BUTTON_PRESS:
        dtw->on_ruler_box_button_press_event(&event->button, Glib::wrap(GTK_EVENT_BOX(widget)), horiz);
        break;
    case GDK_MOTION_NOTIFY:
        dtw->on_ruler_box_motion_notify_event(&event->motion, Glib::wrap(GTK_EVENT_BOX(widget)), horiz);
        break;
    case GDK_BUTTON_RELEASE:
        dtw->on_ruler_box_button_release_event(&event->button, Glib::wrap(GTK_EVENT_BOX(widget)), horiz);
        break;
    default:
            break;
    }

    return FALSE;
}

bool
SPDesktopWidget::on_ruler_box_motion_notify_event(GdkEventMotion *event, Gtk::EventBox *widget, bool horiz)
{
    if (horiz) {
        sp_event_context_snap_delay_handler(desktop->event_context, (gpointer) widget->gobj(), (gpointer) this, event, Inkscape::UI::Tools::DelayedSnapEvent::GUIDE_HRULER);
    }
    else {
        sp_event_context_snap_delay_handler(desktop->event_context, (gpointer) widget->gobj(), (gpointer) this, event, Inkscape::UI::Tools::DelayedSnapEvent::GUIDE_VRULER);
    }

    int wx, wy;

    GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(_canvas));

    gint width, height;

    gdk_window_get_device_position(window, event->device, &wx, &wy, nullptr);
    gdk_window_get_geometry(window, nullptr /*x*/, nullptr /*y*/, &width, &height);

    Geom::Point const event_win(wx, wy);

    if (_ruler_clicked) {
        Geom::Point const event_w(sp_canvas_window_to_world(_canvas, event_win));
        Geom::Point event_dt(desktop->w2d(event_w));

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        gint tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
        if ( ( abs( (gint) event->x - _xp ) < tolerance )
                && ( abs( (gint) event->y - _yp ) < tolerance ) ) {
            return false;
        }

        _ruler_dragged = true;

        // explicitly show guidelines; if I draw a guide, I want them on
        if ((horiz ? wy : wx) >= 0) {
            desktop->namedview->setGuides(true);
        }

        if (!(event->state & GDK_SHIFT_MASK)) {
            ruler_snap_new_guide(desktop, _active_guide, event_dt, _normal);
        }
        sp_guideline_set_normal(SP_GUIDELINE(_active_guide), _normal);
        sp_guideline_set_position(SP_GUIDELINE(_active_guide), event_dt);

        desktop->set_coordinate_status(event_dt);
    }

    return false;
}

bool
SPDesktopWidget::on_ruler_box_button_release_event(GdkEventButton *event, Gtk::EventBox *widget, bool horiz)
{
    int wx, wy;

    GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(_canvas));

    gint width, height;

    gdk_window_get_device_position(window, event->device, &wx, &wy, nullptr);
    gdk_window_get_geometry(window, nullptr /*x*/, nullptr /*y*/, &width, &height);

    Geom::Point const event_win(wx, wy);

    if (_ruler_clicked && event->button == 1) {
        sp_event_context_discard_delayed_snap_event(desktop->event_context);

#if GTK_CHECK_VERSION(3,20,0)
        auto seat = gdk_device_get_seat(event->device);
        gdk_seat_ungrab(seat);
#else
        gdk_device_ungrab(event->device, event->time);
#endif

        Geom::Point const event_w(sp_canvas_window_to_world(_canvas, event_win));
        Geom::Point event_dt(desktop->w2d(event_w));

        if (!(event->state & GDK_SHIFT_MASK)) {
            ruler_snap_new_guide(desktop, _active_guide, event_dt, _normal);
        }

        sp_canvas_item_destroy(_active_guide);
        _active_guide = nullptr;
        if ((horiz ? wy : wx) >= 0) {
            Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
            Inkscape::XML::Node *repr = xml_doc->createElement("sodipodi:guide");

            // If root viewBox set, interpret guides in terms of viewBox (90/96)
            double newx = event_dt.x();
            double newy = event_dt.y();

            // <sodipodi:guide> stores inverted y-axis coordinates
            if (desktop->is_yaxisdown()) {
                newy = desktop->doc()->getHeight().value("px") - newy;
                _normal[Geom::Y] *= -1.0;
            }

            SPRoot *root = desktop->doc()->getRoot();
            if( root->viewBox_set ) {
                newx = newx * root->viewBox.width()  / root->width.computed;
                newy = newy * root->viewBox.height() / root->height.computed;
            }
            sp_repr_set_point(repr, "position", Geom::Point( newx, newy ));
            sp_repr_set_point(repr, "orientation", _normal);
            desktop->namedview->appendChild(repr);
            Inkscape::GC::release(repr);
            DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE,
                    _("Create guide"));
        }
        desktop->set_coordinate_status(event_dt);

        if (!_ruler_dragged) {
            // Ruler click (without drag) toggle the guide visibility on and off
            Inkscape::XML::Node *repr = desktop->namedview->getRepr();
            sp_namedview_toggle_guides(desktop->getDocument(), repr);
        }

        _ruler_clicked = false;
        _ruler_dragged = false;
    }

    return false;
}

bool
SPDesktopWidget::on_ruler_box_button_press_event(GdkEventButton *event, Gtk::EventBox *widget, bool horiz)
{
    int wx, wy;

    GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(_canvas));

    gint width, height;

    gdk_window_get_device_position(window, event->device, &wx, &wy, nullptr);
    gdk_window_get_geometry(window, nullptr /*x*/, nullptr /*y*/, &width, &height);

    Geom::Point const event_win(wx, wy);

    if (event->button == 1) {
        _ruler_clicked = true;
        _ruler_dragged = false;
        // save click origin
        _xp = (gint) event->x;
        _yp = (gint) event->y;

        Geom::Point const event_w(sp_canvas_window_to_world(_canvas, event_win));
        Geom::Point const event_dt(desktop->w2d(event_w));

        // calculate the normal of the guidelines when dragged from the edges of rulers.
        auto const y_dir = desktop->yaxisdir();
        Geom::Point normal_bl_to_tr(1., y_dir); //bottomleft to topright
        Geom::Point normal_tr_to_bl(-1., y_dir); //topright to bottomleft
        normal_bl_to_tr.normalize();
        normal_tr_to_bl.normalize();
        Inkscape::CanvasGrid * grid = sp_namedview_get_first_enabled_grid(desktop->namedview);
        if (grid){
            if (grid->getGridType() == Inkscape::GRID_AXONOMETRIC ) {
                Inkscape::CanvasAxonomGrid *axonomgrid = dynamic_cast<Inkscape::CanvasAxonomGrid *>(grid);
                if (event->state & GDK_CONTROL_MASK) {
                    // guidelines normal to gridlines
                    normal_bl_to_tr = Geom::Point::polar(-axonomgrid->angle_rad[0], 1.0);
                    normal_tr_to_bl = Geom::Point::polar(axonomgrid->angle_rad[2], 1.0);
                } else {
                    normal_bl_to_tr = rot90(Geom::Point::polar(axonomgrid->angle_rad[2], 1.0));
                    normal_tr_to_bl = rot90(Geom::Point::polar(-axonomgrid->angle_rad[0], 1.0));
                }
            }
        }
        if (horiz) {
            if (wx < 50) {
                _normal = normal_bl_to_tr;
            } else if (wx > width - 50) {
                _normal = normal_tr_to_bl;
            } else {
                _normal = Geom::Point(0.,1.);
            }
        } else {
            if (wy < 50) {
                _normal = normal_bl_to_tr;
            } else if (wy > height - 50) {
                _normal = normal_tr_to_bl;
            } else {
                _normal = Geom::Point(1.,0.);
            }
        }

        _active_guide = sp_guideline_new(desktop->guides, nullptr, event_dt, _normal);
        sp_guideline_set_color(SP_GUIDELINE(_active_guide), desktop->namedview->guidehicolor);

        auto window = widget->get_window()->gobj();

#if GTK_CHECK_VERSION(3,20,0)
        auto seat = gdk_device_get_seat(event->device);
        gdk_seat_grab(seat,
                window,
                GDK_SEAT_CAPABILITY_ALL_POINTING,
                FALSE,
                nullptr,
                (GdkEvent*)event,
                nullptr,
                nullptr);
#else
        gdk_device_grab(event->device,
                window,
                GDK_OWNERSHIP_NONE,
                FALSE,
                (GdkEventMask)(GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK ),
                NULL,
                event->time);
#endif
    }

    return false;
}

GtkAllocation
SPDesktopWidget::get_canvas_allocation() const
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(_canvas), &allocation);
    return allocation;
}

void
SPDesktopWidget::ruler_snap_new_guide(SPDesktop *desktop, SPCanvasItem * /*guide*/, Geom::Point &event_dt, Geom::Point &normal)
{
    SnapManager &m = desktop->namedview->snap_manager;
    m.setup(desktop);
    // We're dragging a brand new guide, just pulled of the rulers seconds ago. When snapping to a
    // path this guide will change it slope to become either tangential or perpendicular to that path. It's
    // therefore not useful to try tangential or perpendicular snapping, so this will be disabled temporarily
    bool pref_perp = m.snapprefs.getSnapPerp();
    bool pref_tang = m.snapprefs.getSnapTang();
    m.snapprefs.setSnapPerp(false);
    m.snapprefs.setSnapTang(false);
    // We only have a temporary guide which is not stored in our document yet.
    // Because the guide snapper only looks in the document for guides to snap to,
    // we don't have to worry about a guide snapping to itself here
    Geom::Point normal_orig = normal;
    m.guideFreeSnap(event_dt, normal, false, false);
    // After snapping, both event_dt and normal have been modified accordingly; we'll take the normal (of the
    // curve we snapped to) to set the normal the guide. And rotate it by 90 deg. if needed
    if (pref_perp) { // Perpendicular snapping to paths is requested by the user, so let's do that
        if (normal != normal_orig) {
            normal = Geom::rot90(normal);
        }
    }
    if (!(pref_tang || pref_perp)) { // if we don't want to snap either perpendicularly or tangentially, then
        normal = normal_orig; // we must restore the normal to it's original state
    }
    // Restore the preferences
    m.snapprefs.setSnapPerp(pref_perp);
    m.snapprefs.setSnapTang(pref_tang);
    m.unSetup();
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
