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

    return false;
}

/**
 * This allocation handler function is used to add/remove handle icons in order to be able
 * to hide completely a transversal handle into the sides of a DialogMultipaned.
 *
 * The image has a specific size set up in the constructor and will not naturally shrink/hide.
 * In conclusion, we remove it from the handle and save it into an internal reference.
 */
void MyHandle::resize_handler(Gtk::Allocation &allocation) {
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
    , hide_multipaned(false)
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

    for (std::vector<Gtk::Widget *>::iterator it = children.begin(); it != children.end();) {
        if (dynamic_cast<DialogMultipaned *>(*it) || dynamic_cast<DialogNotebook *>(*it)) {
            delete *it;
        } else {
            it++;
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
 * Hide all children of this container that are of type multipaned by setting their allocation on the main axis to 0.
 */
void DialogMultipaned::toggle_multipaned_children()
{
    hide_multipaned = !hide_multipaned;
    queue_allocate();
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

    if (handle != -1) { // Exchange allocation between the widgets on either side of moved handle
        // Allocation values calculated in on_drag_update();
        children[handle - 1]->size_allocate(allocation1);
        children[handle]->size_allocate(allocationh);
        children[handle + 1]->size_allocate(allocation2);
    } else {
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
            if (hide_multipaned && paned) {
                visible = false;
                expandables.push_back(false);
                sizes_minimums.push_back(0);
                sizes_naturals.push_back(0);
            } else {
                visible = child->get_visible();
                expandables.push_back(child->compute_expand(get_orientation()));

                Gtk::Requisition req_minimum;
                Gtk::Requisition req_natural;
                child->get_preferred_size(req_minimum, req_natural);

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
        } else { //or, if in a sub-dialogmultipaned, give it evenly to widgets
            
            int d = 0;
            for (int i = 0; i < (int)children.size(); ++i) {
                if (expandables[i]) {
                    d++;
                }
            }

            if(d>0) {
                int idx = 0;
                for (int i = 0; i < (int)children.size(); ++i) {
                    if (expandables[i]) {
                        sizes[i] += (left/d);
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
    handle = child_number;
    start_allocation1 = children[handle - 1]->get_allocation();
    start_allocationh = children[handle]->get_allocation();
    start_allocation2 = children[handle + 1]->get_allocation();
}

void DialogMultipaned::on_drag_end(double offset_x, double offset_y)
{
    gesture->set_state(Gtk::EVENT_SEQUENCE_DENIED);
    handle = -1;
}

void DialogMultipaned::on_drag_update(double offset_x, double offset_y)
{
    allocation1 = children[handle - 1]->get_allocation();
    allocationh = children[handle]->get_allocation();
    allocation2 = children[handle + 1]->get_allocation();
    int minimum_size;
    int natural_size;

    // HACK: The bias prevents erratic resizing when dragging the handle fast, outside the bounds of the app.
    const int BIAS = 1;

    if (get_orientation() == Gtk::ORIENTATION_HORIZONTAL) {
        children[handle - 1]->get_preferred_width(minimum_size, natural_size);
        if (start_allocation1.get_width() + offset_x < minimum_size)
            offset_x = -(start_allocation1.get_width() - minimum_size) + BIAS;
        children[handle + 1]->get_preferred_width(minimum_size, natural_size);
        if (start_allocation2.get_width() - offset_x < minimum_size)
            offset_x = start_allocation2.get_width() - minimum_size - BIAS;

        allocation1.set_width(start_allocation1.get_width() + offset_x);
        allocationh.set_x(start_allocationh.get_x() + offset_x);
        allocation2.set_x(start_allocation2.get_x() + offset_x);
        allocation2.set_width(start_allocation2.get_width() - offset_x);
    } else {
        children[handle - 1]->get_preferred_height(minimum_size, natural_size);
        if (start_allocation1.get_height() + offset_y < minimum_size)
            offset_y = -(start_allocation1.get_height() - minimum_size) + BIAS;
        children[handle + 1]->get_preferred_height(minimum_size, natural_size);
        if (start_allocation2.get_height() - offset_y < minimum_size)
            offset_y = start_allocation2.get_height() - minimum_size - BIAS;

        allocation1.set_height(start_allocation1.get_height() + offset_y);
        allocationh.set_y(start_allocationh.get_y() + offset_y);
        allocation2.set_y(start_allocation2.get_y() + offset_y);
        allocation2.set_height(start_allocation2.get_height() - offset_y);
    }

    if (hide_multipaned) {
        DialogMultipaned *left = dynamic_cast<DialogMultipaned *>(children[handle - 1]);
        DialogMultipaned *right = dynamic_cast<DialogMultipaned *>(children[handle + 1]);

        if (left || right) {
            return;
        }
    }

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
