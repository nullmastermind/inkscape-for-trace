// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

// I'm of the opinion that this file should be removed, so I will in the future take the necessary steps to wipe it out.
// Macros are not in general bad, but these particular ones are rather ugly. --Liam

#define sp_signal_disconnect_by_data(o,d) g_signal_handlers_disconnect_matched(o, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, d)

// "primary" modifier: Ctrl on Linux/Windows and Cmd on macOS.
// note: Could query this at runtime with
// `gdk_keymap_get_modifier_mask(..., GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR)`
#ifdef GDK_WINDOWING_QUARTZ
#define INK_GDK_PRIMARY_MASK GDK_MOD2_MASK
#else
#define INK_GDK_PRIMARY_MASK GDK_CONTROL_MASK
#endif

// all modifiers used by Inkscape
#define INK_GDK_MODIFIER_MASK (GDK_SHIFT_MASK | INK_GDK_PRIMARY_MASK | GDK_MOD1_MASK)

// keyboard modifiers in an event
#define MOD__SHIFT(event) ((event)->key.state & GDK_SHIFT_MASK)
#define MOD__CTRL(event) ((event)->key.state & INK_GDK_PRIMARY_MASK)
#define MOD__ALT(event) ((event)->key.state & GDK_MOD1_MASK)
#define MOD__SHIFT_ONLY(event) (((event)->key.state & INK_GDK_MODIFIER_MASK) == GDK_SHIFT_MASK)
#define MOD__CTRL_ONLY(event) (((event)->key.state & INK_GDK_MODIFIER_MASK) == INK_GDK_PRIMARY_MASK)
#define MOD__ALT_ONLY(event) (((event)->key.state & INK_GDK_MODIFIER_MASK) == GDK_MOD1_MASK)

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
