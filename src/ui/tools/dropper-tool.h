// SPDX-License-Identifier: GPL-2.0-or-later
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
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <2geom/point.h>

#include "color-rgba.h"
#include "ui/tools/tool-base.h"

struct SPCanvasItem;

#define SP_DROPPER_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::DropperTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_DROPPER_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::DropperTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

enum {
      SP_DROPPER_PICK_VISIBLE,
      SP_DROPPER_PICK_ACTUAL  
};
enum {
  DONT_REDRAW_CURSOR,
  DRAW_FILL_CURSOR,
  DRAW_STROKE_CURSOR
};

namespace Inkscape {

class CanvasItemBpath;

namespace UI {
namespace Tools {

class DropperTool : public ToolBase {
public:
	DropperTool();
	~DropperTool() override;

	static const std::string prefsPath;

	const std::string& getPrefsPath() override;

	guint32 get_color(bool invert=false, bool non_dropping=false);

        sigc::signal<void, ColorRGBA *> onetimepick_signal;

protected:
	void setup() override;
	void finish() override;
	bool root_handler(GdkEvent* event) override;

private:
    // Stored color.
    double R = 0.0;
    double G = 0.0;
    double B = 0.0;
    double alpha = 0.0;
    // Stored color taken from canvas. Used by clipboard.
    // Identical to R, G, B, alpha if dropping disabled.
    double non_dropping_R = 0.0;
    double non_dropping_G = 0.0;
    double non_dropping_B = 0.0;
    double non_dropping_A = 0.0;

    bool invert = false;   ///< Set color to inverse rgb value
    bool stroke = false;   ///< Set to stroke color. In dropping mode, set from stroke color
    bool dropping = false; ///< When true, get color from selected objects instead of canvas
    bool dragging = false; ///< When true, get average color for region on canvas, instead of a single point

    double radius = 0.0;                       ///< Size of region under dragging mode
    Inkscape::CanvasItemBpath* area = nullptr; ///< Circle depicting region's borders in dragging mode
    Geom::Point centre {0, 0};                 ///< Center of region in dragging mode

};

}
}
}

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
