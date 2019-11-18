// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Cartesian grid item for the Inkscape canvas.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Copyright (C) Johan Engelen 2006-2007 <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_CANVAS_GRID_H
#define INKSCAPE_CANVAS_GRID_H

#include "sp-canvas-item.h"
#include "ui/widget/alignment-selector.h"
#include "ui/widget/registered-widget.h"
#include "ui/widget/registry.h"
#include "line-snapper.h"

class  SPDesktop;
class SPNamedView;
struct SPCanvasBuf;
class  SPDocument;

namespace Gtk {
	class Widget;
}

namespace Inkscape {
class Snapper;

namespace XML {
class Node;
}

namespace Util {
class Unit;
}

enum GridType {
    GRID_RECTANGULAR = 0,
    GRID_AXONOMETRIC = 1
};
#define GRID_MAXTYPENR 1

#define INKSCAPE_TYPE_GRID_CANVASITEM            (Inkscape::grid_canvasitem_get_type ())
#define INKSCAPE_GRID_CANVASITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INKSCAPE_TYPE_GRID_CANVASITEM, GridCanvasItem))
#define INKSCAPE_GRID_CANVASITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INKSCAPE_TYPE_GRID_CANVASITEM, GridCanvasItem))
#define INKSCAPE_IS_GRID_CANVASITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INKSCAPE_TYPE_GRID_CANVASITEM))
#define INKSCAPE_IS_GRID_CANVASITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INKSCAPE_TYPE_GRID_CANVASITEM))

#define GRID_DEFAULT_COLOR 0x3F3FFF20
#define GRID_DEFAULT_EMPCOLOR 0x3F3FFF40

class CanvasGrid;

/** All the variables that are tracked for a grid specific canvas item. */
struct GridCanvasItem : public SPCanvasItem{
    CanvasGrid *grid; // the owning grid object
};

struct GridCanvasItemClass {
    SPCanvasItemClass parent_class;
};

/* Standard Gtk function */
GType grid_canvasitem_get_type ();



class CanvasGrid {
public:
    virtual ~CanvasGrid();

    // TODO: see effect.h and effect.cpp from live_effects how to link enums to SVGname to typename properly. (johan)
    const char * getName() const;
    const char * getSVGName() const;
    GridType     getGridType() const;
    static const char * getName(GridType type);
    static const char * getSVGName(GridType type);
    static GridType     getGridTypeFromSVGName(const char * typestr);
    static GridType     getGridTypeFromName(const char * typestr);

    static CanvasGrid* NewGrid(SPNamedView * nv, Inkscape::XML::Node * repr, SPDocument *doc, GridType gridtype);
    static void writeNewGridToRepr(Inkscape::XML::Node * repr, SPDocument * doc, GridType gridtype);

    GridCanvasItem * createCanvasItem(SPDesktop * desktop);

    virtual void Update (Geom::Affine const &affine, unsigned int flags) = 0;
    virtual void Render (SPCanvasBuf *buf) = 0;

    virtual void readRepr() = 0;
    virtual void onReprAttrChanged (Inkscape::XML::Node * /*repr*/, char const */*key*/, char const */*oldval*/, char const */*newval*/, bool /*is_interactive*/) = 0;

    Gtk::Widget * newWidget();

    void setOrigin(Geom::Point const &origin_px); /**< writes new origin (specified in px units) to SVG */
    Geom::Point origin;     /**< Origin of the grid */

    guint32 color;        /**< Color for normal lines */
    guint32 empcolor;     /**< Color for emphasis lines */
    gint empspacing;      /**< Spacing between emphasis lines */

    Inkscape::Util::Unit const* gridunit;  /**< points to Unit object in UnitTable (so don't delete it) */

    Inkscape::XML::Node * repr;
    SPDocument *doc;

    Inkscape::Snapper* snapper;

    static void on_repr_attr_changed (Inkscape::XML::Node * repr, const gchar *key, const gchar *oldval, const gchar *newval, bool is_interactive, void * data);

    bool isLegacy() const { return legacy; }
    bool isPixel() const { return pixel; }

    bool isVisible() const { return (isEnabled() &&visible); };
    bool isEnabled() const;

    void align_clicked(int align);

protected:
    CanvasGrid(SPNamedView * nv, Inkscape::XML::Node * in_repr, SPDocument *in_doc, GridType type);

    virtual Gtk::Widget * newSpecificWidget() = 0;

    std::vector<SPCanvasItem*> canvasitems;  // list of created canvasitems

    SPNamedView * namedview;

    Inkscape::UI::Widget::Registry _wr;
    bool visible;
    bool render_dotted;

    GridType gridtype;

    // For dealing with old Inkscape SVG files (pre 0.92)
    bool legacy;
    bool pixel;

    Inkscape::UI::Widget::RegisteredCheckButton *_rcb_enabled = nullptr;
    Inkscape::UI::Widget::RegisteredCheckButton *_rcb_snap_visible_only = nullptr;
    Inkscape::UI::Widget::RegisteredCheckButton *_rcb_visible = nullptr;
    Inkscape::UI::Widget::RegisteredCheckButton *_rcb_dotted = nullptr;
    Inkscape::UI::Widget::AlignmentSelector     *_as_alignment = nullptr;

private:
    CanvasGrid(const CanvasGrid&) = delete;
    CanvasGrid& operator=(const CanvasGrid&) = delete;
};


class CanvasXYGrid : public CanvasGrid {
public:
    CanvasXYGrid(SPNamedView * nv, Inkscape::XML::Node * in_repr, SPDocument * in_doc);
    ~CanvasXYGrid() override;

    virtual void Scale  (Geom::Scale const &scale);
    void Update (Geom::Affine const &affine, unsigned int flags) override;
    void Render (SPCanvasBuf *buf) override;

    void readRepr() override;
    void onReprAttrChanged (Inkscape::XML::Node * repr, char const *key, char const *oldval, char const *newval, bool is_interactive) override;

    Geom::Point spacing; /**< Spacing between elements of the grid */
    bool scaled[2];    /**< Whether the grid is in scaled mode, which can
                            be different in the X or Y direction, hence two
                            variables */
    Geom::Point ow;      /**< Transformed origin by the affine for the zoom */
    Geom::Point sw[2];   /**< Transformed spacing by the affine for the zoom */

protected:
    Gtk::Widget * newSpecificWidget() override;

private:
    CanvasXYGrid(const CanvasXYGrid&) = delete;
    CanvasXYGrid& operator=(const CanvasXYGrid&) = delete;

    void updateWidgets();

    Inkscape::UI::Widget::RegisteredUnitMenu *_rumg = nullptr;
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_ox = nullptr;
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_oy = nullptr;
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_sx = nullptr;
    Inkscape::UI::Widget::RegisteredScalarUnit *_rsu_sy = nullptr;
    Inkscape::UI::Widget::RegisteredColorPicker *_rcp_gcol = nullptr;
    Inkscape::UI::Widget::RegisteredColorPicker *_rcp_gmcol = nullptr;
    Inkscape::UI::Widget::RegisteredSuffixedInteger *_rsi = nullptr;
};



class CanvasXYGridSnapper : public LineSnapper
{
public:
    CanvasXYGridSnapper(CanvasXYGrid *grid, SnapManager *sm, Geom::Coord const d);
    bool ThisSnapperMightSnap() const override;

    Geom::Coord getSnapperTolerance() const override; //returns the tolerance of the snapper in screen pixels (i.e. independent of zoom)
    bool getSnapperAlwaysSnap() const override; //if true, then the snapper will always snap, regardless of its tolerance

private:
    LineList _getSnapLines(Geom::Point const &p) const override;
    void _addSnappedLine(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance,  SnapSourceType const &source, long source_num, Geom::Point const &normal_to_line, const Geom::Point &point_on_line) const override;
    void _addSnappedPoint(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const override;
    void _addSnappedLinePerpendicularly(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const override;
    CanvasXYGrid *grid;
};

}; /* namespace Inkscape */




#endif
