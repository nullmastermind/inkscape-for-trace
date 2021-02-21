// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * List of paper sizes
 */
/*
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon Phillips <jon@rejon.org>
 *   Ralf Stephan <ralf@ark.in-berlin.de> (Gtkmm)
 *   Bob Jamison <ishmal@users.sf.net>
 *   Abhishek Sharma
 *   + see git history
 *
 * Copyright (C) 2000 - 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_PAGES_SKELETON_H
#define SEEN_PAGES_SKELETON_H


    /** \note
     * The ISO page sizes in the table below differ from ghostscript's idea of page sizes (by
     * less than 1pt).  Being off by <1pt should be OK for most purposes, but may cause fuzziness
     * (antialiasing) problems when printing to 72dpi or 144dpi printers or bitmap files due to
     * postscript's different coordinate system (y=0 meaning bottom of page in postscript and top
     * of page in SVG).  I haven't looked into whether this does in fact cause fuzziness, I merely
     * note the possibility.  Rounding done by extension/internal/ps.cpp (e.g. floor/ceil calls)
     * will also affect whether fuzziness occurs.
     *
     * The remainder of this comment discusses the origin of the numbers used for ISO page sizes in
     * this table and in ghostscript.
     *
     * The versions here, in mm, are the official sizes according to
     * <a href="http://en.wikipedia.org/wiki/Paper_sizes">http://en.wikipedia.org/wiki/Paper_sizes</a>
     * at 2005-01-25.  (The ISO entries in the below table
     * were produced mechanically from the table on that page.)
     *
     * (The rule seems to be that A0, B0, ..., D0. sizes are rounded to the nearest number of mm
     * from the "theoretical size" (i.e. 1000 * sqrt(2) or pow(2.0, .25) or the like), whereas
     * going from e.g. A0 to A1 always take the floor of halving -- which by chance coincides
     * exactly with flooring the "theoretical size" for n != 0 instead of the rounding to nearest
     * done for n==0.)
     *
     * Ghostscript paper sizes are given in gs_statd.ps according to gs(1).  gs_statd.ps always
     * uses an integer number ofpt: sometimes gs_statd.ps rounds to nearest (e.g. a1), sometimes
     * floors (e.g. a10), sometimes ceils (e.g. a8).
     *
     * I'm not sure how ghostscript's gs_statd.ps was calculated: it isn't just rounding the
     * "theoretical size" of each page topt (see a0), nor is it rounding the a0 size times an
     * appropriate power of two (see a1).  Possibly it was prepared manually, with a human applying
     * inconsistent rounding rules when converting from mm to pt.
     */
    /** \todo
     * Should we include the JIS B series (used in Japan)
     * (JIS B0 is sometimes called JB0, and similarly for JB1 etc)?
     * Should we exclude B7--B10 and A7--10 to make the list smaller ?
     * Should we include any of the ISO C, D and E series (see below) ?
     */



    /* See http://www.hbp.com/content/PCR_envelopes.cfm for a much larger list of US envelope
       sizes. */
    /* Note that `Folio' (used in QPrinter/KPrinter) is deliberately absent from this list, as it
       means different sizes to different people: different people may expect the width to be
       either 8, 8.25 or 8.5 inches, and the height to be either 13 or 13.5 inches, even
       restricting our interpretation to foolscap folio.  If you wish to introduce a folio-like
       page size to the list, then please consider using a name more specific than just `Folio' or
       `Foolscap Folio'. */

static char const pages_skeleton[] = R"(#Inkscape page sizes
#NAME,                    WIDTH, HEIGHT, UNIT
A4,                         210,    297, mm
US Letter,                  8.5,     11, in
US Legal,                   8.5,     14, in
US Executive,              7.25,   10.5, in
US Ledger/Tabloid,           11,     17, in
A0,                         841,   1189, mm
A1,                         594,    841, mm
A2,                         420,    594, mm
A3,                         297,    420, mm
A5,                         148,    210, mm
A6,                         105,    148, mm
A7,                          74,    105, mm
A8,                          52,     74, mm
A9,                          37,     52, mm
A10,                         26,     37, mm
B0,                        1000,   1414, mm
B1,                         707,   1000, mm
B2,                         500,    707, mm
B3,                         353,    500, mm
B4,                         250,    353, mm
B5,                         176,    250, mm
B6,                         125,    176, mm
B7,                          88,    125, mm
B8,                          62,     88, mm
B9,                          44,     62, mm
B10,                         31,     44, mm
C0,                         917,   1297, mm
C1,                         648,    917, mm
C2,                         458,    648, mm
C3,                         324,    458, mm
C4,                         229,    324, mm
C5,                         162,    229, mm
C6,                         114,    162, mm
C7,                          81,    114, mm
C8,                          57,     81, mm
C9,                          40,     57, mm
C10,                         28,     40, mm
D1,                         545,    771, mm
D2,                         385,    545, mm
D3,                         272,    385, mm
D4,                         192,    272, mm
D5,                         136,    192, mm
D6,                          96,    136, mm
D7,                          68,     96, mm
E3,                         400,    560, mm
E4,                         280,    400, mm
E5,                         200,    280, mm
E6,                         140,    200, mm
CSE,                        462,    649, pt
US #10 Envelope,            9.5,  4.125, in
DL Envelope,                220,    110, mm
Banner 468x60,              468,     60, px
Icon 16x16,                  16,     16, px
Icon 32x32,                  32,     32, px
Icon 48x48,                  48,     48, px
ID Card (ISO 7810),       85.60,  53.98, mm
Business Card (US),         3.5,      2, in
Business Card (Europe),      85,     55, mm
Business Card (AU/NZ),       90,     55, mm
Arch A,                       9,     12, in
Arch B,                      12,     18, in
Arch C,                      18,     24, in
Arch D,                      24,     36, in
Arch E,                      36,     48, in
Arch E1,                     30,     42, in
Video SD / PAL,             768,    576, px
Video SD-Widescreen / PAL, 1024,    576, px
Video SD / NTSC,            544,    480, px
Video SD-Widescreen / NTSC, 872,    486, px
Video HD 720p,             1280,    720, px
Video HD 1080p,            1920,   1080, px
Video DCI 2k (Full Frame), 2048,   1080, px
Video UHD 4k,              3840,   2160, px
Video DCI 4k (Full Frame), 4096,   2160, px
Video UHD 8k,              7680,   4320, px
)";

#endif
