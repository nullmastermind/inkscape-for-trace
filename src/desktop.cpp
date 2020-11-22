// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Editable view implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *   John Bintz <jcoswell@coswellproductions.org>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2007 Jon A. Cruz
 * Copyright (C) 2006-2008 Johan Engelen
 * Copyright (C) 2006 John Bintz
 * Copyright (C) 2004 MenTaLguY
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>
#include <2geom/transforms.h>
#include <2geom/rect.h>
#include <memory>

#include "desktop.h"

#include "color.h"
#include "desktop-events.h"
#include "desktop-style.h"
#include "device-manager.h"
#include "document-undo.h"
#include "event-log.h"
#include "inkscape-window.h"
#include "layer-fns.h"
#include "layer-manager.h"
#include "message-context.h"
#include "message-stack.h"

#include "display/drawing.h"
#include "display/control/canvas-temporary-item-list.h"
#include "display/control/canvas-grid.h" // Grid types
#include "display/control/snap-indicator.h"

#include "display/control/canvas-item-catchall.h"
#include "display/control/canvas-item-drawing.h"
#include "display/control/canvas-item-group.h"
#include "display/control/canvas-item-rect.h"
#include "display/control/canvas-item-rotate.h"

#include "helper/action-context.h"
#include "helper/action.h" //sp_action_perform

#include "io/resource-manager.h"

#include "object/sp-namedview.h"
#include "object/sp-root.h"

#include "ui/desktop/menubar.h"
#include "ui/dialog/dialog-container.h"
#include "ui/interface.h" // Only for getLayoutPrefPath
#include "ui/tool-factory.h"
#include "ui/tools/tool-base.h"
#include "ui/tools/box3d-tool.h"
#include "ui/tools/select-tool.h"
#include "ui/widget/canvas.h"

#include "widgets/desktop-widget.h"

// TODO those includes are only for node tool quick zoom. Remove them after fixing it.
#include "ui/tools/node-tool.h"
#include "ui/tool/control-point-selection.h"

namespace Inkscape { namespace XML { class Node; }}

// Callback declarations
static void _onSelectionChanged (Inkscape::Selection *selection, SPDesktop *desktop);
static bool _drawing_handler (GdkEvent *event, Inkscape::DrawingItem *item, SPDesktop *desktop);
static void _layer_activated(SPObject *layer, SPDesktop *desktop);
static void _layer_deactivated(SPObject *layer, SPDesktop *desktop);
static void _layer_hierarchy_changed(SPObject *top, SPObject *bottom, SPDesktop *desktop);
static void _reconstruction_start(SPDesktop * desktop);
static void _reconstruction_finish(SPDesktop * desktop);
static void _namedview_modified (SPObject *obj, guint flags, SPDesktop *desktop);

static gdouble _pinch_begin_zoom = 1.;

static void _pinch_begin_handler(GtkGesture *gesture, GdkEventSequence *sequence, SPDesktop *desktop)
{
    _pinch_begin_zoom = desktop->current_zoom();
}

static void _pinch_scale_changed_handler(GtkGesture *gesture, gdouble delta, SPDesktop *desktop)
{
    GdkEventSequence *sequence = gtk_gesture_get_last_updated_sequence(gesture);
    const GdkEvent *event = gtk_gesture_get_last_event(gesture, sequence);

    Geom::Point button_window(event->button.x, event->button.y);
    Geom::Point button_world = desktop->getCanvas()->canvas_to_world(button_window);
    Geom::Point button_dt(desktop->w2d(button_world));

    desktop->zoom_absolute(button_dt, _pinch_begin_zoom * delta);
}

SPDesktop::SPDesktop()
    : namedview(nullptr)
    , canvas(nullptr)
    , layers(nullptr)
    , selection(nullptr)
    , event_context(nullptr)
    , layer_manager(nullptr)
    , event_log(nullptr)
    , temporary_item_list(nullptr)
    , snapindicator(nullptr)
    , current(nullptr)  // current style
    , _focusMode(false)
    , dkey(0)
    , number(0)
    , window_state(0)
    , interaction_disabled_counter(0)
    , waiting_cursor(false)
    , showing_dialogs(false)
    , guides_active(false)
    , gr_item(nullptr)
    , gr_point_type(POINT_LG_BEGIN)
    , gr_point_i(0)
    , gr_fill_or_stroke(Inkscape::FOR_FILL)
    , _reconstruction_old_layer_id()
    // an id attribute is not allowed to be the empty string
    , _widget(nullptr) // DesktopWidget
    , _guides_message_context(nullptr)
    , _active(false)
    , _image_render_observer(this, "/options/rendering/imageinoutlinemode")
    , grids_visible(false)
{
    layers = new Inkscape::LayerModel();
    layers->_layer_activated_signal.connect(sigc::bind(sigc::ptr_fun(_layer_activated), this));
    layers->_layer_deactivated_signal.connect(sigc::bind(sigc::ptr_fun(_layer_deactivated), this));
    layers->_layer_changed_signal.connect(sigc::bind(sigc::ptr_fun(_layer_hierarchy_changed), this));
    selection = Inkscape::GC::release( new Inkscape::Selection(layers, this) );
}

void
SPDesktop::init (SPNamedView *nv, Inkscape::UI::Widget::Canvas *acanvas, SPDesktopWidget *widget)
{
    namedview = nv;
    canvas = acanvas;
    _widget = widget;

    // Temporary workaround for link order issues:
    Inkscape::DeviceManager::getManager().getDevices();
    Inkscape::ResourceManager::getManager();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    _guides_message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(messageStack()));

    current = prefs->getStyle("/desktop/style");

    SPDocument *document = namedview->document;
    /* XXX:
     * ensureUpToDate() sends a 'modified' signal to the root element.
     * This is reportedly required to prevent flickering after the document
     * loads. However, many SPObjects write to their repr in response
     * to this signal. This is apparently done to support live path effects,
     * which rewrite their result paths after each modification of the base object.
     * This causes the generation of an incomplete undo transaction,
     * which causes problems down the line, including crashes in the
     * Undo History dialog.
     *
     * For now, this is handled by disabling undo tracking during this call.
     * A proper fix would involve modifying the way ensureUpToDate() works,
     * so that the LPE results are not rewritten.
     */
    Inkscape::DocumentUndo::setUndoSensitive(document, false);
    document->ensureUpToDate();
    Inkscape::DocumentUndo::setUndoSensitive(document, true);

    dkey = SPItem::display_key_new(1);

    /* Connect display key to layer model */
    layers->setDisplayKey(dkey);

    /* Connect document */
    setDocument (document);

    number = namedview->getViewCount();

    /* Setup Canvas */
    canvas->set_background_color(0xffffff00); // Background page sits on.

    /* ----------- Canvas Items ------------ */

    /* CanvasItem's: Controls/Grids/etc. Canvas items are owned by the canvas through
     * canvas_item_root. Canvas items are automatically added and removed from the tree when
     * created and deleted (as long as a canvas item group is passed in the constructor).
     * It would probably make sense to move most of this code to the Canvas.
     */

    Inkscape::CanvasItemGroup *canvas_item_root = canvas->get_canvas_item_root();

    // The order in which these canvas items are added determines the z-order. It's therefore
    // important to add the tempgroup (which will contain the snapindicator) before adding the
    // controls. Only this way one will be able to quickly (before the snap indicator has
    // disappeared) reselect a node after snapping it. If the z-order is wrong however, this
    // will not work (the snap indicator is on top of the node handler; is the snapindicator
    // being selected? or does it intercept some of the events that should have gone to the
    // node handler? see bug https://bugs.launchpad.net/inkscape/+bug/414142)

    canvas_catchall       = new Inkscape::CanvasItemCatchall(canvas_item_root); // Lowest item!
    canvas_group_drawing  = new Inkscape::CanvasItemGroup(canvas_item_root);
    canvas_group_grids    = new Inkscape::CanvasItemGroup(canvas_item_root);
    canvas_group_guides   = new Inkscape::CanvasItemGroup(canvas_item_root);
    canvas_group_sketch   = new Inkscape::CanvasItemGroup(canvas_item_root);
    canvas_group_temp     = new Inkscape::CanvasItemGroup(canvas_item_root);
    canvas_group_controls = new Inkscape::CanvasItemGroup(canvas_item_root);

    canvas_group_drawing->set_name( "CanvasItemGroup:Drawing" ); // The actual SVG drawing.
    canvas_group_grids->set_name(   "CanvasItemGroup:Grids"   ); // Grids.
    canvas_group_guides->set_name(  "CanvasItemGroup:Guides"  ); // Guides.
    canvas_group_sketch->set_name(  "CanvasItemGroup:Sketch"  ); // Temporary items before becoming permanent.
    canvas_group_temp->set_name(    "CanvasItemGroup:Temp"    ); // Temporary items that disappear by themselves.
    canvas_group_controls->set_name("CanvasItemGroup:Controls"); // Controls (handles, knots, rectangles, etc.).

    canvas_group_sketch->set_pickable(false);  // Temporary items are not pickable!
    canvas_group_temp->set_pickable(false);    // Temporary items are not pickable!

    // The root should never emit events. The "catchall" should get it! (CHECK)
    canvas_item_root->connect_event(sigc::bind(sigc::ptr_fun(sp_desktop_root_handler), this));
    canvas_catchall->connect_event(sigc::bind(sigc::ptr_fun(sp_desktop_root_handler), this));

    // Drawing
    Geom::Rect const d(Geom::Point(0.0, 0.0),
                       Geom::Point(document->getWidth().value("px"), document->getHeight().value("px")));

    canvas_page = new Inkscape::CanvasItemRect(canvas_group_drawing, d);
    canvas_page->set_name( "CanvasItemRect:Page" );
    canvas_page->set_stroke(0x00000000);

    canvas_shadow = new Inkscape::CanvasItemRect(canvas_group_drawing, d);
    canvas_shadow->set_name( "CanvasItemRect:Shadow" );
    if ( namedview->pageshadow != 0 && namedview->showpageshadow ) {
        canvas_shadow->set_shadow(0x3f3f3fff, namedview->pageshadow);
    }

    canvas_drawing = new Inkscape::CanvasItemDrawing(canvas_group_drawing);
    canvas_drawing->get_drawing()->delta = prefs->getDouble("/options/cursortolerance/value", 1.0);
    canvas_drawing->connect_drawing_event(sigc::bind(sigc::ptr_fun(_drawing_handler), this));
    canvas->set_drawing(canvas_drawing->get_drawing()); // Canvas needs access.

    Inkscape::DrawingItem *drawing_item = document->getRoot()->invoke_show(
        *(canvas_drawing->get_drawing()),
        dkey,
        SP_ITEM_SHOW_DISPLAY);
    if (drawing_item) {
        canvas_drawing->get_drawing()->root()->prependChild(drawing_item);
    }

    // Must be the top most item.
    canvas_rotate = new Inkscape::CanvasItemRotate(canvas_item_root);
    canvas_rotate->hide();

    temporary_item_list = new Inkscape::Display::TemporaryItemList( this );
    snapindicator = new Inkscape::Display::SnapIndicator ( this );

    /* --------- End Canvas Items ----------- */

    /* Connect event for page resize */
    _modified_connection =
        namedview->connectModified(sigc::bind<2>(sigc::ptr_fun(&_namedview_modified), this));

    namedview->show(this);
    /* Ugly hack */
    activate_guides (true);
    /* Ugly hack */
    _namedview_modified (namedview, SP_OBJECT_MODIFIED_FLAG, this);

    // Set the select tool as the active tool.
    setEventContext("/tools/select");

    // display rect and zoom are now handled in sp_desktop_widget_realize()

    // pinch zoom
    zoomgesture = gtk_gesture_zoom_new(GTK_WIDGET(canvas->gobj()));
    gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (zoomgesture), GTK_PHASE_CAPTURE);
    g_signal_connect(zoomgesture, "begin", G_CALLBACK(_pinch_begin_handler), this);
    g_signal_connect(zoomgesture, "scale-changed", G_CALLBACK(_pinch_scale_changed_handler), this);

/* Set up notification of rebuilding the document, this allows
       for saving object related settings in the document. */
    _reconstruction_start_connection =
        document->connectReconstructionStart(sigc::bind(sigc::ptr_fun(_reconstruction_start), this));
    _reconstruction_finish_connection =
        document->connectReconstructionFinish(sigc::bind(sigc::ptr_fun(_reconstruction_finish), this));
    _reconstruction_old_layer_id.clear();

    // ?
    // sp_active_desktop_set (desktop);

    _activate_connection = _activate_signal.connect(
        sigc::bind(
            sigc::ptr_fun(_onActivate),
            this
        )
    );
     _deactivate_connection = _deactivate_signal.connect(
        sigc::bind(
            sigc::ptr_fun(_onDeactivate),
            this
        )
    );

    _sel_modified_connection = selection->connectModified(
        sigc::bind(
            sigc::ptr_fun(&_onSelectionModified),
            this
        )
    );
    _sel_changed_connection = selection->connectChanged(
        sigc::bind(
            sigc::ptr_fun(&_onSelectionChanged),
            this
        )
    );


    /* setup LayerManager */
    //   (Setting up after the connections are all in place, as it may use some of them)
    layer_manager = new Inkscape::LayerManager( this );

    showGrids(namedview->grids_visible, false);
}

void SPDesktop::destroy()
{
    _destroy_signal.emit(this);

    canvas->set_desktop(nullptr);
    canvas->set_drawing(nullptr);

    if (snapindicator) {
        delete snapindicator;
        snapindicator = nullptr;
    }

    if (temporary_item_list) {
        delete temporary_item_list;
        temporary_item_list = nullptr;
    }

    if (selection) {
        delete selection;
        selection = nullptr;
    }

    namedview->hide(this);

    _activate_connection.disconnect();
    _deactivate_connection.disconnect();
    _sel_modified_connection.disconnect();
    _sel_changed_connection.disconnect();
    _modified_connection.disconnect();
    _commit_connection.disconnect();
    _reconstruction_start_connection.disconnect();
    _reconstruction_finish_connection.disconnect();

    if (zoomgesture) {
        g_signal_handlers_disconnect_by_data(zoomgesture, this);
        g_clear_object(&zoomgesture);
    }

    delete layers;

    if (layer_manager) {
        delete layer_manager;
        layer_manager = nullptr;
    }

    if (canvas_drawing) {
        doc()->getRoot()->invoke_hide(dkey);
        delete canvas_drawing; // Why is canvas_drawing special?
        canvas_drawing = nullptr;
    }

    _guides_message_context = nullptr;
}

SPDesktop::~SPDesktop()
= default;


//--------------------------------------------------------------------
/* Public methods */


/* These methods help for temporarily showing things on-canvas.
 * The *only* valid use of the TemporaryItem* that you get from add_temporary_canvasitem
 * is when you want to prematurely remove the item from the canvas, by calling
 * desktop->remove_temporary_canvasitem(tempitem).
 */
/** Note that lifetime is measured in milliseconds
 * One should *not* keep a reference to the SPCanvasItem, the temporary item code will
 * delete the object for you and the reference will become invalid without you knowing it.
 * It is perfectly safe to ignore the returned pointer: the object is deleted by itself, so don't delete it elsewhere!
 * The *only* valid use of the returned TemporaryItem* is as argument for SPDesktop::remove_temporary_canvasitem,
 * because the object might be deleted already without you knowing it.
 * move_to_bottom = true by default so the item does not interfere with handling of other items on the canvas like nodes.
 */
Inkscape::Display::TemporaryItem *
SPDesktop::add_temporary_canvasitem (Inkscape::CanvasItem *item, guint lifetime, bool move_to_bottom)
{
    if (move_to_bottom) {
        item->lower_to_bottom();
    }

    return temporary_item_list->add_item(item, lifetime);
}

/** It is perfectly safe to call this function while the object has already been deleted due to a timeout.
*/
void
SPDesktop::remove_temporary_canvasitem (Inkscape::Display::TemporaryItem * tempitem)
{
    // check for non-null temporary_item_list, because during destruction of desktop, some destructor might try to access this list!
    if (tempitem && temporary_item_list) {
        temporary_item_list->delete_item(tempitem);
    }
}

void SPDesktop::redrawDesktop() {
    canvas->set_affine(_current_affine.d2w()); // For CanvasItem's.
}

// Pass-through LayerModel functions
SPObject *SPDesktop::currentRoot() const
{
    return layers->currentRoot();
}

SPObject *SPDesktop::currentLayer() const
{
    return layers->currentLayer();
}

void SPDesktop::setCurrentLayer(SPObject *object)
{
    layers->setCurrentLayer(object);
}

void SPDesktop::toggleLayerSolo(SPObject *object)
{
    layers->toggleLayerSolo(object);
}

void SPDesktop::toggleHideAllLayers(bool hide)
{
    layers->toggleHideAllLayers(hide);
}

void SPDesktop::toggleLockAllLayers(bool lock)
{
    layers->toggleLockAllLayers(lock);
}

void SPDesktop::toggleLockOtherLayers(SPObject *object)
{
    layers->toggleLockOtherLayers(object);
}

bool SPDesktop::isLayer(SPObject *object) const
{
    return layers->isLayer(object);
}

/**
 * True if desktop viewport intersects \a item's bbox.
 */
bool SPDesktop::isWithinViewport (SPItem *item) const
{
    auto const viewport = get_display_area();
    Geom::OptRect const bbox = item->desktopVisualBounds();
    if (bbox) {
        return viewport.intersects(*bbox);
    } else {
        return false;
    }
}

///
bool SPDesktop::itemIsHidden(SPItem const *item) const {
    return item->isHidden(this->dkey);
}

/**
 * Set activate property of desktop; emit signal if changed.
 */
void
SPDesktop::set_active (bool new_active)
{
    if (new_active != _active) {
        _active = new_active;
        if (new_active) {
            _activate_signal.emit();
        } else {
            _deactivate_signal.emit();
        }
    }
}

/**
 * Set activate status of current desktop's named view.
 */
void
SPDesktop::activate_guides(bool activate)
{
    guides_active = activate;
    namedview->activateGuides(this, activate);
}

/**
 * Make desktop switch documents.
 */
void
SPDesktop::change_document (SPDocument *theDocument)
{
    g_return_if_fail (theDocument != nullptr);

    /* unselect everything before switching documents */
    selection->clear();

    setDocument (theDocument);

    /* update the rulers, connect the desktop widget's signal to the new namedview etc.
       (this can probably be done in a better way) */
    InkscapeWindow *parent = this->getInkscapeWindow();
    g_assert(parent != nullptr);
    parent->change_document(theDocument);
    SPDesktopWidget *dtw = parent->get_desktop_widget();
    if (dtw) {
        dtw->desktop = this;
        dtw->updateNamedview();
    } else {
        std::cerr << "SPDesktop::change_document: failed to get desktop widget!" << std::endl;
    }

    _namedview_modified (namedview, SP_OBJECT_MODIFIED_FLAG, this);
    _document_replaced_signal.emit (this, theDocument);
}

/**
 * Replaces the currently active tool with a new one. Pass the empty string to
 * unset and free the current tool.
 */
void SPDesktop::setEventContext(const std::string& toolName)
{
    // Tool should be able to be replaced with itself. See commit 29df5ca05d
    if (event_context) {
        event_context->finish();
        delete event_context;
    }

    if (toolName.empty()) {
        event_context = nullptr;
    } else {
        event_context = ToolFactory::createObject(toolName);
        event_context->setDesktop(this);
        event_context->message_context = std::unique_ptr<Inkscape::MessageContext>(new Inkscape::MessageContext(this->messageStack()));
        event_context->setup();

        // Make sure no delayed snapping events are carried over after switching tools
        // (this is only an additional safety measure against sloppy coding, because each
        // tool should take care of this by itself)
        sp_event_context_discard_delayed_snap_event(event_context);
    }

    _event_context_changed_signal.emit(this, event_context);
}

/**
 * Sets the coordinate status to a given point
 */
void
SPDesktop::set_coordinate_status (Geom::Point p) {
    _widget->setCoordinateStatus(p);
}

Inkscape::UI::Dialog::DialogContainer *SPDesktop::getContainer()
{
    return _widget->getContainer();
}

/**
 * \see SPDocument::getItemFromListAtPointBottom()
 */
SPItem *SPDesktop::getItemFromListAtPointBottom(const std::vector<SPItem*> &list, Geom::Point const &p) const
{
    g_return_val_if_fail (doc() != nullptr, NULL);
    return SPDocument::getItemFromListAtPointBottom(dkey, doc()->getRoot(), list, p);
}

/**
 * \see SPDocument::getItemAtPoint()
 */
SPItem *SPDesktop::getItemAtPoint(Geom::Point const &p, bool into_groups, SPItem *upto) const
{
    g_return_val_if_fail (doc() != nullptr, NULL);
    return doc()->getItemAtPoint( dkey, p, into_groups, upto);
}

/**
 * \see SPDocument::getGroupAtPoint()
 */
SPItem *SPDesktop::getGroupAtPoint(Geom::Point const &p) const
{
    g_return_val_if_fail (doc() != nullptr, NULL);
    return doc()->getGroupAtPoint(dkey, p);
}

/**
 * Returns the mouse point in document coordinates; if mouse is
 * outside the canvas, returns the center of canvas viewpoint.
 */
Geom::Point SPDesktop::point(bool outside_canvas) const
{
    Geom::Point p = _widget->window_get_pointer();
    Geom::Point pw = canvas->canvas_to_world(p);
    Geom::Rect const r = canvas->get_area_world();

    if (r.interiorContains(pw) || outside_canvas) {
        p = w2d(pw);
        return p;
    }
    Geom::Point r0 = w2d(r.min());
    Geom::Point r1 = w2d(r.max());
    return (r0 + r1) / 2.0;
}


/**
 * Revert back to previous transform if possible. Note: current transform is
 * always at front of stack.
 */
void
SPDesktop::prev_transform()
{
    if (transforms_past.empty()) {
        std::cerr << "SPDesktop::prev_transform: current transform missing!" << std::endl;
        return;
    }

    if (transforms_past.size() == 1) {
        messageStack()->flash(Inkscape::WARNING_MESSAGE, _("No previous transform."));
        return;
    }

    // Push current transform into future transforms list.
    transforms_future.push_front( _current_affine );

    // Remove the current transform from the past transforms list.
    transforms_past.pop_front();

    // restore previous transform
    _current_affine = transforms_past.front();
    set_display_area (false);

}


/**
 * Set transform to next in list.
 */
void SPDesktop::next_transform()
{
    if (transforms_future.empty()) {
        this->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("No next transform."));
        return;
    }

    // restore next transform
    _current_affine = transforms_future.front();
    set_display_area (false);

    // remove the just-used transform from the future transforms list
    transforms_future.pop_front();

    // push current transform into past transforms list
    transforms_past.push_front( _current_affine );
}


/**
 * Clear transform lists.
 */
void
SPDesktop::clear_transform_history()
{
    transforms_past.clear();
    transforms_future.clear();
}


/**
 * Does all the dirty work in setting the display area.
 * _current_affine must already be full updated (including offset).
 * log: if true, save transform in transform stack for reuse.
 */
void
SPDesktop::set_display_area (bool log)
{
    // Save the transform
    if (log) {
        transforms_past.push_front( _current_affine );
        // if we do a logged transform, our transform-forward list is invalidated, so delete it
        transforms_future.clear();
    }

    redrawDesktop();

    // Scroll
    Geom::Point offset = _current_affine.getOffset();
    canvas->scroll_to(offset, true);
    canvas->set_affine(_current_affine.d2w()); // For CanvasItem's.
    // To do: if transform unchanged call with 'false' (redraw only newly exposed areas).

    /* Update perspective lines if we are in the 3D box tool (so that infinite ones are shown
     * correctly) */
    if (SP_IS_BOX3D_CONTEXT(event_context)) {
    	SP_BOX3D_CONTEXT(event_context)->_vpdrag->updateLines();
    }

    // Update GUI (TODO: should be handled by CanvasGrid).
    _widget->update_rulers();
    _widget->update_scrollbars(_current_affine.getZoom());
    _widget->update_zoom();
    _widget->update_rotation();

    signal_zoom_changed.emit(_current_affine.getZoom());  // Observed by path-manipulator to update arrows.
}


/**
 * Map the drawing to the window so that 'c' lies at 'w' where where 'c'
 * is a point on the canvas and 'w' is position in window in screen pixels.
 */
void
SPDesktop::set_display_area (Geom::Point const &c, Geom::Point const &w, bool log)
{
    // The relative offset needed to keep c at w.
    Geom::Point offset = d2w(c) - w;
    _current_affine.addOffset( offset );
    set_display_area( log );
}


/**
 * Map the center of rectangle 'r' (which specifies a non-rotated region of the
 * drawing) to lie at the center of the window. The zoom factor is calculated such that
 * the edges of 'r' closest to 'w' are 'border' length inside of the window (if
 * there is no rotation). 'r' is in document pixel units, 'border' is in screen pixels.
 */
void
SPDesktop::set_display_area( Geom::Rect const &r, double border, bool log)
{
    // Create a rectangle the size of the window aligned with origin.
    Geom::Rect w( Geom::Point(), canvas->get_area_world().dimensions() );

    // Shrink window to account for border padding.
    w.expandBy( -border );

    double zoom = 1.0;
    // Determine which direction limits scale:
    //   if (r.width/w.width > r.height/w.height) then zoom using width.
    //   Avoiding division in test:
    if ( r.width()*w.height() > r.height()*w.width() ) {
        zoom = w.width() / r.width();
    } else {
        zoom = w.height() / r.height();
    }
    zoom = CLAMP(zoom, SP_DESKTOP_ZOOM_MIN, SP_DESKTOP_ZOOM_MAX);
    _current_affine.setScale( Geom::Scale(zoom, yaxisdir() * zoom) );
    // Zero offset, actual offset calculated later.
    _current_affine.setOffset( Geom::Point( 0, 0 ) );

    set_display_area( r.midpoint(), w.midpoint(), log );
}


/**
 * Return canvas viewbox in desktop coordinates
 */
Geom::Parallelogram SPDesktop::get_display_area(bool use_integer_viewbox) const
{
    // viewbox in world coordinates
    Geom::Rect const viewbox = use_integer_viewbox ? canvas->get_area_world_int() : canvas->get_area_world();

    // display area in desktop coordinates
    return Geom::Parallelogram(viewbox) * w2d();
}

/**
 * Zoom to the given absolute zoom level
 *
 * @param center - Point we want to zoom in on
 * @param zoom - Absolute amount of zoom (1.0 is 100%)
 * @param keep_point - Keep center fixed in the desktop window.
 */
void
SPDesktop::zoom_absolute(Geom::Point const &center, double zoom, bool keep_point)
{
    Geom::Point w = d2w(center); // Must be before zoom changed.
    if(!keep_point) {
        w = canvas->get_area_world().midpoint();
    }
    zoom = CLAMP (zoom, SP_DESKTOP_ZOOM_MIN, SP_DESKTOP_ZOOM_MAX);
    _current_affine.setScale( Geom::Scale(zoom, yaxisdir() * zoom) );
    set_display_area( center, w );
}

/**
 * Zoom in or out relatively to the current zoom
 *
 * @param center - Point we want to zoom in on
 * @param zoom - Relative amount of zoom. at 50% + 50% -> 25% zoom
 * @param keep_point - Keep center fixed in the desktop window.
 */
void
SPDesktop::zoom_relative(Geom::Point const &center, double zoom, bool keep_point)
{
    double new_zoom = _current_affine.getZoom() * zoom;
    this->zoom_absolute(center, new_zoom, keep_point);
}

/**
 * Zoom in to an absolute realworld ratio, e.g. 1:1 physical screen units
 *
 * @param center - Point we want to zoom in on.
 * @param ratio - Absolute physical zoom ratio.
 */
void
SPDesktop::zoom_realworld(Geom::Point const &center, double ratio)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double correction = prefs->getDouble("/options/zoomcorrection/value", 1.0);
    this->zoom_absolute(center, ratio * correction, false);
}


/**
 * Set display area to origin and current document dimensions.
 */
void
SPDesktop::zoom_page()
{
    Geom::Rect d(Geom::Point(0, 0),
                 Geom::Point(doc()->getWidth().value("px"), doc()->getHeight().value("px")));

    if (d.minExtent() < 1.0) {
        return;
    }

    set_display_area(d, 10);
}

/**
 * Set display area to current document width.
 */
void
SPDesktop::zoom_page_width()
{
    if (doc()->getWidth().value("px") < 1.0) {
        return;
    }

    auto const center_y = current_center().y();

    Geom::Rect d(Geom::Point(0, center_y), //
                 Geom::Point(doc()->getWidth().value("px"), center_y));

    set_display_area(d, 10);
}


/**
 * Zoom to whole drawing.
 */
void
SPDesktop::zoom_drawing()
{
    g_return_if_fail (doc() != nullptr);
    SPItem *docitem = doc()->getRoot();
    g_return_if_fail (docitem != nullptr);

    docitem->bbox_valid = FALSE;
    Geom::OptRect d = docitem->desktopVisualBounds();

    /* Note that the second condition here indicates that
    ** there are no items in the drawing.
    */
    if ( !d || d->minExtent() < 0.1 ) {
        return;
    }

    set_display_area(*d, 10);
}


/**
 * Zoom to selection.
 */
void
SPDesktop::zoom_selection()
{
    Geom::OptRect const d = selection->visualBounds();

    if ( !d || d->minExtent() < 0.1 ) {
        return;
    }

    set_display_area(*d, 10);
}

/**
 * Centre Page in window, without zooming
 */
void SPDesktop::zoom_center_page()
{
    zoom_absolute(Geom::Point(doc()->getWidth().value("px")/2, doc()->getHeight().value("px")/2), this->current_zoom(), false);
}

Geom::Point SPDesktop::current_center() const {
    return canvas->get_area_world().midpoint() * _current_affine.w2d();
}

/**
 * Performs a quick zoom into what the user is working on.
 *
 * @param  enable  Whether we're going in or out of quick zoom.
 */
void SPDesktop::zoom_quick(bool enable)
{
    if (enable == _quick_zoom_enabled) {
        return;
    }

    if (enable) {
        _quick_zoom_affine = _current_affine;
        bool zoomed = false;

        // TODO This needs to migrate into the node tool, but currently the design
        // of this method is sufficiently wrong to prevent this.
        if (!zoomed && INK_IS_NODE_TOOL(event_context)) {
            Inkscape::UI::Tools::NodeTool *nt = static_cast<Inkscape::UI::Tools::NodeTool*>(event_context);
            if (!nt->_selected_nodes->empty()) {
                Geom::Rect nodes = *nt->_selected_nodes->bounds();
                double area = nodes.area();
                // do not zoom if a single cusp node is selected aand the bounds
                // have zero area.
                if (!Geom::are_near(area, 0)) {
                    set_display_area(nodes, true);
                    zoomed = true;
                }
            }
        }

        if (!zoomed) {
            Geom::OptRect const d = selection->visualBounds();
            if (d) {
                set_display_area(*d, true);
                zoomed = true;
            }
        }

        if (!zoomed) {
            Geom::Rect const d_canvas = canvas->get_area_world();
            Geom::Point midpoint = w2d(d_canvas.midpoint()); // Midpoint of drawing on canvas.
            zoom_relative(midpoint, 2.0, false);
        }
    } else {
        _current_affine = _quick_zoom_affine;
        set_display_area( false );
    }

    _quick_zoom_enabled = enable;
    return;
}


/**
 * Tell widget to let zoom widget grab keyboard focus.
 */
void
SPDesktop::zoom_grab_focus()
{
    _widget->letZoomGrabFocus();
}


/**
 * Set new rotation, keeping the point 'c' fixed in the desktop window.
 *
 * @param c Point in desktop coordinates
 * @param rotate Angle in clockwise direction
 */
void
SPDesktop::rotate_absolute_keep_point (Geom::Point const &c, double rotate)
{
    Geom::Point w = d2w( c ); // Must be before rotate changed.
    _current_affine.setRotate( rotate );
    set_display_area( c, w );
}


/**
 * Rotate keeping the point 'c' fixed in the desktop window.
 *
 * @param c Point in desktop coordinates
 * @param rotate Angle in clockwise direction
 */
void
SPDesktop::rotate_relative_keep_point (Geom::Point const &c, double rotate)
{
    Geom::Point w = d2w( c ); // Must be before rotate changed.
    _current_affine.addRotate( rotate );
    set_display_area( c, w );
}


/**
 * Set new rotation, aligning the point 'c' to the center of desktop window.
 *
 * @param c Point in desktop coordinates
 * @param rotate Angle in clockwise direction
 */
void
SPDesktop::rotate_absolute_center_point (Geom::Point const &c, double rotate)
{
    _current_affine.setRotate( rotate );
    Geom::Rect viewbox = canvas->get_area_world();
    set_display_area(c, viewbox.midpoint());
}


/**
 * Rotate aligning the point 'c' to the center of desktop window.
 *
 * @param c Point in desktop coordinates
 * @param rotate Angle in clockwise direction
 */
void
SPDesktop::rotate_relative_center_point (Geom::Point const &c, double rotate)
{
    _current_affine.addRotate( rotate );
    Geom::Rect viewbox = canvas->get_area_world();
    set_display_area(c, viewbox.midpoint());
}


/**
 * Set new flip direction, keeping the point 'c' fixed in the desktop window.
 *
 * @param c Point in desktop coordinates
 * @param flip Direction the canvas will be set as.
 */
void
SPDesktop::flip_absolute_keep_point (Geom::Point const &c, CanvasFlip flip)
{
    Geom::Point w = d2w(c); // Must be before flip.
    _current_affine.setFlip(flip);
    set_display_area(c, w);
}


/**
 * Flip direction, keeping the point 'c' fixed in the desktop window.
 *
 * @param c Point in desktop coordinates
 * @param flip Direction to flip canvas
 */
void
SPDesktop::flip_relative_keep_point (Geom::Point const &c, CanvasFlip flip)
{
    Geom::Point w = d2w(c); // Must be before flip.
    _current_affine.addFlip(flip);
    set_display_area(c, w);
}


/**
 * Set new flip direction, aligning the point 'c' to the center of desktop window.
 *
 * @param c Point in desktop coordinates
 * @param flip Direction the canvas will be set as.
 */
void
SPDesktop::flip_absolute_center_point (Geom::Point const &c, CanvasFlip flip)
{
    _current_affine.setFlip(flip);
    Geom::Rect viewbox = canvas->get_area_world();
    set_display_area(c, viewbox.midpoint());
}


/**
 * Flip direction, aligning the point 'c' to the center of desktop window.
 *
 * @param c Point in desktop coordinates
 * @param flip Direction to flip canvas
 */
void
SPDesktop::flip_relative_center_point (Geom::Point const &c, CanvasFlip flip)
{
    _current_affine.addFlip(flip);
    Geom::Rect viewbox = canvas->get_area_world();
    set_display_area(c, viewbox.midpoint());
}

bool
SPDesktop::is_flipped (CanvasFlip flip)
{
    return _current_affine.isFlipped(flip);
}


/**
 * Scroll canvas by to a particular point (window coordinates).
 */
void
SPDesktop::scroll_absolute (Geom::Point const &point, bool is_scrolling)
{
    canvas->scroll_to(point, false);
    _current_affine.setOffset( point );

    /*  update perspective lines if we are in the 3D box tool (so that infinite ones are shown correctly) */
    //sp_box3d_context_update_lines(event_context);
    if (SP_IS_BOX3D_CONTEXT(event_context)) {
		SP_BOX3D_CONTEXT(event_context)->_vpdrag->updateLines();
	}

    _widget->update_rulers();
    _widget->update_scrollbars(_current_affine.getZoom());
}


/**
 * Scroll canvas by specific coordinate amount (window coordinates).
 */
void
SPDesktop::scroll_relative (Geom::Point const &delta, bool is_scrolling)
{
    Geom::Rect const viewbox = canvas->get_area_world();
    scroll_absolute( viewbox.min() - delta, is_scrolling );
}


/**
 * Scroll canvas by specific coordinate amount in svg coordinates.
 */
void
SPDesktop::scroll_relative_in_svg_coords (double dx, double dy, bool is_scrolling)
{
    double scale = _current_affine.getZoom();
    scroll_relative(Geom::Point(dx*scale, dy*scale), is_scrolling);
}


/**
 * Scroll screen so as to keep point 'p' visible in window.
 * (Used, for example, when a node is being dragged.)
 * 'p': The point in desktop coordinates.
 * 'autoscrollspeed': The scroll speed (or zero to use preferences' value).
 */
bool
SPDesktop::scroll_to_point (Geom::Point const &p, gdouble autoscrollspeed)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // autoscrolldistance is in screen pixels.
    gdouble autoscrolldistance = (gdouble) prefs->getIntLimited("/options/autoscrolldistance/value", 0, -1000, 10000);

    Geom::Rect w = canvas->get_area_world(); // Window in screen coordinates.
    w.expandBy(-autoscrolldistance);  // Shrink window

    Geom::Point c = d2w(p);  // Point 'p' in screen coordinates.
    if (!w.contains(c)) {

        Geom::Point c2 = w.clamp(c); // Constrain c to window.

        if (autoscrollspeed == 0)
            autoscrollspeed = prefs->getDoubleLimited("/options/autoscrollspeed/value", 1, 0, 10);

        if (autoscrollspeed != 0)
            scroll_relative (autoscrollspeed * (c2 - c) );

        return true;
    }
    return false;
}

bool
SPDesktop::is_iconified()
{
    return 0!=(window_state & GDK_WINDOW_STATE_ICONIFIED);
}

void
SPDesktop::iconify()
{
    _widget->iconify();
}

bool SPDesktop::is_darktheme() { return getToplevel()->get_style_context()->has_class("dark"); }

bool
SPDesktop::is_maximized()
{
    return 0!=(window_state & GDK_WINDOW_STATE_MAXIMIZED);
}

void
SPDesktop::maximize()
{
    _widget->maximize();
}

bool
SPDesktop::is_fullscreen()
{
    return 0!=(window_state & GDK_WINDOW_STATE_FULLSCREEN);
}

void
SPDesktop::fullscreen()
{
    _widget->fullscreen();
}

/**
 * Checks to see if the user is working in focused mode.
 *
 * @return  the value of \c _focusMode.
 */
bool SPDesktop::is_focusMode()
{
    return _focusMode;
}

/**
 * Changes whether the user is in focus mode or not.
 *
 * @param  mode  Which mode the view should be in.
 */
void SPDesktop::focusMode(bool mode)
{
    if (mode == _focusMode) { return; }

    _focusMode = mode;

    layoutWidget();
    //sp_desktop_widget_layout(SPDesktopWidget);

    return;
}

void
SPDesktop::setWindowTitle()
{
    _widget->updateTitle(doc()->getDocumentName());
}

void
SPDesktop::getWindowGeometry (gint &x, gint &y, gint &w, gint &h)
{
    _widget->getWindowGeometry (x, y, w, h);
}

void
SPDesktop::setWindowPosition (Geom::Point p)
{
    _widget->setWindowPosition (p);
}

void
SPDesktop::setWindowSize (gint w, gint h)
{
    _widget->setWindowSize (w, h);
}

void
SPDesktop::setWindowTransient (void *p, int transient_policy)
{
    _widget->setWindowTransient (p, transient_policy);
}

Gtk::Window*
SPDesktop::getToplevel( )
{
    return _widget->window;
}

InkscapeWindow*
SPDesktop::getInkscapeWindow( )
{
    return dynamic_cast<InkscapeWindow*>(_widget->window);
}

void
SPDesktop::presentWindow()
{
    _widget->presentWindow();
}

bool SPDesktop::showInfoDialog( Glib::ustring const & message )
{
    return _widget->showInfoDialog( message );
}

bool
SPDesktop::warnDialog (Glib::ustring const &text)
{
    return _widget->warnDialog (text);
}

void
SPDesktop::toggleRulers()
{
    _widget->toggle_rulers();
    Inkscape::Verb *verb = Inkscape::Verb::get(SP_VERB_TOGGLE_RULERS);
    if (verb) {
        _menu_update.emit(verb->get_code(), getStateFromPref(this, "rulers"));
    }
}

void
SPDesktop::toggleScrollbars()
{
    _widget->toggle_scrollbars();
    Inkscape::Verb *verb = Inkscape::Verb::get(SP_VERB_TOGGLE_SCROLLBARS);
    if (verb) {
        _menu_update.emit(verb->get_code(), getStateFromPref(this, "scrollbars"));
    }
}


void SPDesktop::toggleToolbar(gchar const *toolbar_name, unsigned int verbenum)
{
    Glib::ustring pref_path = getLayoutPrefPath(this) + toolbar_name + "/state";
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean visible = prefs->getBool(pref_path, true);
    prefs->setBool(pref_path, !visible);
    Inkscape::Verb *verb = Inkscape::Verb::get(verbenum);
    if (verb) {
        _menu_update.emit(verb->get_code(), getStateFromPref(this, toolbar_name));
    }
    layoutWidget();
}

void
SPDesktop::layoutWidget()
{
    _widget->layoutWidgets();
}

void
SPDesktop::destroyWidget()
{
    auto *window = _widget->window;
    _widget->window = nullptr;
    delete window; // may also delete _widget  Check if this is logical!
}

bool
SPDesktop::shutdown()
{
    return _widget->shutdown();
}

bool SPDesktop::onDeleteUI (GdkEventAny*)
{
    if(shutdown())
        return true;

    destroyWidget();
    return false;
}

/**
 *  onWindowStateEvent
 *
 *  Called when the window changes its maximize/fullscreen/iconify/pinned state.
 *  Since GTK doesn't have a way to query this state information directly, we
 *  record it for the desktop here, and also possibly trigger a layout.
 */
bool
SPDesktop::onWindowStateEvent (GdkEventWindowState* event)
{
    // Record the desktop window's state
    window_state = event->new_window_state;

    // Layout may differ depending on full-screen mode or not
    GdkWindowState changed = event->changed_mask;
    if (changed & (GDK_WINDOW_STATE_FULLSCREEN|GDK_WINDOW_STATE_MAXIMIZED)) {
        layoutWidget();
    }

    return false;
}


/**
  * Apply the desktop's current style or the tool style to the object.
  */
void SPDesktop::applyCurrentOrToolStyle(SPObject *obj, Glib::ustring const &tool_path, bool with_text)
{
    SPCSSAttr *css_current = sp_desktop_get_style(this, with_text);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool(tool_path + "/usecurrent") && css_current) {
        obj->setCSS(css_current,"style");
    } else {
        SPCSSAttr *css = prefs->getInheritedStyle(tool_path + "/style");
        obj->setCSS(css,"style");
        sp_repr_css_attr_unref(css);
    }
    if (css_current) {
        sp_repr_css_attr_unref(css_current);
    }
}


void
SPDesktop::setToolboxFocusTo (gchar const *label)
{
    _widget->setToolboxFocusTo (label);
}

void
SPDesktop::setToolboxAdjustmentValue (gchar const* id, double val)
{
    _widget->setToolboxAdjustmentValue (id, val);
}

Gtk::Toolbar*
SPDesktop::get_toolbar_by_name(const Glib::ustring& name)
{
    return _widget->get_toolbar_by_name(name);
}

bool
SPDesktop::isToolboxButtonActive (gchar const *id)
{
    return _widget->isToolboxButtonActive (id);
}

void
SPDesktop::emitToolSubselectionChanged(gpointer data)
{
    _tool_subselection_changed.emit(data);
    INKSCAPE.subselection_changed (this);
}

void SPDesktop::updateNow()
{
    canvas->redraw_now();
}

void
SPDesktop::enableInteraction()
{
  _widget->enableInteraction();
}

void SPDesktop::disableInteraction()
{
  _widget->disableInteraction();
}

void SPDesktop::setWaitingCursor()
{
    Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    Glib::RefPtr<Gdk::Cursor> waiting = Gdk::Cursor::create(display, "wait");
    canvas->get_window()->set_cursor(waiting);
    // GDK needs the flush for the cursor change to take effect
    display->flush();
    waiting_cursor = true;
}

void SPDesktop::clearWaitingCursor() {
  if (waiting_cursor) {
      this->event_context->sp_event_context_update_cursor();
  }
}

void SPDesktop::toggleColorProfAdjust()
{
    _widget->toggle_color_prof_adj();
}

void SPDesktop::toggleGuidesLock()
{
    sp_namedview_guides_toggle_lock(this->getDocument(), namedview);
    Inkscape::Verb *verb = Inkscape::Verb::get(SP_VERB_EDIT_GUIDES_TOGGLE_LOCK);
    if (verb) {
        _menu_update.emit(verb->get_code(), namedview->lockguides);
    }
}

bool SPDesktop::colorProfAdjustEnabled()
{
    return _widget->get_color_prof_adj_enabled();
}

void SPDesktop::toggleGrids()
{
    if (! namedview->grids.empty()) {
            showGrids(!grids_visible);
    } else {
        //there is no grid present at the moment. add a rectangular grid and make it visible
        namedview->writeNewGrid(this->getDocument(), Inkscape::GRID_RECTANGULAR);
        showGrids(true);
    }
    Inkscape::Verb *verb = Inkscape::Verb::get(SP_VERB_TOGGLE_GRID);
    if (verb) {
        _menu_update.emit(verb->get_code(), gridsEnabled());
    }
}

void SPDesktop::showGrids(bool show, bool dirty_document)
{
    grids_visible = show;
    sp_namedview_show_grids(namedview, grids_visible, dirty_document);
    if (show) {
        canvas_group_grids->show();
    } else {
        canvas_group_grids->hide();
    }
}

void SPDesktop::toggleSnapGlobal()
{
    bool v = namedview->getSnapGlobal();
    namedview->setSnapGlobal(!v);
}

//----------------------------------------------------------------------
// Callback implementations. The virtual ones are connected by the view.

void
SPDesktop::onResized (double /*x*/, double /*y*/)
{
   // Nothing called here
}

/**
 * Redraw callback; queues Gtk redraw; connected by View.
 */
void SPDesktop::storeDesktopPosition()
{
    if (_widget) {
        _widget->storeDesktopPosition();
    }
}

/**
 * Redraw callback; queues Gtk redraw; connected by View.
 */
void
SPDesktop::onRedrawRequested ()
{
    if (_widget) {
        _widget->requestCanvasUpdate();
    }
}

void
SPDesktop::updateCanvasNow()
{
  _widget->requestCanvasUpdateAndWait();
}

/**
 * Associate document with desktop.
 */
void
SPDesktop::setDocument (SPDocument *doc)
{
    if (!doc) return;

    if (this->doc()) {
        namedview->hide(this);
        this->doc()->getRoot()->invoke_hide(dkey);
    }

    layers->setDocument(doc);
    selection->setDocument(doc);

    if (event_log) {
        // Remove it from the replaced document. This prevents Inkscape from
        // crashing since we access it in the replaced document's destructor
        // which results in an undefined behavior. (See also: bug #1670688)
        if (this->doc()) {
            this->doc()->removeUndoObserver(*event_log);
        }
        delete event_log;
        event_log = nullptr;
    }

    /* setup EventLog */
    event_log = new Inkscape::EventLog(doc);
    doc->addUndoObserver(*event_log);

    _commit_connection.disconnect();
    _commit_connection = doc->connectCommit(sigc::mem_fun(*this, &SPDesktop::updateNow));

    /// \todo fixme: This condition exists to make sure the code
    /// inside is NOT called on initialization, only on replacement. But there
    /// are surely more safe methods to accomplish this.
    // TODO since the comment had reversed logic, check the intent of this block of code:
    if (canvas_drawing) {

        namedview = sp_document_namedview (doc, nullptr);
        _modified_connection = namedview->connectModified(sigc::bind<2>(sigc::ptr_fun(&_namedview_modified), this));
        number = namedview->getViewCount();

        Inkscape::DrawingItem *drawing_item = doc->getRoot()->invoke_show(
            *(canvas_drawing->get_drawing()),
            dkey,
            SP_ITEM_SHOW_DISPLAY);
        if (drawing_item) {
            canvas_drawing->get_drawing()->root()->prependChild(drawing_item);
        }

        namedview->show(this);
        /* Ugly hack */
        activate_guides (true);
        /* Ugly hack */
        _namedview_modified (namedview, SP_OBJECT_MODIFIED_FLAG, this);
    }


    _document_replaced_signal.emit (this, doc);

    View::setDocument (doc);
}

void
SPDesktop::onStatusMessage
(Inkscape::MessageType type, gchar const *message)
{
    if (_widget) {
        _widget->setMessage(type, message);
    }
}

void
SPDesktop::onDocumentURISet (gchar const* uri)
{
    _widget->updateTitle(uri);
}

/**
 * Resized callback.
 */
void
SPDesktop::onDocumentResized (gdouble width, gdouble height)
{
    assert(canvas->get_affine() == _current_affine.d2w());

    Geom::Rect const a(Geom::Point(0, 0), Geom::Point(width, height));
    canvas_page->set_rect(a);
    canvas_shadow->set_rect(a);
}


void
SPDesktop::_onActivate (SPDesktop* dt)
{
    if (!dt->_widget) return;
    sp_dtw_desktop_activate(dt->_widget);
}

void
SPDesktop::_onDeactivate (SPDesktop* dt)
{
    if (!dt->_widget) return;
    sp_dtw_desktop_deactivate(dt->_widget);
}

void
SPDesktop::_onSelectionModified
(Inkscape::Selection *selection, guint /*flags*/, SPDesktop *dt)
{
    if (!dt->_widget) return;
    dt->_widget->update_scrollbars (dt->_current_affine.getZoom());
    if (selection->desktop()->getInkscapeWindow()) {
        selection->desktop()->getInkscapeWindow()->on_selection_changed();
    }
}

static void
_onSelectionChanged
(Inkscape::Selection *selection, SPDesktop *desktop)
{
    /** \todo
     * only change the layer for single selections, or what?
     * This seems reasonable -- for multiple selections there can be many
     * different layers involved.
     */
    SPItem *item=selection->singleItem();
    if (item) {
        SPObject *layer=desktop->layers->layerForObject(item);
        if ( layer && layer != desktop->currentLayer() ) {
            desktop->layers->setCurrentLayer(layer);
        }
    }
    if (selection->desktop()->getInkscapeWindow()) {
        selection->desktop()->getInkscapeWindow()->on_selection_changed();
    }
}

/**
 * Calls event handler of current event context.
 */
static bool
_drawing_handler (GdkEvent *event, Inkscape::DrawingItem *drawing_item, SPDesktop *desktop)
{
    if (event->type == GDK_KEY_PRESS && Inkscape::UI::Tools::get_latin_keyval(&event->key) == GDK_KEY_space &&
        desktop->event_context->space_panning)
    {
        return true;
    }

    if (drawing_item) {
        SPItem *spi = drawing_item->getItem();
        return sp_event_context_item_handler (desktop->event_context, spi, event);
    } else {
        return sp_event_context_root_handler (desktop->event_context, event);
    }
}

static void
_layer_activated(SPObject *layer, SPDesktop *desktop) {
    g_return_if_fail(SP_IS_GROUP(layer));
    SP_GROUP(layer)->setLayerDisplayMode(desktop->dkey, SPGroup::LAYER);
}

/// Callback
static void
_layer_deactivated(SPObject *layer, SPDesktop *desktop) {
    g_return_if_fail(SP_IS_GROUP(layer));
    SP_GROUP(layer)->setLayerDisplayMode(desktop->dkey, SPGroup::GROUP);
}

/// Callback
static void
_layer_hierarchy_changed(SPObject */*top*/, SPObject *bottom,
                                         SPDesktop *desktop)
{
    desktop->_layer_changed_signal.emit (bottom);
}

/// Called when document is starting to be rebuilt.
static void _reconstruction_start(SPDesktop * desktop)
{
    desktop->_reconstruction_old_layer_id = desktop->currentLayer()->getId() ? desktop->currentLayer()->getId() : "";
    desktop->layers->reset();

    desktop->selection->clear();
}

/// Called when document rebuild is finished.
static void _reconstruction_finish(SPDesktop * desktop)
{
    g_debug("Desktop, finishing reconstruction\n");
    if ( !desktop->_reconstruction_old_layer_id.empty() ) {
        SPObject * newLayer = desktop->namedview->document->getObjectById(desktop->_reconstruction_old_layer_id);
        if (newLayer != nullptr) {
            desktop->layers->setCurrentLayer(newLayer);
        }

        desktop->_reconstruction_old_layer_id.clear();
    }
    g_debug("Desktop, finishing reconstruction end\n");
}

/**
 * Namedview_modified callback.
 */
static void _namedview_modified (SPObject *obj, guint flags, SPDesktop *desktop)
{
    SPNamedView *nv=SP_NAMEDVIEW(obj);

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        if (nv->pagecheckerboard) {
            desktop->getCanvas()->set_background_checkerboard(nv->pagecolor);
        } else {
            desktop->getCanvas()->set_background_color(nv->pagecolor);
        }

        /* Show/hide page border */
        if (nv->showborder) {
            desktop->getCanvasPage()->set_stroke(nv->bordercolor);
            desktop->getCanvasPage()->show();

            // place in the z-order stack
            if (nv->borderlayer == SP_BORDER_LAYER_BOTTOM) {
                desktop->getCanvasPage()->set_z_position(1); // In display group, on top of shadow.
            } else {
                desktop->getCanvasPage()->raise_to_top(); // In display group.
            }

            /* Show/hide page shadow */
            if (nv->showpageshadow && nv->pageshadow) {
                desktop->getCanvasShadow()->set_shadow(nv->bordercolor, nv->pageshadow);
                desktop->getCanvasShadow()->show();
            } else {
                desktop->getCanvasShadow()->hide();
            }

        } else {
            desktop->getCanvasPage()->hide();
            desktop->getCanvasShadow()->hide(); // No page border, no shadow!
        }

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (SP_RGBA32_R_U(nv->pagecolor) +
            SP_RGBA32_G_U(nv->pagecolor) +
            SP_RGBA32_B_U(nv->pagecolor) >= 384) {
            // the background color is light, use black outline
            desktop->getCanvasDrawing()->get_drawing()->outlinecolor =
                prefs->getInt("/options/wireframecolors/onlight", 0xff);
        } else { // use white outline
            desktop->getCanvasDrawing()->get_drawing()->outlinecolor =
                prefs->getInt("/options/wireframecolors/ondark", 0xffffffff);
        }
    }
}

Geom::Affine SPDesktop::w2d() const
{
    return _current_affine.w2d();
}

Geom::Point SPDesktop::w2d(Geom::Point const &p) const
{
    return p * _current_affine.w2d();
}

Geom::Point SPDesktop::d2w(Geom::Point const &p) const
{
    return p * _current_affine.d2w();
}

const Geom::Affine &SPDesktop::doc2dt() const
{
    g_assert(doc() != nullptr);
    return doc()->doc2dt();
}

Geom::Affine SPDesktop::dt2doc() const
{
    g_assert(doc() != nullptr);
    return doc()->dt2doc();
}

Geom::Point SPDesktop::doc2dt(Geom::Point const &p) const
{
    return p * doc2dt();
}

Geom::Point SPDesktop::dt2doc(Geom::Point const &p) const
{
    return p * dt2doc();
}

/*
 * Pop event context from desktop's context stack. Never used.
 */
// void
// SPDesktop::pop_event_context (unsigned int key)
// {
//    ToolBase *ec = NULL;
//
//    if (event_context && event_context->key == key) {
//        g_return_if_fail (event_context);
//        g_return_if_fail (event_context->next);
//        ec = event_context;
//        sp_event_context_deactivate (ec);
//        event_context = ec->next;
//        sp_event_context_activate (event_context);
//        _event_context_changed_signal.emit (this, ec);
//    }
//
//    ToolBase *ref = event_context;
//    while (ref && ref->next && ref->next->key != key)
//        ref = ref->next;
//
//    if (ref && ref->next) {
//        ec = ref->next;
//        ref->next = ec->next;
//    }
//
//    if (ec) {
//        sp_event_context_finish (ec);
//        g_object_unref (G_OBJECT (ec));
//    }
// }

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
