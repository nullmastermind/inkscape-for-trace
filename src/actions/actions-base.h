// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_BASE_H
#define INK_ACTIONS_BASE_H

template<class T> class ConcreteInkscapeApplication;

template<class T>
void add_actions_base(ConcreteInkscapeApplication<T>* app);

#endif // INK_ACTIONS_BASE_H
