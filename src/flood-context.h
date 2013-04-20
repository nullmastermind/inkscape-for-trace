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

#define SP_FLOOD_CONTEXT(obj) ((SPFloodContext*)obj)
#define SP_IS_FLOOD_CONTEXT(obj) (dynamic_cast<const SPFloodContext*>((const SPEventContext*)obj))


#define FLOOD_COLOR_CHANNEL_R 1
#define FLOOD_COLOR_CHANNEL_G 2
#define FLOOD_COLOR_CHANNEL_B 4
#define FLOOD_COLOR_CHANNEL_A 8

class SPFloodContext : public SPEventContext {
public:
	SPFloodContext();
	virtual ~SPFloodContext();

	SPItem *item;

	sigc::connection sel_changed_connection;

	Inkscape::MessageContext *_message_context;

	static const std::string prefsPath;

	virtual void setup();
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();
};

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
