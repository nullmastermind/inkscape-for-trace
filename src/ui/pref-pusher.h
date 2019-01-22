// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_PREF_PUSHER_H
#define SEEN_PREF_PUSHER_H

#include "preferences.h"

typedef struct _GtkToggleAction GtkToggleAction;

namespace Inkscape {
namespace UI {

/**
 * A simple mediator class that keeps UI controls matched to the preference values they set.
 */
class PrefPusher : public Inkscape::Preferences::Observer
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
    PrefPusher( GtkToggleAction       *act,
                Glib::ustring const &  path,
                void (*callback)(gpointer) = nullptr,
                gpointer cbData = nullptr );

    /**
     * Destructor that unregisters the preference callback.
     */
    ~PrefPusher() override;

    /**
     * Callback method invoked when the preference setting changes.
     */
    void notify(Inkscape::Preferences::Entry const &new_val) override;


private:
    /**
     * Callback hook invoked when the widget changes.
     *
     * @param act the toggle action widget that was changed.
     * @param self the PrefPusher instance the callback was registered to.
     */
   static void toggleCB( GtkToggleAction *act, PrefPusher *self );

    /**
     * Method to handle the widget change.
     */
    void handleToggled();

    GtkToggleAction *act;
    void (*callback)(gpointer);
    gpointer cbData;
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
