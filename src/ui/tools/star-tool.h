// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_STAR_CONTEXT_H__
#define __SP_STAR_CONTEXT_H__

/*
 * Star drawing context
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Mitsuru Oka
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstddef>
#include <sigc++/sigc++.h>
#include <2geom/point.h>
#include "ui/tools/tool-base.h"

class SPStar;

namespace Inkscape {
namespace UI {
namespace Tools {

class StarTool : public ToolBase {
public:
	StarTool();
	~StarTool() override;

	static const std::string prefsPath;

	void setup() override;
	void finish() override;
	void set(const Inkscape::Preferences::Entry& val) override;
	bool root_handler(GdkEvent* event) override;

	const std::string& getPrefsPath() override;

private:
	SPStar* star;

	Geom::Point center;

    /* Number of corners */
    gint magnitude;

    /* Outer/inner radius ratio */
    gdouble proportion;

    /* flat sides or not? */
    bool isflatsided;

    /* rounded corners ratio */
    gdouble rounded;

    // randomization
    gdouble randomized;

    sigc::connection sel_changed_connection;

	void drag(Geom::Point p, guint state);
	void finishItem();
	void cancel();
	void selection_changed(Inkscape::Selection* selection);
};

}
}
}

#endif
