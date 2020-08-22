// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"

#define SP_ZOOM_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::ZoomTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_ZOOM_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::ZoomTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
namespace UI {
namespace Tools {

class ZoomTool : public ToolBase {
public:
	ZoomTool();
	~ZoomTool() override;

	static const std::string prefsPath;

	void setup() override;
	void finish() override;
	bool root_handler(GdkEvent* event) override;

	const std::string& getPrefsPath() override;

private:
	bool escaped;
};

}
}
}

#endif
