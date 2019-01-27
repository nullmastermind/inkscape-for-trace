// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SPIN_BUTTON_TOOL_ITEM_H
#define SEEN_SPIN_BUTTON_TOOL_ITEM_H

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtkmm/toolitem.h>

namespace Gtk {
class RadioButtonGroup;
class RadioMenuItem;
}

namespace Inkscape {
namespace UI {
namespace Widget {

class SpinButton;

/**
 * \brief A spin-button with a label that can be added to a toolbar
 */
class SpinButtonToolItem : public Gtk::ToolItem
{
private:
    Glib::ustring  _name;           ///< A unique ID for the widget (NOT translatable)
    SpinButton    *_btn;            ///< The spin-button within the widget
    Glib::ustring  _label_text;     ///< A string to use in labels for the widget (translatable)
    double         _last_val;       ///< The last value of the adjustment
    bool           _transfer_focus; ///< Whether or not to transfer focus

    /** A widget that grabs focus when this one loses it */
    Gtk::Widget * _focus_widget;

    // Event handlers
    bool on_btn_focus_in_event(GdkEventFocus  *focus_event);
    bool on_btn_focus_out_event(GdkEventFocus *focus_event);
    bool on_btn_key_press_event(GdkEventKey   *key_event);
    bool on_btn_button_press_event(GdkEventButton *button_event);
    bool on_popup_menu();
    void do_popup_menu(GdkEventButton *button_event);

    void defocus();
    bool process_tab(int direction);

    void on_numeric_menu_item_toggled(double value);

    Gtk::Menu * create_numeric_menu();

    Gtk::RadioMenuItem * create_numeric_menu_item(Gtk::RadioButtonGroup *group,
                                                  double                 value,
                                                  const Glib::ustring&   label = "");

protected:
    bool on_create_menu_proxy() override;
    void on_grab_focus() override;

public:
    SpinButtonToolItem(const Glib::ustring            name,
                       const Glib::ustring&           label_text,
                       Glib::RefPtr<Gtk::Adjustment>& adjustment,
                       double                         climb_rate = 0.1,
                       double                         digits     = 3);

    void set_all_tooltip_text(const Glib::ustring& text);
    void set_focus_widget(Gtk::Widget *widget);
    void grab_button_focus();
};
} // namespace Widget
} // namespace UI
} // namespace Inkscape
#endif // SEEN_SPIN_BUTTON_TOOL_ITEM_H
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
