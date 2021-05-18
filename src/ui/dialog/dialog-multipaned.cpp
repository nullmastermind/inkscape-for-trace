// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief A widget with multiple panes. Agnostic to type what kind of widgets panes contain.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2020 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "dialog-multipaned.h"

#include <glibmm/i18n.h>
#include <glibmm/objectbase.h>
#include <gtkmm/container.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <iostream>
#include <numeric>

#include "ui/dialog/dialog-notebook.h"
#include "ui/widget/canvas-grid.h"
#include "dialog-window.h"

#define DROPZONE_SIZE 16
#define HANDLE_SIZE 12
#define HANDLE_CROSS_SIZE 25

namespace Inkscape {
namespace UI {
namespace Dialog {

/*
 * References:
 *   https://blog.gtk.org/2017/06/
 *   https://developer.gnome.org/gtkmm-tutorial/stable/sec-custom-containers.html.en
 *   https://wiki.gnome.org/HowDoI/Gestures
 *
 * The children widget sizes are "sticky". They change a minimal
 * amount when the parent widget is resized or a child is added or
 * removed.
 *
 * A gesture is used to track handle movement. This must be attached
 * to the parent widget (the offset_x/offset_y values are relative to
 * the widget allocation which changes for the handles as they are
 * moved).
 */

/* ============ MyDropZone ============ */

MyDropZone::MyDropZone(Gtk::Orientation orientation, int size = DROPZONE_SIZE)
    : Glib::ObjectBase("MultipanedDropZone")
    , Gtk::Orientable()
    , Gtk::EventBox()
{
    set_name("MultipanedDropZone");
    set_orientation(orientation);

    if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        set_size_request(size, -1);
    } else {
        set_size_request(-1, size);
    }
}

/* ============  MyHandle  ============ */

MyHandle::MyHandle(Gtk::Orientation orientation, int size = HANDLE_SIZE)
    : Glib::ObjectBase("MultipanedHandle")
    , Gtk::Orientable()
    , Gtk::EventBox()
    , _cross_size(0)
    , _child(nullptr)
{
    set_name("MultipanedHandle");
    set_orientation(orientation);

    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);

    Gtk::Image *image = Gtk::manage(new Gtk::Image());
    if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        image->set_from_icon_name("view-more-symbolic", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        set_size_request(size, -1);
    } else {
        image->set_from_icon_name("view-more-horizontal-symbolic", Gtk::ICON_SIZE_SMALL_TOOLBAR);
        set_size_request(-1, size);
    }
    image->set_pixel_size(size);
    add(*image);

    // Signal
    signal_size_allocate().connect(sigc::mem_fun(*this, &MyHandle::resize_handler));

    show_all();
}

// draw rectangle with rounded corners
void rounded_rectangle(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double w, double h, double r) {
    cr->begin_new_sub_path();
    cr->arc(x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cr->arc(x + w - r, y + r, r, 3 * M_PI / 2, 2 * M_PI);
    cr->arc(x + w - r, y + h - r, r, 0, M_PI / 2);
    cr->arc(x + r, y + h - r, r, M_PI / 2, M_PI);
    cr->close_path();
}

// part of the handle where clicking makes it automatically collapse/expand docked dialogs
Cairo::Rectangle MyHandle::get_active_click_zone() {
    const Gtk::Allocation& allocation = get_allocation();
    double width = allocation.get_width();
    double height = allocation.get_height();
    double h = height / 5;

    Cairo::Rectangle rect = { .x = 0, .y = (height - h) / 2, .width = width, .height = h };
    return rect;
}

bool MyHandle::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    bool ret = EventBox::on_draw(cr);

    // show click indicator/highlight?
    if (_click_indicator && is_click_resize_active() && !_dragging) {
        auto rect = get_active_click_zone();

        if (rect.width > 4 && rect.height > 0) {
            Glib::RefPtr<Gtk::StyleContext> style_context = get_style_context();
            Gdk::RGBA fg = style_context->get_color(get_state_flags());
            rounded_rectangle(cr, rect.x + 2, rect.y, rect.width - 4, rect.height, 3);
            cr->set_source_rgba(fg.get_red(), fg.get_green(), fg.get_blue(), 0.26);
            cr->fill();
        }
    }

    return ret;
}

void MyHandle::set_dragging(bool dragging) {
    if (_dragging != dragging) {
        _dragging = dragging;
        if (_click_indicator) {
            queue_draw();
        }
    }
}

/**
 * Change the mouse pointer into a resize icon to show you can drag.
 */
bool MyHandle::on_enter_notify_event(GdkEventCrossing *crossing_event)
{
    auto window = get_window();
    auto display = get_display();

    if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        auto cursor = Gdk::Cursor::create(display, "col-resize");
        window->set_cursor(cursor);
    } else {
        auto cursor = Gdk::Cursor::create(display, "row-resize");
        window->set_cursor(cursor);
    }

    update_click_indicator(crossing_event->x, crossing_event->y);

    return false;
}

bool MyHandle::on_leave_notify_event(GdkEventCrossing* crossing_event) {
    show_click_indicator(false);
    return false;
}

void MyHandle::show_click_indicator(bool show) {
    if (!is_click_resize_active()) return;

    if (show != _click_indicator) {
        _click_indicator = show;
        this->queue_draw();
    }
}

void MyHandle::update_click_indicator(double x, double y) {
    if (!is_click_resize_active()) return;

    auto rect = get_active_click_zone();
    bool inside =
        x >= rect.x && x < rect.x + rect.width &&
        y >= rect.y && y < rect.y + rect.height;

    show_click_indicator(inside);
}

bool MyHandle::is_click_resize_active() const {
    return get_orientation() == Gtk::ORIENTATION_HORIZONTAL;
}

bool MyHandle::on_button_press_event(GdkEventButton* button_event) {
    // detect single-clicks
    _click = button_event->button == 1 && button_event->type == GDK_BUTTON_PRESS;
    return false;
}

bool MyHandle::on_button_release_event(GdkEventButton* event) {
    // single-click on active zone?
    if (_click && event->type == GDK_BUTTON_RELEASE && event->button == 1 && _click_indicator) {
        _click = false;
        _dragging = false;
        // handle clicked
        if (is_click_resize_active()) {
            toggle_multipaned();
            return true;
        }
    }

    _click = false;
    return false;
}

void MyHandle::toggle_multipaned() {
    // visibility toggle of multipaned in a floating dialog window doesn't make sense; skip
    if (dynamic_cast<DialogWindow*>(get_toplevel())) return;

    auto panel = dynamic_cast<DialogMultipaned*>(get_parent());
    if (!panel) return;

    auto children = panel->get_children();
    Gtk::Widget* multi = nullptr; // multipaned widget to toggle
    bool left_side = true; // panels to the left of canvas
    size_t i = 0;

    // find multipaned widget to resize; it is adjacent (sibling) to 'this' handle in widget hierarchy
    for (auto widget : children) {
        if (dynamic_cast<Inkscape::UI::Widget::CanvasGrid*>(widget)) {
            // widget past canvas are on the right side (of canvas)
            left_side = false;
        }

        if (widget == this) {
            if (left_side && i > 0) {
                // handle to the left of canvas toggles preceeding panel
                multi = dynamic_cast<DialogMultipaned*>(children[i - 1]);
            }
            else if (!left_side && i + 1 < children.size()) {
                // handle to the right of canvas toggles next panel
                multi = dynamic_cast<DialogMultipaned*>(children[i + 1]);
            }

            if (multi) {
                if (multi->is_visible()) {
                    multi->hide();
                }
                else {
                    multi->show();
                }
                // resize parent
                panel->children_toggled();
            }
            break;
        }

        ++i;
    }
}

bool MyHandle::on_motion_notify_event(GdkEventMotion* motion_event) {
    // motion invalidates click; it activates resizing
    _click = false;
    // show_click_indicator(false);
    update_click_indicator(motion_event->x, motion_event->y);
    return false;
}

/**
 * This allocation handler function is used to add/remove handle icons in order to be able
 * to hide completely a transversal handle into the sides of a DialogMultipaned.
 *
 * The image has a specific size set up in the constructor and will not naturally shrink/hide.
 * In conclusion, we remove it from the handle and save it into an internal reference.
 */
void MyHandle::resize_handler(Gtk::Allocation &allocation)
{
    int size = (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) ? allocation.get_height() : allocation.get_width();

    if (_cross_size > size && HANDLE_CROSS_SIZE > size && !_child) {
        _child = get_child();
        remove();
    } else if (_cross_size < size && HANDLE_CROSS_SIZE < size && _child) {
        add(*_child);
        _child = nullptr;
    }

    _cross_size = size;
}

/* ============ DialogMultipaned ============= */

DialogMultipaned::DialogMultipaned(Gtk::Orientation orientation)
    : Glib::ObjectBase("DialogMultipaned")
    , Gtk::Orientable()
    , Gtk::Container()
    , _empty_widget(nullptr)
{
    set_name("DialogMultipaned");
    set_orientation(orientation);
    set_has_window(false);
    set_redraw_on_allocate(false);

    // ============= Add dropzones ==============
    MyDropZone *dropzone_s = Gtk::manage(new MyDropZone(orientation));
    MyDropZone *dropzone_e = Gtk::manage(new MyDropZone(orientation));

    dropzone_s->set_parent(*this);
    dropzone_e->set_parent(*this);

    children.push_back(dropzone_s);
    children.push_back(dropzone_e);

    // ============ Connect signals =============
    gesture = Gtk::GestureDrag::create(*this);

    _connections.emplace_back(
        gesture->signal_drag_begin().connect(sigc::mem_fun(*this, &DialogMultipaned::on_drag_begin)));
    _connections.emplace_back(gesture->signal_drag_end().connect(sigc::mem_fun(*this, &DialogMultipaned::on_drag_end)));
    _connections.emplace_back(
        gesture->signal_drag_update().connect(sigc::mem_fun(*this, &DialogMultipaned::on_drag_update)));

    _connections.emplace_back(
        signal_drag_data_received().connect(sigc::mem_fun(*this, &DialogMultipaned::on_drag_data)));
    _connections.emplace_back(
        dropzone_s->signal_drag_data_received().connect(sigc::mem_fun(*this, &DialogMultipaned::on_prepend_drag_data)));
    _connections.emplace_back(
        dropzone_e->signal_drag_data_received().connect(sigc::mem_fun(*this, &DialogMultipaned::on_append_drag_data)));

    // add empty widget to initiate the container
    add_empty_widget();

    show_all();
}

DialogMultipaned::~DialogMultipaned()
{
    // Disconnect all signals
    for_each(_connections.begin(), _connections.end(), [&](auto c) { c.disconnect(); });
    /*
        for (std::vector<Gtk::Widget *>::iterator it = children.begin(); it != children.end();) {
            if (dynamic_cast<DialogMultipaned *>(*it) || dynamic_cast<DialogNotebook *>(*it)) {
                delete *it;
            } else {
                it++;
            }
        }
    */

    for (;;) {
        auto it = std::find_if(children.begin(), children.end(), [](auto w) {
            return dynamic_cast<DialogMultipaned *>(w) || dynamic_cast<DialogNotebook *>(w);
        });
        if (it != children.end()) {
            // delete dialog multipanel or notebook; this action results in its removal from 'children'!
            delete *it;
        } else {
            // no more dialog panels
            break;
        }
    }

    children.clear();
}

void DialogMultipaned::prepend(Gtk::Widget *child)
{
    remove_empty_widget(); // Will remove extra widget if existing

    // If there are MyMultipane children that are empty, they will be removed
    for (auto const &child1 : children) {
        DialogMultipaned *paned = dynamic_cast<DialogMultipaned *>(child1);
        if (paned && paned->has_empty_widget()) {
            remove(*child1);
            remove_empty_widget();
        }
    }

    if (child) {
        // Add handle
        if (children.size() > 2) {
            MyHandle *my_handle = Gtk::manage(new MyHandle(get_orientation()));
            my_handle->set_parent(*this);
            children.insert(children.begin() + 1, my_handle); // After start dropzone
        }

        // Add child
        children.insert(children.begin() + 1, child);
        if (!child->get_parent())
            child->set_parent(*this);

        // Ideally, we would only call child->show() here and assume that the
        // child has already configured visibility of all its own children.
        child->show_all();
    }
}

void DialogMultipaned::append(Gtk::Widget *child)
{
    remove_empty_widget(); // Will remove extra widget if existing

    // If there are MyMultipane children that are empty, they will be removed
    for (auto const &child1 : children) {
        DialogMultipaned *paned = dynamic_cast<DialogMultipaned *>(child1);
        if (paned && paned->has_empty_widget()) {
            remove(*child1);
            remove_empty_widget();
        }
    }

    if (child) {
        // Add handle
        if (children.size() > 2) {
            MyHandle *my_handle = Gtk::manage(new MyHandle(get_orientation()));
            my_handle->set_parent(*this);
            children.insert(children.end() - 1, my_handle); // Before end dropzone
        }

        // Add child
        children.insert(children.end() - 1, child);
        if (!child->get_parent())
            child->set_parent(*this);

        // See comment in DialogMultipaned::prepend
        child->show_all();
    }
}

void DialogMultipaned::add_empty_widget()
{
    const int EMPTY_WIDGET_SIZE = 60; // magic nummber

    // The empty widget is a label
    auto label = Gtk::manage(new Gtk::Label(_("You can drop dockable dialogs here.")));
    label->set_line_wrap();
    label->set_justify(Gtk::JUSTIFY_CENTER);
    label->set_valign(Gtk::ALIGN_CENTER);
    label->set_vexpand();

    append(label);
    _empty_widget = label;

    if (get_orientation() == Gtk::ORIENTATION_VERTICAL) {
        int dropzone_size = (get_height() - EMPTY_WIDGET_SIZE) / 2;
        if (dropzone_size > DROPZONE_SIZE) {
            set_dropzone_sizes(dropzone_size, dropzone_size);
        }
    }
}

void DialogMultipaned::remove_empty_widget()
{
    if (_empty_widget) {
        auto it = std::find(children.begin(), children.end(), _empty_widget);
        if (it != children.end()) {
            children.erase(it);
        }
        _empty_widget->unparent();
        _empty_widget = nullptr;
    }

    if (get_orientation() == Gtk::ORIENTATION_VERTICAL) {
        set_dropzone_sizes(DROPZONE_SIZE, DROPZONE_SIZE);
    }
}

Gtk::Widget *DialogMultipaned::get_first_widget()
{
    if (children.size() > 2) {
        return children[1];
    } else {
        return nullptr;
    }
}

Gtk::Widget *DialogMultipaned::get_last_widget()
{
    if (children.size() > 2) {
        return children[children.size() - 2];
    } else {
        return nullptr;
    }
}

/**
 * Set the sizes of the DialogMultipaned dropzones.
 * @param start, the size you want or -1 for the default `DROPZONE_SIZE`
 * @param end, the size you want or -1 for the default `DROPZONE_SIZE`
 */
void DialogMultipaned::set_dropzone_sizes(int start, int end)
{
    bool orientation = get_orientation() == Gtk::ORIENTATION_HORIZONTAL;

    if (start == -1) {
        start = DROPZONE_SIZE;
    }

    MyDropZone *dropzone_s = dynamic_cast<MyDropZone *>(children[0]);

    if (dropzone_s) {
        if (orientation) {
            dropzone_s->set_size_request(start, -1);
        } else {
            dropzone_s->set_size_request(-1, start);
        }
    }

    if (end == -1) {
        end = DROPZONE_SIZE;
    }

    MyDropZone *dropzone_e = dynamic_cast<MyDropZone *>(children[children.size() - 1]);

    if (dropzone_e) {
        if (orientation) {
            dropzone_e->set_size_request(end, -1);
        } else {
            dropzone_e->set_size_request(-1, end);
        }
    }
}

/**
 * Show/hide as requested all children of this container that are of type multipaned
 */
void DialogMultipaned::toggle_multipaned_children(bool show)
{
    _handle = -1;
    _drag_handle = -1;

    for (auto child : children) {
        if (auto panel = dynamic_cast<DialogMultipaned*>(child)) {
            if (show) {
                panel->show();
            }
            else {
                panel->hide();
            }
        }
    }
}

/**
 * Ensure that this dialog container is visible.
 */
void DialogMultipaned::ensure_multipaned_children()
{
    toggle_multipaned_children(true);
    // hide_multipaned = false;
    // queue_allocate();
}

// ****************** OVERRIDES ******************

// The following functions are here to define the behavior of our custom container

Gtk::SizeRequestMode DialogMultipaned::get_request_mode_vfunc() const
{
    if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        return Gtk::SIZE_REQUEST_WIDTH_FOR_HEIGHT;
    } else {
        return Gtk::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
    }
}

void DialogMultipaned::get_preferred_width_vfunc(int &minimum_width, int &natural_width) const
{
    minimum_width = 0;
    natural_width = 0;
    for (auto const &child : children) {
        if (child && child->is_visible()) {
            int child_minimum_width = 0;
            int child_natural_width = 0;
            child->get_preferred_width(child_minimum_width, child_natural_width);
            if (get_orientation() == Gtk::ORIENTATION_VERTICAL) {
                minimum_width = std::max(minimum_width, child_minimum_width);
                natural_width = std::max(natural_width, child_natural_width);
            } else {
                minimum_width += child_minimum_width;
                natural_width += child_natural_width;
            }
        }
    }
}

void DialogMultipaned::get_preferred_height_vfunc(int &minimum_height, int &natural_height) const
{
    minimum_height = 0;
    natural_height = 0;
    for (auto const &child : children) {
        if (child && child->is_visible()) {
            int child_minimum_height = 0;
            int child_natural_height = 0;
            child->get_preferred_height(child_minimum_height, child_natural_height);
            if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
                minimum_height = std::max(minimum_height, child_minimum_height);
                natural_height = std::max(natural_height, child_natural_height);
            } else {
                minimum_height += child_minimum_height;
                natural_height += child_natural_height;
            }
        }
    }
}

void DialogMultipaned::get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const
{
    minimum_width = 0;
    natural_width = 0;
    for (auto const &child : children) {
        if (child && child->is_visible()) {
            int child_minimum_width = 0;
            int child_natural_width = 0;
            child->get_preferred_width_for_height(height, child_minimum_width, child_natural_width);
            if (get_orientation() == Gtk::ORIENTATION_VERTICAL) {
                minimum_width = std::max(minimum_width, child_minimum_width);
                natural_width = std::max(natural_width, child_natural_width);
            } else {
                minimum_width += child_minimum_width;
                natural_width += child_natural_width;
            }
        }
    }
}

void DialogMultipaned::get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const
{
    minimum_height = 0;
    natural_height = 0;
    for (auto const &child : children) {
        if (child && child->is_visible()) {
            int child_minimum_height = 0;
            int child_natural_height = 0;
            child->get_preferred_height_for_width(width, child_minimum_height, child_natural_height);
            if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
                minimum_height = std::max(minimum_height, child_minimum_height);
                natural_height = std::max(natural_height, child_natural_height);
            } else {
                minimum_height += child_minimum_height;
                natural_height += child_natural_height;
            }
        }
    }
}


void DialogMultipaned::children_toggled() {
    _handle = -1;
    _drag_handle = -1;
    queue_allocate();
}

/**
 * This function allocates the sizes of the children widgets (be them internal or not) from
 * the container's allocated size.
 *
 * Natural width: The width the widget really wants.
 * Minimum width: The minimum width for a widget to be useful.
 * Minimum <= Natural.
 */
void DialogMultipaned::on_size_allocate(Gtk::Allocation &allocation)
{
    set_allocation(allocation);
    bool horizontal = get_orientation() == Gtk::ORIENTATION_HORIZONTAL;

    if (_drag_handle != -1) { // Exchange allocation between the widgets on either side of moved handle
        // Allocation values calculated in on_drag_update();
        children[_drag_handle - 1]->size_allocate(allocation1);
        children[_drag_handle]->size_allocate(allocationh);
        children[_drag_handle + 1]->size_allocate(allocation2);
        _drag_handle = -1;
    }

    {
        std::vector<bool> expandables;              // Is child expandable?
        std::vector<int> sizes_minimums;            // Difference between allocated space and minimum space.
        std::vector<int> sizes_naturals;            // Difference between allocated space and natural space.
        std::vector<int> sizes(children.size(), 0); // The new allocation sizes
        std::vector<int> sizes_current;             // The current sizes along main axis
        int left = horizontal ? allocation.get_width() : allocation.get_height();

        int index = 0;

        int canvas_index = -1;
        for (auto &child : children) {
            bool visible;
            DialogMultipaned *paned = dynamic_cast<DialogMultipaned *>(child);
            Inkscape::UI::Widget::CanvasGrid *canvas = dynamic_cast<Inkscape::UI::Widget::CanvasGrid *>(child);
            if (canvas) {
                canvas_index = index;
            }

            {
                visible = child->get_visible();
                expandables.push_back(child->compute_expand(get_orientation()));

                Gtk::Requisition req_minimum;
                Gtk::Requisition req_natural;
                child->get_preferred_size(req_minimum, req_natural);
                if (child == _resizing_widget1 || child == _resizing_widget2) {
                    // ignore limits for widget being resized interactively and use their current size
                    req_minimum.width = req_minimum.height = 0;
                    auto alloc = child->get_allocation();
                    req_natural.width = alloc.get_width();
                    req_natural.height = alloc.get_height();
                }

                sizes_minimums.push_back(visible ? horizontal ? req_minimum.width : req_minimum.height : 0);
                sizes_naturals.push_back(visible ? horizontal ? req_natural.width : req_natural.height : 0);
            }

            Gtk::Allocation child_allocation = child->get_allocation();
            sizes_current.push_back(visible ? horizontal ? child_allocation.get_width() : child_allocation.get_height()
                                            : 0);
            index++;
        }

        // Precalculate the minimum, natural and current totals
        int sum_minimums = std::accumulate(sizes_minimums.begin(), sizes_minimums.end(), 0);
        int sum_naturals = std::accumulate(sizes_naturals.begin(), sizes_naturals.end(), 0);
        int sum_current = std::accumulate(sizes_current.begin(), sizes_current.end(), 0);

        if (sum_naturals <= left) {
            sizes = sizes_naturals;
            left -= sum_naturals;
        } else if (sum_minimums <= left && left < sum_naturals) {
            sizes = sizes_minimums;
            left -= sum_minimums;
        }

        if (canvas_index >= 0) { // give remaining space to canvas element
            sizes[canvas_index] += left;
        } else { // or, if in a sub-dialogmultipaned, give it evenly to widgets

            int d = 0;
            for (int i = 0; i < (int)children.size(); ++i) {
                if (expandables[i]) {
                    d++;
                }
            }

            if (d > 0) {
                int idx = 0;
                for (int i = 0; i < (int)children.size(); ++i) {
                    if (expandables[i]) {
                        sizes[i] += (left / d);
                        if (idx < (left % d))
                            sizes[i]++;
                        idx++;
                    }
                }
            }
        }
        left = 0;

        // Check if we actually need to change the sizes on the main axis
        left = horizontal ? allocation.get_width() : allocation.get_height();
        if (left == sum_current) {
            bool valid = true;
            for (int i = 0; i < (int)children.size(); ++i) {
                valid = valid && (sizes_minimums[i] <= sizes_current[i]) &&        // is it over the minimums?
                        (expandables[i] || sizes_current[i] <= sizes_naturals[i]); // but does it want to be expanded?
                if (!valid)
                    break;
            }
            if (valid)
                sizes = sizes_current; // The current sizes are good, don't change anything;
        }

        // Set x and y values of allocations (widths should be correct).
        int current_x = allocation.get_x();
        int current_y = allocation.get_y();

        // Allocate
        for (int i = 0; i < (int)children.size(); ++i) {
            Gtk::Allocation child_allocation = children[i]->get_allocation();
            child_allocation.set_x(current_x);
            child_allocation.set_y(current_y);

            int size = sizes[i];

            if (horizontal) {
                child_allocation.set_width(size);
                current_x += size;
                child_allocation.set_height(allocation.get_height());
            } else {
                child_allocation.set_height(size);
                current_y += size;
                child_allocation.set_width(allocation.get_width());
            }

            children[i]->size_allocate(child_allocation);
        }
    }

    _resizing_widget1 = nullptr;
    _resizing_widget2 = nullptr;
}

void DialogMultipaned::forall_vfunc(gboolean, GtkCallback callback, gpointer callback_data)
{
    for (auto const &child : children) {
        if (child) {
            callback(child->gobj(), callback_data);
        }
    }
}

void DialogMultipaned::on_add(Gtk::Widget *child)
{
    if (child) {
        append(child);
    }
}

/**
 * Callback when a widget is removed from DialogMultipaned and executes the removal.
 * It does not remove handles or dropzones.
 */
void DialogMultipaned::on_remove(Gtk::Widget *child)
{
    if (child) {
        MyDropZone *dropzone = dynamic_cast<MyDropZone *>(child);
        if (dropzone) {
            return;
        }
        MyHandle *my_handle = dynamic_cast<MyHandle *>(child);
        if (my_handle) {
            return;
        }

        const bool visible = child->get_visible();
        if (children.size() > 2) {
            auto it = std::find(children.begin(), children.end(), child);
            if (it != children.end()) {         // child found
                if (it + 2 != children.end()) { // not last widget
                    my_handle = dynamic_cast<MyHandle *>(*(it + 1));
                    my_handle->unparent();
                    child->unparent();
                    children.erase(it, it + 2);
                } else {                        // last widget
                    if (children.size() == 3) { // only widget
                        child->unparent();
                        children.erase(it);
                    } else { // not only widget, delete preceding handle
                        my_handle = dynamic_cast<MyHandle *>(*(it - 1));
                        my_handle->unparent();
                        child->unparent();
                        children.erase(it - 1, it + 1);
                    }
                }
            }
        }
        if (visible) {
            queue_resize();
        }

        if (children.size() == 2) {
            add_empty_widget();
            _empty_widget->set_size_request(300, -1);
            _signal_now_empty.emit();
        }
    }
}

void DialogMultipaned::on_drag_begin(double start_x, double start_y)
{
    _hide_widget1 = _hide_widget2 = nullptr;
    _resizing_widget1 = _resizing_widget2 = nullptr;
    // We clicked on handle.
    bool found = false;
    int child_number = 0;
    Gtk::Allocation allocation = get_allocation();
    for (auto const &child : children) {
        MyHandle *my_handle = dynamic_cast<MyHandle *>(child);
        if (my_handle) {
            Gtk::Allocation child_allocation = my_handle->get_allocation();

            // Did drag start in handle?
            int x = child_allocation.get_x() - allocation.get_x();
            int y = child_allocation.get_y() - allocation.get_y();
            if (x < start_x && start_x < x + child_allocation.get_width() && y < start_y &&
                start_y < y + child_allocation.get_height()) {
                found = true;
                my_handle->set_dragging(true);
                break;
            }
        }
        ++child_number;
    }

    if (!found) {
        gesture->set_state(Gtk::EVENT_SEQUENCE_DENIED);
        return;
    }

    if (child_number < 1 || child_number > (int)(children.size() - 2)) {
        std::cerr << "DialogMultipaned::on_drag_begin: Invalid child (" << child_number << "!!" << std::endl;
        gesture->set_state(Gtk::EVENT_SEQUENCE_DENIED);
        return;
    }

    gesture->set_state(Gtk::EVENT_SEQUENCE_CLAIMED);

    // Save for use in on_drag_update().
    _handle = child_number;
    start_allocation1 = children[_handle - 1]->get_allocation();
    if (!children[_handle - 1]->is_visible()) {
        start_allocation1.set_width(0);
        start_allocation1.set_height(0);
    }
    start_allocationh = children[_handle]->get_allocation();
    start_allocation2 = children[_handle + 1]->get_allocation();
    if (!children[_handle + 1]->is_visible()) {
        start_allocation2.set_width(0);
        start_allocation2.set_height(0);
    }
}

void DialogMultipaned::on_drag_end(double offset_x, double offset_y)
{
    if (_handle >= 0 && _handle < children.size()) {
        if (auto my_handle = dynamic_cast<MyHandle*>(children[_handle])) {
            my_handle->set_dragging(false);
        }
    }

    gesture->set_state(Gtk::EVENT_SEQUENCE_DENIED);
    _handle = -1;
    _drag_handle = -1;
    if (_hide_widget1) {
        _hide_widget1->hide();
    }
    if (_hide_widget2) {
        _hide_widget2->hide();
    }
    _hide_widget1 = nullptr;
    _hide_widget2 = nullptr;
    _resizing_widget1 = nullptr;
    _resizing_widget2 = nullptr;

    queue_allocate(); // reimpose limits if any were bent during interactive resizing
}

// docking panels in application window can be collapsed (to left or right side) to make more
// room for canvas; this functionality is only meaningful in app window, not in floating dialogs
bool can_collapse(Gtk::Widget* widget, Gtk::Widget* handle) {
    // can only collapse DialogMultipaned widgets
    if (!widget || dynamic_cast<DialogMultipaned*>(widget) == nullptr) return false;

    // collapsing is not supported in floating dialogs
    if (dynamic_cast<DialogWindow*>(widget->get_toplevel())) return false;

    auto parent = handle->get_parent();
    if (!parent) return false;

    // find where the resizing handle is in relation to canvas area: left or right side;
    // next, find where the panel is in relation to the handle: on its left or right
    bool left_side = true;
    bool left_handle = false;
    size_t panel_index = 0;
    size_t handle_index = 0;
    size_t i = 0;
    for (auto child : parent->get_children()) {
        if (dynamic_cast<Inkscape::UI::Widget::CanvasGrid*>(child)) {
            left_side = false;
        }
        else if (child == handle) {
            left_handle = left_side;
            handle_index = i;
        }
        else if (child == widget) {
            panel_index = i;
        }
        ++i;
    }

    if (left_handle && panel_index < handle_index) {
        return true;
    }
    if (!left_handle && panel_index > handle_index) {
        return true;
    }

    return false;
}

// return minimum widget size; this fn works for hidden widgets too
int get_min_width(Gtk::Widget* widget) {
    bool hidden = !widget->is_visible();
    if (hidden) widget->show();
    int minimum_size = 0;
    int natural_size = 0;
    widget->get_preferred_width(minimum_size, natural_size);
    if (hidden) widget->hide();
    return minimum_size;
}

// Different docking resizing activities use easing functions to speed up or slow down certain phases of resizing
// Below are two picewise linear functions used for that purpose

// easing function for revealing collapsed panels
double reveal_curve(double val, double size) {
    if (size > 0 && val <= size && val >= 0) {
        // slow start (resistance to opening) and then quick reveal
        auto x = val / size;
        auto pos = x;
        if (x <= 0.2) {
            pos = x * 0.25;
        }
        else {
            pos = x * 9.5 - 1.85;
            if (pos > 1) pos = 1;
        }
        return size * pos;
    }

    return val;
}

// easing function for collapsing panels
// note: factors for x dictate how fast resizing happens when moving mouse (with 1 being at the same speed);
// other constants are to make this fn produce values in 0..1 range and seamlessly connect three segments
double collapse_curve(double val, double size) {
    if (size > 0 && val <= size && val >= 0) {
        // slow start (resistance), short pause and then quick collapse
        auto x = val / size;
        auto pos = x;
        if (x < 0.5) {
            // fast collapsing
            pos = x * 10 - 5 + 0.92;
            if (pos < 0) {
                // panel collapsed
                pos = 0;
            }
        }
        else if (x < 0.6) {
            // pause
            pos = 0.2 * 0.6 + 0.8; // = 0.92;
        }
        else {
            // resistance to collapsing (move slow, x 0.2 decrease)
            pos = x * 0.2 + 0.8;
        }
        return size * pos;
    }

    return val;
}

void DialogMultipaned::on_drag_update(double offset_x, double offset_y)
{
    if (_handle < 0) return;

    auto child1 = children[_handle - 1];
    auto child2 = children[_handle + 1];
    allocation1 = children[_handle - 1]->get_allocation();
    allocationh = children[_handle]->get_allocation();
    allocation2 = children[_handle + 1]->get_allocation();

    // HACK: The bias prevents erratic resizing when dragging the handle fast, outside the bounds of the app.
    const int BIAS = 1;

    if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {

        auto handle = children[_handle];

        // function to resize panel
        auto resize_fn = [](Gtk::Widget* handle, Gtk::Widget* child, int start_width, double& offset_x) {
            int minimum_size = get_min_width(child);
            auto width = start_width + offset_x;
            bool resizing = false;
            bool hide = false;

            if (!child->is_visible() && can_collapse(child, handle)) {
                child->show();
                resizing = true;
            }

            if (width < minimum_size) {
                if (can_collapse(child, handle)) {
                    resizing = true;
                    auto w = start_width == 0 ? reveal_curve(width, minimum_size) : collapse_curve(width, minimum_size);
                    offset_x = w - start_width;
                    // facilitate closing/opening panels: users don't have to drag handle all the
                    // way to collapse/expand a panel, they just need to move it fraction of the way;
                    // note: those thresholds correspond to the easing functions used
                    auto threshold = start_width == 0 ? minimum_size * 0.20 : minimum_size * 0.42;
                    hide = width <= threshold ? child : nullptr;
                }
                else {
                    offset_x = -(start_width - minimum_size) + BIAS;
                }
            }

            return std::make_pair(resizing, hide);
        };

        /*
        TODO NOTE:
        Resizing should ideally take into account all columns, not just adjacent ones (left and right here).
        Without it, expanding second collapsed column does not work, since first one may already have min width,
        and cannot be shrunk anymore. Instead it should be pushed out of the way (canvas should be shrunk).
        */

        // panel on the left
        auto action1 = resize_fn(handle, child1, start_allocation1.get_width(), offset_x);
        _resizing_widget1 = action1.first ? child1 : nullptr;
        _hide_widget1 = action1.second ? child1 : nullptr;

        // panel on the right (needs reversing offset_x, so it can use the same logic)
        offset_x = -offset_x;
        auto action2 = resize_fn(handle, child2, start_allocation2.get_width(), offset_x);
        _resizing_widget2 = action2.first ? child2 : nullptr;
        _hide_widget2 = action2.second ? child2 : nullptr;
        offset_x = -offset_x;

        // set new sizes; they may temporarily violate min size panel requirements
        // GTK is not happy about 0-size allocations
        allocation1.set_width(start_allocation1.get_width() + offset_x);
        allocationh.set_x(start_allocationh.get_x() + offset_x);
        allocation2.set_x(start_allocation2.get_x() + offset_x);
        allocation2.set_width(start_allocation2.get_width() - offset_x);
    } else {
        // nothing fancy about resizing in vertical direction; no panel collapsing happens here
        int minimum_size;
        int natural_size;
        children[_handle - 1]->get_preferred_height(minimum_size, natural_size);
        if (start_allocation1.get_height() + offset_y < minimum_size)
            offset_y = -(start_allocation1.get_height() - minimum_size) + BIAS;
        children[_handle + 1]->get_preferred_height(minimum_size, natural_size);
        if (start_allocation2.get_height() - offset_y < minimum_size)
            offset_y = start_allocation2.get_height() - minimum_size - BIAS;

        allocation1.set_height(start_allocation1.get_height() + offset_y);
        allocationh.set_y(start_allocationh.get_y() + offset_y);
        allocation2.set_y(start_allocation2.get_y() + offset_y);
        allocation2.set_height(start_allocation2.get_height() - offset_y);
    }

    _drag_handle = _handle;
    queue_allocate(); // Relayout DialogMultipaned content.
}

void DialogMultipaned::set_target_entries(const std::vector<Gtk::TargetEntry> &target_entries)
{
    drag_dest_set(target_entries);
    ((MyDropZone *)children[0])->drag_dest_set(target_entries, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);
    ((MyDropZone *)children[children.size() - 1])
        ->drag_dest_set(target_entries, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);
}

void DialogMultipaned::on_drag_data(const Glib::RefPtr<Gdk::DragContext> context, int x, int y,
                                    const Gtk::SelectionData &selection_data, guint info, guint time)
{
    _signal_prepend_drag_data.emit(context);
}

void DialogMultipaned::on_prepend_drag_data(const Glib::RefPtr<Gdk::DragContext> context, int x, int y,
                                            const Gtk::SelectionData &selection_data, guint info, guint time)
{
    _signal_prepend_drag_data.emit(context);
}

void DialogMultipaned::on_append_drag_data(const Glib::RefPtr<Gdk::DragContext> context, int x, int y,
                                           const Gtk::SelectionData &selection_data, guint info, guint time)
{
    _signal_append_drag_data.emit(context);
}

// Signals
sigc::signal<void, const Glib::RefPtr<Gdk::DragContext>> DialogMultipaned::signal_prepend_drag_data()
{
    resize_children();
    return _signal_prepend_drag_data;
}

sigc::signal<void, const Glib::RefPtr<Gdk::DragContext>> DialogMultipaned::signal_append_drag_data()
{
    resize_children();
    return _signal_append_drag_data;
}

sigc::signal<void> DialogMultipaned::signal_now_empty()
{
    return _signal_now_empty;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
