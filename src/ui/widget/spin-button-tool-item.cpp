// SPDX-License-Identifier: GPL-2.0-or-later

#include "spin-button-tool-item.h"

#include <algorithm>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/toolbar.h>

#include <cmath>
#include <utility>

#include "spinbutton.h"
#include "ui/icon-loader.h"

namespace Inkscape {
namespace UI {
namespace Widget {

/**
 * \brief Handler for the button's "focus-in-event" signal
 *
 * \param focus_event The event that triggered the signal
 *
 * \detail This just logs the current value of the spin-button
 *         and sets the _transfer_focus flag
 */
bool
SpinButtonToolItem::on_btn_focus_in_event(GdkEventFocus * /* focus_event */)
{
    _last_val = _btn->get_value();
    _transfer_focus = true;

    return false; // Event not consumed
}

/**
 * \brief Handler for the button's "focus-out-event" signal
 *
 * \param focus_event The event that triggered the signal
 *
 * \detail This just unsets the _transfer_focus flag
 */
bool
SpinButtonToolItem::on_btn_focus_out_event(GdkEventFocus * /* focus_event */)
{
    _transfer_focus = false;

    return false; // Event not consumed
}

/**
 * \brief Handler for the button's "key-press-event" signal
 *
 * \param key_event The event that triggered the signal
 *
 * \detail If the ESC key was pressed, restore the last value and defocus.
 *         If the Enter key was pressed, just defocus.
 */
bool
SpinButtonToolItem::on_btn_key_press_event(GdkEventKey *key_event)
{
    bool was_consumed = false; // Whether event has been consumed or not
    auto display = Gdk::Display::get_default();
    auto keymap  = display->get_keymap();
    guint key = 0;
    gdk_keymap_translate_keyboard_state(keymap, key_event->hardware_keycode,
                                        static_cast<GdkModifierType>(key_event->state),
                                        0, &key, 0, 0, 0);

    auto val = _btn->get_value();

    switch(key) {
        case GDK_KEY_Escape:
        {
            _transfer_focus = true;
            _btn->set_value(_last_val);
            defocus();
            was_consumed = true;
        }
        break;

        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        {
            _transfer_focus = true;
            defocus();
            was_consumed = true;
        }
        break;

        case GDK_KEY_Tab:
        {
            _transfer_focus = false;
            was_consumed = process_tab(1);
        }
        break;

        case GDK_KEY_ISO_Left_Tab:
        {
            _transfer_focus = false;
            was_consumed = process_tab(-1);
        }
        break;

        // TODO: Enable variable step-size if this is ever used
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
        {
            _transfer_focus = false;
            _btn->set_value(val+1);
            was_consumed=true;
        }
        break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
        {
            _transfer_focus = false;
            _btn->set_value(val-1);
            was_consumed=true;
        }
        break;

        case GDK_KEY_Page_Up:
        case GDK_KEY_KP_Page_Up:
        {
            _transfer_focus = false;
            _btn->set_value(val+10);
            was_consumed=true;
        }
        break;

        case GDK_KEY_Page_Down:
        case GDK_KEY_KP_Page_Down:
        {
            _transfer_focus = false;
            _btn->set_value(val-10);
            was_consumed=true;
        }
        break;

        case GDK_KEY_z:
        case GDK_KEY_Z:
        {
            _transfer_focus = false;
            _btn->set_value(_last_val);
            was_consumed = true;
        }
        break;
    }

    return was_consumed;
}

/**
 * \brief Shift focus to a different widget
 *
 * \details This only has an effect if the _transfer_focus flag and the _focus_widget are set
 */
void
SpinButtonToolItem::defocus()
{
    if(_transfer_focus && _focus_widget) {
        _focus_widget->grab_focus();
    }
}

/**
 * \brief Move focus to another spinbutton in the toolbar
 *
 * \param increment[in] The number of places to shift within the toolbar
 */
bool
SpinButtonToolItem::process_tab(int increment)
{
    // If the increment is zero, do nothing
    if(increment == 0) return true;

    // Here, we're working through the widget hierarchy:
    // Toolbar
    // |- ToolItem (*this)
    //    |-> Box
    //       |-> SpinButton (*_btn)
    //
    // Our aim is to find the next/previous spin-button within a toolitem in our toolbar

    bool handled = false;

    // We only bother doing this if the current item is actually in a toolbar!
    auto toolbar = dynamic_cast<Gtk::Toolbar *>(get_parent());

    if (toolbar) {
        // Get the index of the current item within the toolbar and the total number of items
        auto my_index = toolbar->get_item_index(*this);
        auto n_items  = toolbar->get_n_items();

        auto test_index = my_index + increment; // The index of the item we want to check

        // Loop through tool items as long as we're within the limits of the toolbar and
        // we haven't yet found our new item to focus on
        while(test_index > 0 && test_index <= n_items && !handled) {

            auto tool_item = toolbar->get_nth_item(test_index);

            if(tool_item) {
                // There are now two options that we support:
                if (auto sb_tool_item = dynamic_cast<SpinButtonToolItem *>(tool_item)) {
                    // (1) The tool item is a SpinButtonToolItem, in which case, we just pass
                    //     focus to its spin-button
                    sb_tool_item->grab_button_focus();
                    handled = true;
                }
                else if(dynamic_cast<Gtk::SpinButton *>(tool_item->get_child())) {
                    // (2) The tool item contains a plain Gtk::SpinButton, in which case we
                    //     pass focus directly to it
                    tool_item->get_child()->grab_focus();
                }
            }

            test_index += increment;
        }
    }

    return handled;
}

/**
 * \brief Handler for toggle events on numeric menu items
 *
 * \details Sets the adjustment to the desired value
 */
void
SpinButtonToolItem::on_numeric_menu_item_toggled(double value)
{
    auto adj = _btn->get_adjustment();
    adj->set_value(value);
}

Gtk::RadioMenuItem *
SpinButtonToolItem::create_numeric_menu_item(Gtk::RadioButtonGroup *group,
                                             double                 value,
                                             const Glib::ustring&   label)
{
    // Represent the value as a string
    std::ostringstream ss;
    ss << value;

    // Append the label if specified
    if (!label.empty()) {
        ss << ": " << label;
    }

    auto numeric_option = Gtk::manage(new Gtk::RadioMenuItem(*group, ss.str()));

    // Set the adjustment value in response to changes in the selected item
    auto toggled_handler = sigc::bind(sigc::mem_fun(*this, &SpinButtonToolItem::on_numeric_menu_item_toggled), value);
    numeric_option->signal_toggled().connect(toggled_handler);

    return numeric_option;
}

/**
 * \brief Create a menu containing fixed numeric options for the adjustment
 *
 * \details Each of these values represents a snap-point for the adjustment's value
 */
Gtk::Menu *
SpinButtonToolItem::create_numeric_menu()
{
    auto numeric_menu = Gtk::manage(new Gtk::Menu());

    Gtk::RadioMenuItem::Group group;

    // Get values for the adjustment
    auto adj = _btn->get_adjustment();
    auto adj_value = round_to_precision(adj->get_value());
    auto lower = round_to_precision(adj->get_lower());
    auto upper = round_to_precision(adj->get_upper());
    auto page = adj->get_page_increment();

    // Start by setting some fixed values based on the adjustment's
    // parameters.
    NumericMenuData values;

    // first add all custom items (necessary)
    for (auto custom_data : _custom_menu_data) {
        if (custom_data.first >= lower && custom_data.first <= upper) {
            values.emplace(custom_data);
        }
    }

    values.emplace(adj_value, "");

    // for quick page changes using mouse, step can changes can be done with +/- buttons on
    // SpinButton
    values.emplace(::fmin(adj_value + page, upper), "");
    values.emplace(::fmax(adj_value - page, lower), "");

    // add upper/lower limits to options
    if (_show_upper_limit) {
        values.emplace(upper, "");
    }
    if (_show_lower_limit) {
        values.emplace(lower, "");
    }

    auto add_item = [&numeric_menu, this, &group, adj_value](ValueLabel value){
        auto numeric_menu_item = create_numeric_menu_item(&group, value.first, value.second);
        numeric_menu->append(*numeric_menu_item);

        if (adj_value == value.first) {
            numeric_menu_item->set_active();
        }
    };

    if (_sort_decreasing) {
        std::for_each(values.crbegin(), values.crend(), add_item);
    } else {
        std::for_each(values.cbegin(), values.cend(), add_item);
    }

    return numeric_menu;
}

/**
 * \brief Create a menu-item in response to the "create-menu-proxy" signal
 *
 * \detail This is an override for the default Gtk::ToolItem handler so
 *         we don't need to explicitly connect this to the signal. It
 *         runs if the toolitem is unable to fit on the toolbar, and
 *         must be represented by a menu item instead.
 */
bool
SpinButtonToolItem::on_create_menu_proxy()
{
    // The main menu-item.  It just contains the label that normally appears
    // next to the spin-button, and an indicator for a sub-menu.
    auto menu_item = Gtk::manage(new Gtk::MenuItem(_label_text));
    auto numeric_menu = create_numeric_menu();
    menu_item->set_submenu(*numeric_menu);

    set_proxy_menu_item(_name, *menu_item);

    return true; // Finished handling the event
}

/**
 * \brief Create a new SpinButtonToolItem
 *
 * \param[in] name       A unique ID for this tool-item (not translatable)
 * \param[in] label_text The text to display in the toolbar
 * \param[in] adjustment The Gtk::Adjustment to attach to the spinbutton
 * \param[in] climb_rate The climb rate for the spin button (default = 0)
 * \param[in] digits     Number of decimal places to display
 */
SpinButtonToolItem::SpinButtonToolItem(const Glib::ustring            name,
                                       const Glib::ustring&           label_text,
                                       Glib::RefPtr<Gtk::Adjustment>& adjustment,
                                       double                         climb_rate,
                                       int                            digits)
    : _btn(Gtk::manage(new SpinButton(adjustment, climb_rate, digits))),
      _name(std::move(name)),
      _label_text(label_text),
      _digits(digits)
{
    set_margin_start(3);
    set_margin_end(3);
    set_name(_name);

    // Handle popup menu
    _btn->signal_popup_menu().connect(sigc::mem_fun(*this, &SpinButtonToolItem::on_popup_menu), false);

    // Handle button events
    auto btn_focus_in_event_cb = sigc::mem_fun(*this, &SpinButtonToolItem::on_btn_focus_in_event);
    _btn->signal_focus_in_event().connect(btn_focus_in_event_cb, false);

    auto btn_focus_out_event_cb = sigc::mem_fun(*this, &SpinButtonToolItem::on_btn_focus_out_event);
    _btn->signal_focus_out_event().connect(btn_focus_out_event_cb, false);

    auto btn_key_press_event_cb = sigc::mem_fun(*this, &SpinButtonToolItem::on_btn_key_press_event);
    _btn->signal_key_press_event().connect(btn_key_press_event_cb, false);

    auto btn_button_press_event_cb = sigc::mem_fun(*this, &SpinButtonToolItem::on_btn_button_press_event);
    _btn->signal_button_press_event().connect(btn_button_press_event_cb, false);

    _btn->add_events(Gdk::KEY_PRESS_MASK);

    // Create a label
    _label = Gtk::manage(new Gtk::Label(label_text));

    // Arrange the widgets in a horizontal box
    _hbox = Gtk::manage(new Gtk::Box());
    _hbox->set_spacing(3);
    _hbox->pack_start(*_label);
    _hbox->pack_start(*_btn);
    add(*_hbox);
    show_all();
}

void
SpinButtonToolItem::set_icon(const Glib::ustring& icon_name)
{
    _hbox->remove(*_label);
    _icon = Gtk::manage(sp_get_icon_image(icon_name, Gtk::ICON_SIZE_SMALL_TOOLBAR));

    if(_icon) {
        _hbox->pack_start(*_icon);
        _hbox->reorder_child(*_icon, 0);
    }

    show_all();
}

bool
SpinButtonToolItem::on_btn_button_press_event(const GdkEventButton *button_event)
{
    if (gdk_event_triggers_context_menu(reinterpret_cast<const GdkEvent *>(button_event)) &&
            button_event->type == GDK_BUTTON_PRESS) {
        do_popup_menu(button_event);
        return true;
    }

    return false;
}

void
SpinButtonToolItem::do_popup_menu(const GdkEventButton *button_event)
{
    auto menu = create_numeric_menu();
    menu->attach_to_widget(*_btn);
    menu->show_all();
    menu->popup_at_pointer(reinterpret_cast<const GdkEvent *>(button_event));
}

/**
 * \brief Create a popup menu
 */
bool
SpinButtonToolItem::on_popup_menu()
{
    do_popup_menu(nullptr);
    return true;
}

/**
 * \brief Transfers focus to the child spinbutton by default
 */
void
SpinButtonToolItem::on_grab_focus()
{
    grab_button_focus();
}

/**
 * \brief Set the tooltip to display on this (and all child widgets)
 *
 * \param[in] text The tooltip to display
 */
void
SpinButtonToolItem::set_all_tooltip_text(const Glib::ustring& text)
{
    set_tooltip_text(text);
    _btn->set_tooltip_text(text);
}

/**
 * \brief Set the widget that focus moves to when this one loses focus
 *
 * \param widget The widget that will gain focus
 */
void
SpinButtonToolItem::set_focus_widget(Gtk::Widget *widget)
{
    _focus_widget = widget;
}

/**
 * \brief Grab focus on the spin-button widget
 */
void
SpinButtonToolItem::grab_button_focus()
{
    _btn->grab_focus();
}

/**
 * \brief A wrapper of Geom::decimal_round to remember precision
 */
double
SpinButtonToolItem::round_to_precision(double value) {
    return Geom::decimal_round(value, _digits);
}

/**
 * \brief     [discouraged] Set numeric data option in Radio menu.
 *
 * \param[in] values  values to provide as options
 * \param[in] labels  label to show for the value at same index in values.
 *
 * \detail    Use is advised only when there are no labels.
 *            This is discouraged in favor of other overloads of the function, due to error prone
 *            usage. Using two vectors for related data, undermining encapsulation.
 */
void
SpinButtonToolItem::set_custom_numeric_menu_data(const std::vector<double>&        values,
                                                 const std::vector<Glib::ustring>& labels)
{

    if (values.size() != labels.size() && !labels.empty()) {
        g_warning("Cannot add custom menu items. Value and label arrays are different sizes");
        return;
    }

    _custom_menu_data.clear();

    if (labels.empty()) {
        for (const auto &value : values) {
            _custom_menu_data.emplace(round_to_precision(value), "");
        }
        return;
    }

    int i = 0;
    for (const auto &value : values) {
        _custom_menu_data.emplace(round_to_precision(value), labels[i++]);
    }
}

/**
 * \brief     Set numeric data options for Radio menu (densely labeled data).
 *
 * \param[in] value_labels  value and labels to provide as options
 *
 * \detail    Should be used when most of the values have an associated label (densely labeled data)
 *
 */
void
SpinButtonToolItem::set_custom_numeric_menu_data(const std::vector<ValueLabel>& value_labels) {
    _custom_menu_data.clear();
    for(const auto& value_label : value_labels) {
        _custom_menu_data.emplace(round_to_precision(value_label.first), value_label.second);
    }
}


/**
 * \brief     Set numeric data options for Radio menu (sparsely labeled data).
 *
 * \param[in] values         values without labels
 * \param[in] sparse_labels  value and labels to provide as options
 *
 * \detail    Should be used when very few values have an associated label (sparsely labeled data).
 *            Duplicate values in vector and map are acceptable but, values labels in map are
 *            preferred. Avoid using duplicate values intentionally though.
 *
 */
void
SpinButtonToolItem::set_custom_numeric_menu_data(const std::vector<double> &values,
                                                      const std::unordered_map<double, Glib::ustring> &sparse_labels)
{
    _custom_menu_data.clear();

    for(const auto& value_label : sparse_labels) {
        _custom_menu_data.emplace(round_to_precision(value_label.first), value_label.second);
    }

    for(const auto& value : values) {
        _custom_menu_data.emplace(round_to_precision(value), "");
    }

}


void SpinButtonToolItem::show_upper_limit(bool show) { _show_upper_limit = show; }

void SpinButtonToolItem::show_lower_limit(bool show) { _show_lower_limit = show; }

void SpinButtonToolItem::show_limits(bool show) { _show_upper_limit = _show_lower_limit = show; }

void SpinButtonToolItem::sort_decreasing(bool decreasing) { _sort_decreasing = decreasing; }

Glib::RefPtr<Gtk::Adjustment>
SpinButtonToolItem::get_adjustment()
{
    return _btn->get_adjustment();
}
} // namespace Widget
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
