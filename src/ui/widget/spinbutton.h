// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Johan B. C. Engelen
 *
 * Copyright (C) 2011 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_SPINBUTTON_H
#define INKSCAPE_UI_WIDGET_SPINBUTTON_H

#include <gtkmm/spinbutton.h>

#include "scrollprotected.h"

namespace Inkscape {
namespace UI {
namespace Widget {

class UnitMenu;
class UnitTracker;

/**
 * SpinButton widget, that allows entry of simple math expressions (also units, when linked with UnitMenu),
 * and allows entry of both '.' and ',' for the decimal, even when in numeric mode.
 *
 * Calling "set_numeric()" effectively disables the expression parsing. If no unit menu is linked, all unitlike characters are ignored.
 */
class SpinButton : public ScrollProtected<Gtk::SpinButton>
{
    using parent_type = ScrollProtected<Gtk::SpinButton>;

public:
    using parent_type::parent_type;

  void setUnitMenu(UnitMenu* unit_menu) { _unit_menu = unit_menu; };
  
  void addUnitTracker(UnitTracker* ut) { _unit_tracker = ut; };

  // TODO: Might be better to just have a default value and a reset() method?
  inline void set_zeroable(const bool zeroable = true) { _zeroable = zeroable; }
  inline void set_oneable(const bool oneable = true) { _oneable = oneable; }

  inline bool get_zeroable() const { return _zeroable; }
  inline bool get_oneable() const { return _oneable; }

  void defocus();

protected:
  UnitMenu    *_unit_menu    = nullptr; ///< Linked unit menu for unit conversion in entered expressions.
  UnitTracker *_unit_tracker = nullptr; ///< Linked unit tracker for unit conversion in entered expressions.
  double _on_focus_in_value  = 0.;
  Gtk::Widget *_defocus_widget = nullptr; ///< Widget that should grab focus when the spinbutton defocuses

  bool _zeroable = false; ///< Reset-value should be zero
  bool _oneable  = false; ///< Reset-value should be one

  bool _stay = false; ///< Whether to ignore defocusing
  bool _dont_evaluate = false; ///< Don't attempt to evaluate expressions

    /**
     * This callback function should try to convert the entered text to a number and write it to newvalue.
     * It calls a method to evaluate the (potential) mathematical expression.
     *
     * @retval false No conversion done, continue with default handler.
     * @retval true  Conversion successful, don't call default handler. 
     */
    int on_input(double* newvalue) override;

    /**
     * When focus is obtained, save the value to enable undo later.
     * @retval false continue with default handler.
     * @retval true  don't call default handler. 
     */
    bool on_focus_in_event(GdkEventFocus *) override;

    /**
     * Handle specific keypress events, like Ctrl+Z.
     *
     * @retval false continue with default handler.
     * @retval true  don't call default handler. 
     */
    bool on_key_press_event(GdkEventKey *) override;

    /**
     * Undo the editing, by resetting the value upon when the spinbutton got focus.
     */
    void undo();

  public:
    inline void set_defocus_widget(const decltype(_defocus_widget) widget) { _defocus_widget = widget; }
    inline void set_dont_evaluate(bool flag) { _dont_evaluate = flag; }
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_SPINBUTTON_H

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
