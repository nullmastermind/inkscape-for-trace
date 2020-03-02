// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path offsets.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef PATH_OFFSET_H
#define PATH_OFFSET_H

class SPDesktop;

// offset/inset of a curve
// takes the fill-rule in consideration
// offset amount is the stroke-width of the curve
void sp_selected_path_offset (SPDesktop *desktop);
void sp_selected_path_offset_screen (SPDesktop *desktop, double pixels);
void sp_selected_path_inset (SPDesktop *desktop);
void sp_selected_path_inset_screen (SPDesktop *desktop, double pixels);
void sp_selected_path_create_offset (SPDesktop *desktop);
void sp_selected_path_create_inset (SPDesktop *desktop);
void sp_selected_path_create_updating_offset (SPDesktop *desktop);
void sp_selected_path_create_updating_inset (SPDesktop *desktop);

void sp_selected_path_create_offset_object_zero (SPDesktop *desktop);
void sp_selected_path_create_updating_offset_object_zero (SPDesktop *desktop);

#endif // PATH_OFFSET_H

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
