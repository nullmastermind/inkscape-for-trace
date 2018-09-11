// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_FLOOD_CONTEXT_H__
#define __SP_FLOOD_CONTEXT_H__

/*
 * Flood fill drawing context
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   John Bintz <jcoswell@coswellproductions.org>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include <sigc++/connection.h>

#include "ui/tools/tool-base.h"

#define SP_FLOOD_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::FloodTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_FLOOD_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::FloodTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {

class Selection;

namespace UI {
namespace Tools {

class FloodTool : public ToolBase {
public:
	FloodTool();
	~FloodTool() override;

	SPItem *item;

	sigc::connection sel_changed_connection;

	static const std::string prefsPath;

	void setup() override;
	bool root_handler(GdkEvent* event) override;
	bool item_handler(SPItem* item, GdkEvent* event) override;

	const std::string& getPrefsPath() override;

	static void set_channels(gint channels);
	static const std::vector<Glib::ustring> channel_list;
	static const std::vector<Glib::ustring> gap_list;

private:
	void selection_changed(Inkscape::Selection* selection);
	void finishItem();
};

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

}
}
}

#endif
