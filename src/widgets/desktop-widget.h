// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_DESKTOP_WIDGET_H
#define SEEN_SP_DESKTOP_WIDGET_H

/** \file
 * SPDesktopWidget: handling Gtk events on a desktop.
 *
 * Authors:
 *      Jon A. Cruz <jon@joncruz.org> (c) 2010
 *      John Bintz <jcoswell@coswellproductions.org> (c) 2006
 *      Ralf Stephan <ralf@ark.in-berlin.de> (c) 2005
 *      Abhishek Sharma
 *      ? -2004
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <gtkmm.h>

#include "message.h"
#include "ui/view/view-widget.h"
#include "ui/view/edit-widget-interface.h"

#include <cstddef>
#include <sigc++/connection.h>
#include <2geom/point.h>

// forward declaration
typedef struct _EgeColorProfTracker EgeColorProfTracker;
struct SPCanvas;
struct SPCanvasItem;
class SPDocument;
class SPDesktop;
struct SPDesktopWidget;
class SPObject;

namespace Inkscape {
namespace UI {
namespace Dialog {
class SwatchesPanel;
} // namespace Dialog

namespace Widget {
class Button;
class LayerSelector;
class SelectedStyle;
class Ruler;
} // namespace Widget
} // namespace UI
} // namespace Inkscape

#define SP_TYPE_DESKTOP_WIDGET SPDesktopWidget::getType()
#define SP_DESKTOP_WIDGET(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidget))
#define SP_DESKTOP_WIDGET_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), SP_TYPE_DESKTOP_WIDGET, SPDesktopWidgetClass))
#define SP_IS_DESKTOP_WIDGET(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_DESKTOP_WIDGET))
#define SP_IS_DESKTOP_WIDGET_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SP_TYPE_DESKTOP_WIDGET))

/**
 * Create a new SPDesktopWidget
 */
SPDesktopWidget *sp_desktop_widget_new(SPDocument* document);

void sp_desktop_widget_show_decorations(SPDesktopWidget *dtw, gboolean show);
void sp_desktop_widget_update_hruler (SPDesktopWidget *dtw);
void sp_desktop_widget_update_vruler (SPDesktopWidget *dtw);

/* Show/hide rulers & scrollbars */
void sp_desktop_widget_update_scrollbars (SPDesktopWidget *dtw, double scale);

void sp_dtw_desktop_activate (SPDesktopWidget *dtw);
void sp_dtw_desktop_deactivate (SPDesktopWidget *dtw);

/// A GtkEventBox on an SPDesktop.
struct SPDesktopWidget {
    SPViewWidget viewwidget;

    unsigned int update : 1;

    sigc::connection modified_connection;

    SPDesktop *desktop;

    Gtk::Window *window;

    static void dispose(GObject *object);

private:
    // Flags for ruler event handling
    bool _ruler_clicked; ///< True if the ruler has been clicked
    bool _ruler_dragged; ///< True if a drag on the ruler is occurring

    SPCanvasItem *_active_guide; ///< The guide currently being handled during a ruler event
    Geom::Point _normal; ///< Normal to the guide currently being handled during ruler event
    int _xp; ///< x coordinate for start of drag
    int _yp; ///< y coordinate for start of drag

    // The root vbox of the window layout.
    Gtk::Box *_vbox;

    Gtk::Box *_hbox;

    Gtk::MenuBar *_menubar;  // TEMP
    Gtk::Box     *_statusbar;

    Inkscape::UI::Dialog::SwatchesPanel *_panels;

    Glib::RefPtr<Gtk::Adjustment> _hadj;
    Glib::RefPtr<Gtk::Adjustment> _vadj;

    Gtk::ToggleButton *_guides_lock;

    Inkscape::UI::Widget::Button *_cms_adjust;
    Gtk::ToggleButton *_sticky_zoom;
    Gtk::Grid *_coord_status;

    Gtk::Label *_coord_status_x;
    Gtk::Label *_coord_status_y;
    Gtk::SpinButton *_zoom_status;
    sigc::connection _zoom_status_input_connection;
    sigc::connection _zoom_status_output_connection;
    sigc::connection _zoom_status_value_changed_connection;
    sigc::connection _zoom_status_populate_popup_connection;
    Gtk::Label *_select_status;
    Gtk::SpinButton *_rotation_status;

    sigc::connection _rotation_status_input_connection;
    sigc::connection _rotation_status_output_connection;
    sigc::connection _rotation_status_value_changed_connection;
    sigc::connection _rotation_status_populate_popup_connection;

    Inkscape::UI::Widget::Dock *_dock;

    Gtk::Scrollbar *_hscrollbar;
    Gtk::Scrollbar *_vscrollbar;
    Gtk::Box *_vscrollbar_box;

    Inkscape::UI::Widget::SelectedStyle *_selected_style;

    /** A table for displaying the canvas, rulers etc */
    Gtk::Grid *_canvas_tbl;
    sigc::connection _canvas_tbl_size_allocate_connection;

    Gtk::EventBox *_hruler_box;
    Gtk::EventBox *_vruler_box; // eventboxes for setting tooltips

    /* Rulers */
    Inkscape::UI::Widget::Ruler *_hruler;
    Inkscape::UI::Widget::Ruler *_vruler;
    Gtk::Allocation _allocation;

    unsigned int _interaction_disabled_counter;

    Geom::Point _ruler_origin;
    double _dt2r;

    SPCanvas *_canvas;

public:
    Inkscape::UI::Widget::LayerSelector *layer_selector;

    EgeColorProfTracker* _tracker;

    struct WidgetStub : public Inkscape::UI::View::EditWidgetInterface {
        SPDesktopWidget *_dtw;
        WidgetStub (SPDesktopWidget* dtw) : _dtw(dtw) {}

        void setTitle (gchar const *uri) override
        { _dtw->updateTitle (uri); }
        Gtk::Window* getWindow() override
        { return _dtw->window; }

        void layout() override {
            _dtw->layoutWidgets();
        }

        void present() override
        { _dtw->presentWindow(); }
        void getGeometry (gint &x, gint &y, gint &w, gint &h) override
        { _dtw->getWindowGeometry (x, y, w, h); }
        void setSize (gint w, gint h) override
        { _dtw->setWindowSize (w, h); }
        void setPosition (Geom::Point p) override
        { _dtw->setWindowPosition (p); }
        void setTransient (void* p, int transient_policy) override
        { _dtw->setWindowTransient (p, transient_policy); }
        Geom::Point getPointer() override
        { return _dtw->window_get_pointer(); }
        void setIconified() override
        { _dtw->iconify(); }
        void setMaximized() override
        { _dtw->maximize(); }
        void setFullscreen() override
        { _dtw->fullscreen(); }
        bool shutdown() override
        { return _dtw->shutdown(); }
        void destroy() override
        {
            if(_dtw->window != nullptr)
                delete _dtw->window;
            _dtw->window = nullptr;
        }

        void storeDesktopPosition() override { _dtw->storeDesktopPosition(); }
        void requestCanvasUpdate() override { _dtw->requestCanvasUpdate(); }
        void requestCanvasUpdateAndWait() override { _dtw->requestCanvasUpdateAndWait(); }
        void enableInteraction() override { _dtw->enableInteraction(); }
        void disableInteraction() override { _dtw->disableInteraction(); }
        void activateDesktop() override { sp_dtw_desktop_activate(_dtw); }
        void deactivateDesktop() override { sp_dtw_desktop_deactivate(_dtw); }
        void updateRulers() override { _dtw->update_rulers(); }
        void updateScrollbars(double scale) override { _dtw->update_scrollbars(scale); }
        void toggleRulers() override { _dtw->toggle_rulers(); }
        void toggleScrollbars() override { _dtw->toggle_scrollbars(); }
        void toggleColorProfAdjust() override { _dtw->toggle_color_prof_adj(); }
        bool colorProfAdjustEnabled() override { return _dtw->get_color_prof_adj_enabled(); }
        void updateZoom() override { _dtw->update_zoom(); }
        void letZoomGrabFocus() override { _dtw->letZoomGrabFocus(); }
        void updateRotation() override { _dtw->update_rotation(); }
        Gtk::Toolbar* get_toolbar_by_name(const Glib::ustring& name) override {return _dtw->get_toolbar_by_name(name);}
        void setToolboxFocusTo(const gchar *id) override { _dtw->setToolboxFocusTo(id); }
        void setToolboxAdjustmentValue(const gchar *id, double val) override
        { _dtw->setToolboxAdjustmentValue (id, val); }
        bool isToolboxButtonActive (gchar const* id) override
        { return _dtw->isToolboxButtonActive (id); }
        void setCoordinateStatus (Geom::Point p) override
        { _dtw->setCoordinateStatus (p); }
        void setMessage (Inkscape::MessageType type, gchar const* msg) override
        { _dtw->setMessage (type, msg); }

        bool showInfoDialog( Glib::ustring const &message ) override {
            return _dtw->showInfoDialog( message );
        }

        bool warnDialog (Glib::ustring const &text) override
        { return _dtw->warnDialog (text); }

        Inkscape::UI::Widget::Dock* getDock () override
        { return _dtw->getDock(); }
    };

    WidgetStub *stub;

    void setMessage(Inkscape::MessageType type, gchar const *message);
    Geom::Point window_get_pointer();
    bool shutdown();
    void viewSetPosition (Geom::Point p);
    void letZoomGrabFocus();
    void getWindowGeometry (gint &x, gint &y, gint &w, gint &h);
    void setWindowPosition (Geom::Point p);
    void setWindowSize (gint w, gint h);
    void setWindowTransient (void *p, int transient_policy);
    void presentWindow();
    bool showInfoDialog( Glib::ustring const &message );
    bool warnDialog (Glib::ustring const &text);
    Gtk::Toolbar* get_toolbar_by_name(const Glib::ustring& name);
    void setToolboxFocusTo (gchar const *);
    void setToolboxAdjustmentValue (gchar const * id, double value);
    bool isToolboxButtonActive (gchar const *id);
    void setToolboxPosition(Glib::ustring const& id, GtkPositionType pos);
    void setCoordinateStatus(Geom::Point p);
    void storeDesktopPosition();
    void requestCanvasUpdate();
    void requestCanvasUpdateAndWait();
    void enableInteraction();
    void disableInteraction();
    void updateTitle(gchar const *uri);
    bool onFocusInEvent(GdkEventFocus*);

    Gtk::MenuBar *menubar() { return _menubar; }

    Inkscape::UI::Widget::Dock* getDock();

    static GType getType();
    static SPDesktopWidget* createInstance(SPDocument *document);

    void updateNamedview();
    void update_guides_lock();

    /// Get the CMS adjustment button widget
    decltype(_cms_adjust) get_cms_adjust() const {return _cms_adjust;}

    void cms_adjust_set_sensitive(bool enabled);
    bool get_color_prof_adj_enabled() const;
    void toggle_color_prof_adj();
    bool get_sticky_zoom_active() const;
    void update_zoom();
    void update_rotation();
    void update_rulers();
    double get_hruler_thickness() const;
    double get_vruler_thickness() const;
    GtkAllocation get_canvas_allocation() const;
    void iconify();
    void maximize();
    void fullscreen();
    static gint ruler_event(GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw, bool horiz);

    private:
    GtkWidget *tool_toolbox;
    GtkWidget *aux_toolbox;
    GtkWidget *commands_toolbox;
    GtkWidget *snap_toolbox;

    static void init(SPDesktopWidget *widget);
    void layoutWidgets();

    void namedviewModified(SPObject *obj, guint flags);
    void on_adjustment_value_changed();
    void toggle_scrollbars();
    void update_scrollbars(double scale);
    void toggle_rulers();
    void sticky_zoom_toggled();
    int zoom_input(double *new_val);
    bool zoom_output();
    void zoom_value_changed();
    void zoom_menu_handler(double factor);
    void zoom_populate_popup(Gtk::Menu *menu);
    int rotation_input(double *new_val);
    bool rotation_output();
    void rotation_value_changed();
    void rotation_populate_popup(Gtk::Menu *menu);
    void canvas_tbl_size_allocate(Gtk::Allocation &allocation);

#if defined(HAVE_LIBLCMS2)
    static void cms_adjust_toggled( GtkWidget *button, gpointer data );
    static void color_profile_event(EgeColorProfTracker *tracker, SPDesktopWidget *dtw);
#endif
    static void ruler_snap_new_guide(SPDesktop *desktop, SPCanvasItem *guide, Geom::Point &event_dt, Geom::Point &normal);
    static gint event(GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw);
    bool on_ruler_box_button_press_event(GdkEventButton *event, Gtk::EventBox *widget, bool horiz);
    bool on_ruler_box_button_release_event(GdkEventButton *event, Gtk::EventBox *widget, bool horiz);
    bool on_ruler_box_motion_notify_event(GdkEventMotion *event, Gtk::EventBox *widget, bool horiz);
};

/// The SPDesktopWidget vtable
struct SPDesktopWidgetClass {
    SPViewWidgetClass parent_class;
};

#endif /* !SEEN_SP_DESKTOP_WIDGET_H */

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
