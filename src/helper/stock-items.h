// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * John Cliff <simarilius@yahoo.com>
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INK_STOCK_ITEMS_H
#define SEEN_INK_STOCK_ITEMS_H

#include <glib.h>

class SPObject;

SPObject *get_stock_item(gchar const *urn, gboolean stock=FALSE);

#endif // SEEN_INK_STOCK_ITEMS_H
