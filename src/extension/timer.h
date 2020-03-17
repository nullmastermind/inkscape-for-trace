// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Here is where the extensions can get timed on when they load and
 * unload.  All of the timing is done in here.
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_TIMER_H__
#define INKSCAPE_EXTENSION_TIMER_H__

#include <cstddef>
#include <sigc++/sigc++.h>
#include <glibmm/datetime.h>

namespace Inkscape {
namespace Extension {

class Extension;

class ExpirationTimer {
    /** \brief Circularly linked list of all timers */
    static ExpirationTimer * timer_list;
    /** \brief Which timer was on top when we started the idle loop */
    static ExpirationTimer * idle_start;
    /** \brief What the current timeout is (in seconds) */
    static long timeout;
    /** \brief Has the timer been started? */
    static bool timer_started;

    /** \brief Is this extension locked from being unloaded? */
    int locked;
    /** \brief Next entry in the list */
    ExpirationTimer * next;
    /** \brief When this timer expires */
    Glib::DateTime expiration;
    /** \brief What extension this function relates to */
    Extension * extension;

    bool expired() const;

    static bool idle_func ();
    static bool timer_func ();

public:
    ExpirationTimer(Extension * in_extension);
    virtual ~ExpirationTimer();

    void touch ();
    void lock ()   { locked++;  };
    void unlock () { locked--; };

    /** \brief Set the timeout variable */
    static void set_timeout (long in_seconds) { timeout = in_seconds; };
};

}; }; /* namespace Inkscape, Extension */

#endif /* INKSCAPE_EXTENSION_TIMER_H__ */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
