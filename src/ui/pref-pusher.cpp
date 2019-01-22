// SPDX-License-Identifier: GPL-2.0-or-later

#include "pref-pusher.h"

#include <gtk/gtk.h>

namespace Inkscape {
namespace UI {
PrefPusher::PrefPusher( GtkToggleAction *act, Glib::ustring const &path, void (*callback)(gpointer), gpointer cbData ) :
    Observer(path),
    act(act),
    callback(callback),
    cbData(cbData),
    freeze(false)
{
    g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggleCB), this);
    freeze = true;
    gtk_toggle_action_set_active( act, Inkscape::Preferences::get()->getBool(observed_path) );
    freeze = false;

    Inkscape::Preferences::get()->addObserver(*this);
}

PrefPusher::~PrefPusher()
{
    Inkscape::Preferences::get()->removeObserver(*this);
}

void PrefPusher::toggleCB( GtkToggleAction * /*act*/, PrefPusher *self )
{
    if (self) {
        self->handleToggled();
    }
}

void PrefPusher::handleToggled()
{
    if (!freeze) {
        freeze = true;
        Inkscape::Preferences::get()->setBool(observed_path, gtk_toggle_action_get_active( act ));
        if (callback) {
            (*callback)(cbData);
        }
        freeze = false;
    }
}

void PrefPusher::notify(Inkscape::Preferences::Entry const &newVal)
{
    bool newBool = newVal.getBool();
    bool oldBool = gtk_toggle_action_get_active(act);

    if (!freeze && (newBool != oldBool)) {
        gtk_toggle_action_set_active( act, newBool );
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
