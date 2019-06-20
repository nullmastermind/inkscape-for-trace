// SPDX-License-Identifier: GPL-2.0-or-later

#include "spin-button-tool-item.h"

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/toolbar.h>

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

        // Loop through tool items as long as we're within the bounds of the toolbar and
        // we haven't yet found our new item to focus on
        while(test_index > 0 && test_index <= n_items && !handled) {

            auto tool_item = toolbar->get_nth_item(test_index);

            if(tool_item) {
                // There are now two options that we support:
                if(dynamic_cast<SpinButtonToolItem *>(tool_item)) {
                    // (1) The tool item is a SpinButtonToolItem, in which case, we just pass
                    //     focus to its spin-button
                    dynamic_cast<SpinButtonToolItem *>(tool_item)->grab_button_focus();
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
    auto adj_value = adj->get_value();
    auto lower = adj->get_lower();
    auto upper = adj->get_upper();
    auto step = adj->get_step_increment();
    auto page = adj->get_page_increment();

    auto digits = _btn->get_digits();

    // A number a little smaller than the smallest increment that can be
    // displayed in the spinbutton entry.
    //
    // For example, if digits = 0, we are displaying integers only and
    // epsilon = 0.9 * 10^-0 = 0.9
    //
    // For digits = 1, we get epsilon = 0.9 * 10^-1 = 0.09
    // For digits = 2, we get epsilon = 0.9 * 10^-2 = 0.009 etc...
    auto epsilon = 0.9 * pow(10.0, -float(digits));

    // Start by setting some fixed values based on the adjustment's
    // parameters.
    NumericMenuData values;
    values.push_back(std::make_pair(upper,            ""));
    values.push_back(std::make_pair(adj_value + page, ""));
    values.push_back(std::make_pair(adj_value + step, ""));
    values.push_back(std::make_pair(adj_value,        ""));
    values.push_back(std::make_pair(adj_value - step, ""));
    values.push_back(std::make_pair(adj_value - page, ""));
    values.push_back(std::make_pair(lower,            ""));

    // Now add any custom items
    for (auto custom_data : _custom_menu_data) {
        values.push_back(custom_data);
    }

    // Sort the numeric menu items into reverse numerical order (largest at top of menu)
    std::sort   (begin(values), end(values));
    std::reverse(begin(values), end(values));

    for (auto value : values)
    {
        auto numeric_menu_item = create_numeric_menu_item(&group, value.first, value.second);
        numeric_menu->append(*numeric_menu_item);

        if (fabs(adj_value - value.first) < epsilon) {
            // If the adjustment value is very close to the value of this menu item,
            // make this menu item active
            numeric_menu_item->set_active();
        }
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
      _last_val(0.0),
      _transfer_focus(false),
      _focus_widget(nullptr)
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

void
SpinButtonToolItem::set_custom_numeric_menu_data(std::vector<double>&              values,
                                                 const std::vector<Glib::ustring>& labels)
{
    if(values.size() != labels.size() && !labels.empty()) {
        g_warning("Cannot add custom menu items.  Value and label arrays are different sizes");
        return;
    }

    _custom_menu_data.clear();

    int i = 0;

    for (auto value : values) {
        if(labels.empty()) {
            _custom_menu_data.push_back(std::make_pair(value, ""));
        }
        else {
            _custom_menu_data.push_back(std::make_pair(value, labels[i++]));
        }
    }
}

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
