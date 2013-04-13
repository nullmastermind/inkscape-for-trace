#ifndef __SP_ZOOM_CONTEXT_H__
#define __SP_ZOOM_CONTEXT_H__

/*
 * Handy zooming tool
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "event-context.h"

#define SP_TYPE_ZOOM_CONTEXT (sp_zoom_context_get_type ())
//#define SP_ZOOM_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_ZOOM_CONTEXT, SPZoomContext))
//#define SP_IS_ZOOM_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_ZOOM_CONTEXT))
#define SP_ZOOM_CONTEXT(obj) ((SPZoomContext*)obj)
#define SP_IS_ZOOM_CONTEXT(obj) (((SPEventContext*)obj)->types.count(typeid(SPZoomContext)))

class CZoomContext;

class SPZoomContext : public SPEventContext {
public:
	SPZoomContext();
	CZoomContext* czoomcontext;

	//SPEventContext event_context;
	SPCanvasItem *grabbed;

	static const std::string prefsPath;
};

struct SPZoomContextClass {
	SPEventContextClass parent_class;
};

class CZoomContext : public CEventContext {
public:
	CZoomContext(SPZoomContext* zoomcontext);

	virtual void setup();
	virtual void finish();
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();
private:
	SPZoomContext* spzoomcontext;
};

GType sp_zoom_context_get_type (void);

#endif
