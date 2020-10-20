// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SP_SELECT_CONTEXT_H__
#define __SP_SELECT_CONTEXT_H__

/*
 * Select tool
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tools/tool-base.h"

#define SP_SELECT_CONTEXT(obj) (dynamic_cast<Inkscape::UI::Tools::SelectTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define SP_IS_SELECT_CONTEXT(obj) (dynamic_cast<const Inkscape::UI::Tools::SelectTool*>((const Inkscape::UI::Tools::ToolBase*)obj) != NULL)

namespace Inkscape {
  class SelTrans;
  class SelectionDescriber;
}

namespace Inkscape {
namespace UI {
namespace Tools {

class SelectTool : public ToolBase {
public:
	SelectTool();
	~SelectTool() override;

	bool dragging;
	bool moved;
	guint button_press_state;

        std::vector<SPItem *> cycling_items;
        std::vector<SPItem *> cycling_items_cmp;
        SPItem *cycling_cur_item;
	bool cycling_wrap;

	SPItem *item;
        Inkscape::CanvasItem *grabbed = nullptr;
	Inkscape::SelTrans *_seltrans;
	Inkscape::SelectionDescriber *_describer;
	gchar *no_selection_msg = nullptr;

	static const std::string prefsPath;

	void setup() override;
	void set(const Inkscape::Preferences::Entry& val) override;
	bool root_handler(GdkEvent* event) override;
	bool item_handler(SPItem* item, GdkEvent* event) override;

	const std::string& getPrefsPath() override;

private:
	bool sp_select_context_abort();
	void sp_select_context_cycle_through_items(Inkscape::Selection *selection, GdkEventScroll *scroll_event);
	void sp_select_context_reset_opacities();

    Glib::RefPtr<Gdk::Cursor> _cursor_mouseover;
    Glib::RefPtr<Gdk::Cursor> _cursor_dragging;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
