// SPDX-License-Identifier: GPL-2.0-or-later

#include "simple-pref-pusher.h"

#include <gtkmm/toggletoolbutton.h>

namespace Inkscape {
namespace UI {
SimplePrefPusher::SimplePrefPusher( Gtk::ToggleToolButton *btn, Glib::ustring const &path ) :
    Observer(path),
    _btn(btn),
    freeze(false)
{
    freeze = true;
    _btn->set_active( Inkscape::Preferences::get()->getBool(observed_path) );
    freeze = false;

    Inkscape::Preferences::get()->addObserver(*this);
}

SimplePrefPusher::~SimplePrefPusher()
{
    Inkscape::Preferences::get()->removeObserver(*this);
}

void
SimplePrefPusher::notify(Inkscape::Preferences::Entry const &newVal)
{
    bool newBool = newVal.getBool();
    bool oldBool = _btn->get_active();

    if (!freeze && (newBool != oldBool)) {
        _btn->set_active(newBool);
    }
}

}
}

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
