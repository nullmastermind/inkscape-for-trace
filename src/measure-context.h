#ifndef SEEN_SP_MEASURING_CONTEXT_H
#define SEEN_SP_MEASURING_CONTEXT_H

/*
 * Our fine measuring tool
 *
 * Authors:
 *   Felipe Correa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2011 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "event-context.h"

#define SP_MEASURE_CONTEXT(obj) ((SPMeasureContext*)obj)
#define SP_IS_MEASURE_CONTEXT(obj) (dynamic_cast<const SPMeasureContext*>((const SPEventContext*)obj))

class SPMeasureContext : public SPEventContext {
public:
	SPMeasureContext();
	virtual ~SPMeasureContext();

	//SPEventContext event_context;
	SPCanvasItem *grabbed;

	static const std::string prefsPath;

	virtual void setup();
	virtual void finish();
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();
};

#endif // SEEN_SP_MEASURING_CONTEXT_H
