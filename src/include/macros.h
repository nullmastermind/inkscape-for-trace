#ifndef SEEN_MACROS_H
#define SEEN_MACROS_H

/**
 * Useful macros for inkscape
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

// I'm of the opinion that this file should be removed, so I will in the future take the necessary steps to wipe it out.
// Macros are not in general bad, but these particular ones are rather ugly. Especially that sp_round one. --Liam

#define sp_signal_disconnect_by_data(o,d) g_signal_handlers_disconnect_matched(o, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, d)

#define sp_round(v,m) (((v) < 0.0) ? ((ceil((v) / (m) - 0.5)) * (m)) : ((floor((v) / (m) + 0.5)) * (m)))


// keyboard modifiers in an event
#define MOD__SHIFT(event) ((event)->key.state & GDK_SHIFT_MASK)
#define MOD__CTRL(event) ((event)->key.state & GDK_CONTROL_MASK)
#define MOD__ALT(event) ((event)->key.state & GDK_MOD1_MASK)
#define MOD__SHIFT_ONLY(event) (((event)->key.state & GDK_SHIFT_MASK) && !((event)->key.state & GDK_CONTROL_MASK) && !((event)->key.state & GDK_MOD1_MASK))
#define MOD__CTRL_ONLY(event) (!((event)->key.state & GDK_SHIFT_MASK) && ((event)->key.state & GDK_CONTROL_MASK) && !((event)->key.state & GDK_MOD1_MASK))
#define MOD__ALT_ONLY(event) (!((event)->key.state & GDK_SHIFT_MASK) && !((event)->key.state & GDK_CONTROL_MASK) && ((event)->key.state & GDK_MOD1_MASK))

#endif  // SEEN_MACROS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
