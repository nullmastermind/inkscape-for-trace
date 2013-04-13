#ifndef __SP_FLOOD_CONTEXT_H__
#define __SP_FLOOD_CONTEXT_H__

/*
 * Flood fill drawing context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   John Bintz <jcoswell@coswellproductions.org>
 *
 * Released under GNU GPL
 */

#include <stddef.h>
#include <sigc++/sigc++.h>
#include <gtk/gtk.h>
#include "event-context.h"
#include "helper/unit-menu.h"
#include "helper/units.h"

#define SP_TYPE_FLOOD_CONTEXT            (sp_flood_context_get_type ())
//#define SP_FLOOD_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOOD_CONTEXT, SPFloodContext))
#define SP_FLOOD_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOOD_CONTEXT, SPFloodContextClass))
//#define SP_IS_FLOOD_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOOD_CONTEXT))
#define SP_IS_FLOOD_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOOD_CONTEXT))
#define SP_FLOOD_CONTEXT(obj) ((SPFloodContext*)obj)
#define SP_IS_FLOOD_CONTEXT(obj) (((SPEventContext*)obj)->types.count(typeid(SPFloodContext)))


#define FLOOD_COLOR_CHANNEL_R 1
#define FLOOD_COLOR_CHANNEL_G 2
#define FLOOD_COLOR_CHANNEL_B 4
#define FLOOD_COLOR_CHANNEL_A 8

class CFloodContext;

class SPFloodContext : public SPEventContext {
public:
	SPFloodContext();
	CFloodContext* cfloodcontext;

	SPItem *item;

	sigc::connection sel_changed_connection;

	Inkscape::MessageContext *_message_context;

	static const std::string prefsPath;
};

struct SPFloodContextClass {
	SPEventContextClass parent_class;
};

class CFloodContext : public CEventContext {
public:
	CFloodContext(SPFloodContext* floodcontext);

	virtual void setup();
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();
private:
	SPFloodContext* spfloodcontext;
};

/* Standard Gtk function */

GType sp_flood_context_get_type (void);

GList* flood_channels_dropdown_items_list (void);
GList* flood_autogap_dropdown_items_list (void);
void flood_channels_set_channels( gint channels );

enum PaintBucketChannels {
    FLOOD_CHANNELS_RGB,
    FLOOD_CHANNELS_R,
    FLOOD_CHANNELS_G,
    FLOOD_CHANNELS_B,
    FLOOD_CHANNELS_H,
    FLOOD_CHANNELS_S,
    FLOOD_CHANNELS_L,
    FLOOD_CHANNELS_ALPHA
};

#endif
