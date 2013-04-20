/** @file
 * @brief New node tool with support for multiple path editing
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_UI_TOOL_NODE_TOOL_H
#define SEEN_UI_TOOL_NODE_TOOL_H

#include <memory>
#include <boost/ptr_container/ptr_map.hpp>
#include <glib.h>
#include "event-context.h"

#define INK_NODE_TOOL(obj) ((InkNodeTool*)obj)
#define INK_IS_NODE_TOOL(obj) (dynamic_cast<const InkNodeTool*>((const SPEventContext*)obj))

namespace Inkscape {

namespace Display {
class TemporaryItem;
} // namespace Display
namespace UI {
class MultiPathManipulator;
class ControlPointSelection;
class Selector;
struct PathSharedData;
} // namespace UI
} // namespace Inkscape

//typedef std::auto_ptr<Inkscape::UI::MultiPathManipulator> MultiPathPtr;
//typedef std::auto_ptr<Inkscape::UI::ControlPointSelection> CSelPtr;
//typedef std::auto_ptr<Inkscape::UI::Selector> SelectorPtr;
//typedef std::auto_ptr<Inkscape::UI::PathSharedData> PathSharedDataPtr;


typedef Inkscape::UI::MultiPathManipulator* MultiPathPtr;
typedef Inkscape::UI::ControlPointSelection* CSelPtr;
typedef Inkscape::UI::Selector* SelectorPtr;
typedef Inkscape::UI::PathSharedData* PathSharedDataPtr;

typedef boost::ptr_map<SPItem*, ShapeEditor> ShapeEditors;


class InkNodeTool : public SPEventContext {
public:
	InkNodeTool();
	virtual ~InkNodeTool();

	sigc::connection _selection_changed_connection;
    sigc::connection _mouseover_changed_connection;
    sigc::connection _selection_modified_connection;
    sigc::connection _sizeUpdatedConn;
    Inkscape::MessageContext *_node_message_context;
    SPItem *flashed_item;
    Inkscape::Display::TemporaryItem *flash_tempitem;
    CSelPtr _selected_nodes;
    MultiPathPtr _multipath;
    SelectorPtr _selector;
    PathSharedDataPtr _path_data;
    SPCanvasGroup *_transform_handle_group;
    SPItem *_last_over;
    ShapeEditors _shape_editors;

    unsigned cursor_drag : 1;
    unsigned show_handles : 1;
    unsigned show_outline : 1;
    unsigned live_outline : 1;
    unsigned live_objects : 1;
    unsigned show_path_direction : 1;
    unsigned show_transform_handles : 1;
    unsigned single_node_transform_handles : 1;
    unsigned edit_clipping_paths : 1;
    unsigned edit_masks : 1;

	static const std::string prefsPath;

	virtual void setup();
	virtual void set(Inkscape::Preferences::Entry* val);
	virtual gint root_handler(GdkEvent* event);
	virtual gint item_handler(SPItem* item, GdkEvent* event);

	virtual const std::string& getPrefsPath();
};

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
