#ifndef __SP_DROPPER_CONTEXT_H__
#define __SP_DROPPER_CONTEXT_H__

/*
 * Tool for picking colors from drawing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "event-context.h"

#define SP_DROPPER_CONTEXT(obj) ((SPDropperContext*)obj)
#define SP_IS_DROPPER_CONTEXT(obj) (dynamic_cast<const SPDropperContext*>((const SPEventContext*)obj))

enum {
      SP_DROPPER_PICK_VISIBLE,
      SP_DROPPER_PICK_ACTUAL  
};

class SPDropperContext : public SPEventContext {
public:
	SPDropperContext();
	virtual ~SPDropperContext();

    //SPEventContext event_context;

	static const std::string prefsPath;

	virtual void setup();
	virtual void finish();
	virtual gint root_handler(GdkEvent* event);

	virtual const std::string& getPrefsPath();

//private:
    double        R;
    double        G;
    double        B;
    double        alpha;

    unsigned int  dragging : 1;

    SPCanvasItem *grabbed;
    SPCanvasItem *area;
    Geom::Point   centre;
};

guint32 sp_dropper_context_get_color(SPEventContext *ec);

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
