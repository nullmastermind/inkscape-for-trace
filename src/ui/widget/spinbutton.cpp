// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Johan B. C. Engelen
 *
 * Copyright (C) 2011 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spinbutton.h"
#include "unit-menu.h"
#include "unit-tracker.h"
#include "util/expression-evaluator.h"
#include "ui/tools/tool-base.h"

namespace Inkscape {
namespace UI {
namespace Widget {


void
SpinButton::connect_signals() {
    signal_input().connect(sigc::mem_fun(this, &SpinButton::on_input));
    signal_focus_in_event().connect(sigc::mem_fun(this, &SpinButton::on_my_focus_in_event));
    signal_key_press_event().connect(sigc::mem_fun(this, &SpinButton::on_my_key_press_event));
    gtk_widget_add_events(GTK_WIDGET(gobj()), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
    signal_scroll_event().connect(sigc::mem_fun(this, &SpinButton::on_scroll_event));
    set_focus_on_click(true);
};

int SpinButton::on_input(double* newvalue)
{
    if (_dont_evaluate) return false;

    try {
        Inkscape::Util::EvaluatorQuantity result;
        if (_unit_menu || _unit_tracker) {
            Unit const *unit = nullptr;
            if (_unit_menu) {
                unit = _unit_menu->getUnit();
            } else {
                unit = _unit_tracker->getActiveUnit();
            }
            Inkscape::Util::ExpressionEvaluator eval = Inkscape::Util::ExpressionEvaluator(get_text().c_str(), unit);
            result = eval.evaluate();
            // check if output dimension corresponds to input unit
            if (result.dimension != (unit->isAbsolute() ? 1 : 0) ) {
                throw Inkscape::Util::EvaluatorException("Input dimensions do not match with parameter dimensions.","");
            }
        } else {
            Inkscape::Util::ExpressionEvaluator eval = Inkscape::Util::ExpressionEvaluator(get_text().c_str(), nullptr);
            result = eval.evaluate();
        }
        *newvalue = result.value;
    }
    catch(Inkscape::Util::EvaluatorException &e) {
        g_message ("%s", e.what());

        return false;
    }

    return true;
}

bool SpinButton::on_my_focus_in_event(GdkEventFocus* /*event*/)
{
    _on_focus_in_value = get_value();
    return false; // do not consume the event
}



bool SpinButton::on_scroll_event(GdkEventScroll *event)
{
    if (!is_focus()) {
        return false;
    }
    double step, page;
    get_increments(step, page);
    if (event->state & GDK_CONTROL_MASK) {
        step = page;
    }
    double change = 0.0;
    if (event->direction == GDK_SCROLL_UP) {
        change = step;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        change = -step;
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
        double delta_y_clamped = CLAMP(event->delta_y, -1, 1); // values > 1 result in excessive changes
        change = step * -delta_y_clamped;
    } else {
        return false;
    }
    set_value(get_value() + change);
    return true;
}

bool SpinButton::on_my_key_press_event(GdkEventKey* event)
{
    switch (Inkscape::UI::Tools::get_latin_keyval (event)) {
    case GDK_KEY_Escape: // defocus
        undo();
        defocus();
        return true; // I consumed the event
        break;
    case GDK_KEY_Return: // defocus
    case GDK_KEY_KP_Enter:
        defocus();
        return true;
        break;
    case GDK_KEY_Tab:
    case GDK_KEY_ISO_Left_Tab:
        // set the flag meaning "do not leave toolbar when changing value"
        _stay = true;
        return false;
        break;
    case GDK_KEY_z:
    case GDK_KEY_Z:
        _stay = true;
        if (event->state & GDK_CONTROL_MASK) {
            undo();
            return true; // I consumed the event
        }
        break;
    default:
        break;
    }

    return false; // do not consume the event
}

void SpinButton::undo()
{
    set_value(_on_focus_in_value);
}

void SpinButton::defocus()
{
    // defocus spinbutton by moving focus to the canvas, unless "stay" is on
    if (_stay) {
        _stay = false;
    } else {
        if (_defocus_widget) {
            _defocus_widget->grab_focus();
        }
    }
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
