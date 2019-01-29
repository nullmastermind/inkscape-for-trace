// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SIMPLE_PREF_PUSHER_H
#define SEEN_SIMPLE_PREF_PUSHER_H

#include "preferences.h"

namespace Gtk {
class ToggleToolButton;
}

namespace Inkscape {
namespace UI {

/**
 * A simple mediator class that sets the state of a Gtk::ToggleToolButton when
 * a preference is changed.  Unlike the PrefPusher class, this does not provide
 * the reverse process, so you still need to write your own handler for the
 * "toggled" signal on the ToggleToolButton.
 */
class SimplePrefPusher : public Inkscape::Preferences::Observer
{
public:
    /**
     * Constructor for a boolean value that syncs to the supplied path.
     * Initializes the widget to the current preference stored state and registers callbacks
     * for widget changes and preference changes.
     *
     * @param act the widget to synchronize preference with.
     * @param path the path to the preference the widget is synchronized with.
     * @param callback function to invoke when changes are pushed.
     * @param cbData data to be passed on to the callback function.
     */
    SimplePrefPusher(Gtk::ToggleToolButton *btn,
                     Glib::ustring const &  path);

    /**
     * Destructor that unregisters the preference callback.
     */
    ~SimplePrefPusher() override;

    /**
     * Callback method invoked when the preference setting changes.
     */
    void notify(Inkscape::Preferences::Entry const &new_val) override;


private:
    Gtk::ToggleToolButton *_btn;
    bool freeze;
};

}
}
#endif

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
