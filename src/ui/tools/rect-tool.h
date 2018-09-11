// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_RECT_CONTEXT_H__
#define __SP_RECT_CONTEXT_H__

/*
 * Rectangle drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstddef>
#include <sigc++/sigc++.h>
#include <2geom/point.h>
#include "ui/tools/tool-base.h"

class SPRect;

namespace Inkscape {
namespace UI {
namespace Tools {

class RectTool : public ToolBase {
public:
	RectTool();
	~RectTool() override;

	static const std::string prefsPath;

	void setup() override;
	void finish() override;
	void set(const Inkscape::Preferences::Entry& val) override;
	bool root_handler(GdkEvent* event) override;
	bool item_handler(SPItem* item, GdkEvent* event) override;

	const std::string& getPrefsPath() override;

private:
	SPRect *rect;
	Geom::Point center;

  	gdouble rx;	/* roundness radius (x direction) */
  	gdouble ry;	/* roundness radius (y direction) */

	sigc::connection sel_changed_connection;

	void drag(Geom::Point const pt, guint state);
	void finishItem();
	void cancel();
	void selection_changed(Inkscape::Selection* selection);
};

}
}
}

#endif
