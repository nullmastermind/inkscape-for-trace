// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * \brief Remove overlaps function
 */
/*
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_REMOVEOVERLAP_H
#define SEEN_REMOVEOVERLAP_H

#include <vector>

class SPItem;

void removeoverlap(std::vector<SPItem*> const &items, double xGap, double yGap);

#endif // SEEN_REMOVEOVERLAP_H
