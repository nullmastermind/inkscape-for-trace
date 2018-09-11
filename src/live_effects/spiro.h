// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * C implementation of third-order polynomial spirals.
 *//*
 * Authors: see git history
 * Raph Levien
 * Johan Engelen
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SPIRO_H
#define INKSCAPE_SPIRO_H

#include "live_effects/spiro-converters.h"
#include <2geom/forward.h>

class SPCurve;

namespace Spiro {

struct spiro_cp {
    double x;
    double y;
    char ty;
};


void spiro_run(const spiro_cp *src, int src_len, SPCurve &curve);
void spiro_run(const spiro_cp *src, int src_len, Geom::Path &path);

/* the following methods are only for expert use: */
typedef struct spiro_seg_s spiro_seg;
spiro_seg * run_spiro(const spiro_cp *src, int n);
void free_spiro(spiro_seg *s);
void spiro_to_otherpath(const spiro_seg *s, int n, ConverterBase &bc);
double get_knot_th(const spiro_seg *s, int i);


} // namespace Spiro

#endif // INKSCAPE_SPIRO_H
