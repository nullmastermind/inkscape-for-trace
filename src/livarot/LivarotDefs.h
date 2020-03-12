// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * Fred
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef my_defs
#define my_defs

#include <cstdint>

// error codes (mostly obsolete)
enum
{
  avl_no_err = 0,		// 0 is the error code for "everything OK"
  avl_bal_err = 1,
  avl_rm_err = 2,
  avl_ins_err = 3,
  shape_euler_err = 4,		// computations result in a non-eulerian graph, thus the function cannot do a proper polygon
  // despite the rounding sheme, this still happen with uber-complex graphs
  // note that coordinates are stored in double => double precision for the computation is not even
  // enough to get exact results (need quadruple precision, i think).
  shape_input_err = 5,		// the function was given an incorrect input (not a polygon, or not eulerian)
  shape_nothing_to_do = 6		// the function had nothing to do (zero offset, etc)
};

// return codes for the find function in the AVL tree (private)
enum
{
  not_found = 0,
  found_exact = 1,
  found_on_left = 2,
  found_on_right = 3,
  found_between = 4
};

// types of cap for stroking polylines
enum butt_typ
{
  butt_straight,		// straight line
  butt_square,			// half square
  butt_round,			// half circle
  butt_pointy			// a little pointy hat
};
// types of joins for stroking paths
enum join_typ
{
  join_straight,		// a straight line
  join_round,			// arc of circle (in fact, one or two quadratic bezier curve chunks)
  join_pointy			// a miter join (uses the miter parameter)
};
typedef enum butt_typ ButtType;
typedef enum join_typ JoinType;

enum fill_typ
{
  fill_oddEven   = 0,
  fill_nonZero   = 1,
  fill_positive  = 2,
  fill_justDont = 3
};
typedef enum fill_typ FillRule;

// info for a run of pixel to fill
struct raster_info {
		int       startPix,endPix;  // start and end pixel from the polygon POV
		int       sth,stv;          // coordinates for the first pixel in the run, in (possibly another) POV
		uint32_t* buffer;           // pointer to the first pixel in the run
};
typedef void (*RasterInRunFunc) (raster_info &dest,void *data,int nst,float vst,int nen,float ven);	// init for position ph,pv; the last parameter is a pointer


enum Side {
    LEFT = 0,
    RIGHT = 1
};

enum FirstOrLast {
    FIRST = 0,
    LAST = 1
};

#endif
