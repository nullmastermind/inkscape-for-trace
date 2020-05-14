// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_SPIRAL_CONTEXT_H__
#define __SP_SPIRAL_CONTEXT_H__

/** \file
 * Spiral drawing context
 */
/*
 * Authors:
 *   Mitsuru Oka
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2001-2002 Mitsuru Oka
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/connection.h>
#include <2geom/point.h>
#include "ui/tools/tool-base.h"

#define SP_SPIRAL_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::SpiralTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_SPIRAL_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::SpiralTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

class SPSpiral;

namespace Inkscape {

class Selection;

namespace UI {
namespace Tools {

class SpiralTool : public ToolBase {
public:
	SpiralTool();
	~SpiralTool() override;

	static const std::string prefsPath;

	void setup() override;
	void finish() override;
	void set(const Inkscape::Preferences::Entry& val) override;
	bool root_handler(GdkEvent* event) override;

	const std::string& getPrefsPath() override;

private:
	SPSpiral * spiral;
	Geom::Point center;
	gdouble revo;
	gdouble exp;
	gdouble t0;

    sigc::connection sel_changed_connection;

	void drag(Geom::Point const &p, guint state);
	void finishItem();
	void cancel();
	void selection_changed(Inkscape::Selection *selection);
};

}
}
}

#endif
