// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef __GRAYMAP_GDK_H__
#define __GRAYMAP_GDK_H__

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "imagemap.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

/*#########################################################################
### I M A G E    M A P --- GDK
#########################################################################*/



#ifdef __cplusplus
extern "C" {
#endif

GrayMap *gdkPixbufToGrayMap(GdkPixbuf *buf);
GdkPixbuf *grayMapToGdkPixbuf(GrayMap *grayMap);
RgbMap *gdkPixbufToRgbMap(GdkPixbuf *buf);
GdkPixbuf *indexedMapToGdkPixbuf(IndexedMap *iMap);


#ifdef __cplusplus
}
#endif


#endif /* __GRAYMAP_GDK_H__ */

/*#########################################################################
### E N D    O F    F I L E
#########################################################################*/
