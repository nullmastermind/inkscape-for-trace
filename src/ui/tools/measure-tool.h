// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MEASURING_CONTEXT_H
#define SEEN_SP_MEASURING_CONTEXT_H

/*
 * Our fine measuring tool
 *
 * Authors:
 *   Felipe Correa da Silva Sanches <juca@members.fsf.org>
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 * Copyright (C) 2011 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstddef>
#include <boost/optional.hpp>
#include <optional>

#include <sigc++/sigc++.h>

#include <2geom/point.h>

#include "ui/tools/tool-base.h"

#include "display/control/canvas-temporary-item.h"
#include "display/control/canvas-item-enums.h"

class SPKnot;

namespace Inkscape {

class CanvasItemCurve;

namespace UI {
namespace Tools {

class MeasureTool : public ToolBase {
public:
    MeasureTool();
    ~MeasureTool() override;

    static const std::string prefsPath;

    void finish() override;
    bool root_handler(GdkEvent* event) override;
    virtual void showCanvasItems(bool to_guides = false, bool to_item = false, bool to_phantom = false, Inkscape::XML::Node *measure_repr = nullptr);
    virtual void reverseKnots();
    virtual void toGuides();
    virtual void toPhantom();
    virtual void toMarkDimension();
    virtual void toItem();
    virtual void reset();
    virtual void setMarkers();
    virtual void setMarker(bool isStart);
    const std::string& getPrefsPath() override;
    Geom::Point readMeasurePoint(bool is_start);

    void showInfoBox(Geom::Point cursor, bool into_groups);
    void showItemInfoText(Geom::Point pos, Glib::ustring const &measure_str, double fontsize);
    void writeMeasurePoint(Geom::Point point, bool is_start);
    void setGuide(Geom::Point origin, double angle, const char *label);
    void setPoint(Geom::Point origin, Inkscape::XML::Node *measure_repr);
    void setLine(Geom::Point start_point,Geom::Point end_point, bool markers, guint32 color,
                 Inkscape::XML::Node *measure_repr = nullptr);
    void setMeasureCanvasText(bool is_angle, double precision, double amount, double fontsize,
                              Glib::ustring unit_name, Geom::Point position, guint32 background,
                              Inkscape::CanvasItemTextAnchor text_anchor, bool to_item, bool to_phantom,
                              Inkscape::XML::Node *measure_repr);
    void setMeasureCanvasItem(Geom::Point position, bool to_item, bool to_phantom,
                              Inkscape::XML::Node *measure_repr);
    void setMeasureCanvasControlLine(Geom::Point start, Geom::Point end, bool to_item, bool to_phantom,
                                     Inkscape::CanvasItemColor color, Inkscape::XML::Node *measure_repr);
    void setLabelText(Glib::ustring const &value, Geom::Point pos, double fontsize, Geom::Coord angle,
                      guint32 background,
                      Inkscape::XML::Node *measure_repr = nullptr,
                      Inkscape::CanvasItemTextAnchor text_anchor = Inkscape::CANVAS_ITEM_TEXT_ANCHOR_CENTER);

    void knotStartMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state);
    void knotEndMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state);
    void knotClickHandler(SPKnot *knot, guint state);
    void knotUngrabbedHandler(SPKnot */*knot*/,  unsigned int /*state*/);

private:
    std::optional<Geom::Point> explicit_base;
    std::optional<Geom::Point> last_end;
    SPKnot *knot_start = nullptr;
    SPKnot *knot_end   = nullptr;
    gint dimension_offset = 20;
    Geom::Point start_p;
    Geom::Point end_p;
    Geom::Point last_pos;

    std::vector<Inkscape::CanvasItem *> measure_tmp_items;
    std::vector<Inkscape::CanvasItem *> measure_phantom_items;
    std::vector<Inkscape::CanvasItem *> measure_item;

    double item_width;
    double item_height;
    double item_x;
    double item_y;
    double item_length;
    SPItem *over;
    sigc::connection _knot_start_moved_connection;
    sigc::connection _knot_start_ungrabbed_connection;
    sigc::connection _knot_start_click_connection;
    sigc::connection _knot_end_moved_connection;
    sigc::connection _knot_end_click_connection;
    sigc::connection _knot_end_ungrabbed_connection;
};

}
}
}

#endif // SEEN_SP_MEASURING_CONTEXT_H

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
