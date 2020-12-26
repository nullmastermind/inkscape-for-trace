// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New node tool with support for multiple path editing
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_TOOL_NODE_TOOL_H
#define SEEN_UI_TOOL_NODE_TOOL_H

#include <glib.h>
#include "ui/tools/tool-base.h"

// we need it to call it from Live Effect
#include "selection.h"

namespace Inkscape {
    namespace Display {
        class TemporaryItem;
    }

    namespace UI {
        class MultiPathManipulator;
        class ControlPointSelection;
        class Selector;
        class ControlPoint;

        struct PathSharedData;
    }
}

struct SPCanvasGroup;

#define INK_NODE_TOOL(obj) (dynamic_cast<Inkscape::UI::Tools::NodeTool*>((Inkscape::UI::Tools::ToolBase*)obj))
#define INK_IS_NODE_TOOL(obj) (dynamic_cast<const Inkscape::UI::Tools::NodeTool*>((const Inkscape::UI::Tools::ToolBase*)obj))

namespace Inkscape {
namespace UI {
namespace Tools {

class NodeTool : public ToolBase {
public:
    NodeTool();
    ~NodeTool() override;

    Inkscape::UI::ControlPointSelection* _selected_nodes = nullptr;
    Inkscape::UI::MultiPathManipulator* _multipath = nullptr;
    std::vector<Inkscape::Display::TemporaryItem *> _helperpath_tmpitem;

    bool edit_clipping_paths = false;
    bool edit_masks = false;

    static const std::string prefsPath;

    void setup() override;
    void finish() override;
    void set(const Inkscape::Preferences::Entry& val) override;
    bool root_handler(GdkEvent* event) override;

    const std::string& getPrefsPath() override;
    std::map<SPItem *, std::unique_ptr<ShapeEditor>> _shape_editors;

private:
    sigc::connection _selection_changed_connection;
    sigc::connection _mouseover_changed_connection;

    SPItem *flashed_item = nullptr;

    Inkscape::Display::TemporaryItem *flash_tempitem = nullptr;
    Inkscape::UI::Selector* _selector = nullptr;
    Inkscape::UI::PathSharedData* _path_data = nullptr;
    Inkscape::CanvasItemGroup *_transform_handle_group = nullptr;
    SPItem *_last_over = nullptr;

    bool cursor_drag = false;
    bool show_handles = false;
    bool show_outline =false;
    bool live_outline = false;
    bool live_objects = false;
    bool show_path_direction = false;
    bool show_transform_handles = false;
    bool single_node_transform_handles = false;

    std::vector<SPItem*> _current_selection;
    std::vector<SPItem*> _previous_selection;

    void selection_changed(Inkscape::Selection *sel);

    void select_area(Geom::Rect const &sel, GdkEventButton *event);
    void select_point(Geom::Point const &sel, GdkEventButton *event);
    void mouseover_changed(Inkscape::UI::ControlPoint *p);
    void update_tip(GdkEvent *event);
    void handleControlUiStyleChange();
};
void sp_update_helperpath(SPDesktop *desktop);
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
