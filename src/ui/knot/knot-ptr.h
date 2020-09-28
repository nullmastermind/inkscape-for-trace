// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef KNOT_PTR_DETECTOR
#define KNOT_PTR_DETECTOR

void knot_deleted_callback(void* knot);
void knot_created_callback(void* knot);
void check_if_knot_deleted(void* knot);

#endif
