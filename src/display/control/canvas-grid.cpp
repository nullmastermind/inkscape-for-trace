// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Cartesian grid item for the Inkscape canvas.
 *//*
 * Authors:
 * see git history
 * Copyright (C) Johan Engelen 2006-2007 <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 * Copyright (C) Tavmong Bah 2017 <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

/* As a general comment, I am not exactly proud of how things are done.
 * (for example the 'enable' widget and readRepr things)
 * It does seem to work however. I intend to clean up and sort things out later, but that can take forever...
 * Don't be shy to correct things.
 */

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/grid.h>

#include <glibmm/i18n.h>

#include "canvas-grid.h"
#include "canvas-axonomgrid.h"
#include "canvas-item-grid.h"

#include "desktop.h"
#include "document.h"
#include "inkscape.h"
#include "preferences.h"
#include "verbs.h"

#include "display/cairo-utils.h"

#include "helper/mathfns.h"

#include "object/sp-namedview.h"
#include "object/sp-object.h"
#include "object/sp-root.h"

#include "svg/stringstream.h"
#include "svg/svg-color.h"

#include "ui/widget/canvas.h"

#include "util/units.h"

#include "xml/node-event-vector.h"

using Inkscape::DocumentUndo;
using Inkscape::Util::unit_table;

namespace Inkscape {

static gchar const *const grid_name[] = {
    N_("Rectangular grid"),
    N_("Axonometric grid")
};

static gchar const *const grid_svgname[] = {
    "xygrid",
    "axonomgrid"
};

// ##########################################################
//   CanvasGrid

    static Inkscape::XML::NodeEventVector const _repr_events = {
        nullptr, /* child_added */
        nullptr, /* child_removed */
        CanvasGrid::on_repr_attr_changed,
        nullptr, /* content_changed */
        nullptr  /* order_changed */
    };

CanvasGrid::CanvasGrid(SPNamedView * nv, Inkscape::XML::Node * in_repr, SPDocument *in_doc, GridType type)
    : visible(true), gridtype(type), legacy(false), pixel(false)
{
    repr = in_repr;
    doc = in_doc;
    if (repr) {
        repr->addListener (&_repr_events, this);
    }

    namedview = nv;
}

CanvasGrid::~CanvasGrid()
{
    if (repr) {
        repr->removeListenerByData (this);
    }

    for (auto grid : canvas_item_grids) {
        delete grid;
    }
    canvas_item_grids.clear();
}

const char *
CanvasGrid::getName() const
{
    return _(grid_name[gridtype]);
}

const char *
CanvasGrid::getSVGName() const
{
    return grid_svgname[gridtype];
}

GridType
CanvasGrid::getGridType() const
{
    return gridtype;
}


char const *
CanvasGrid::getName(GridType type)
{
    return _(grid_name[type]);
}

char const *
CanvasGrid::getSVGName(GridType type)
{
    return grid_svgname[type];
}

GridType
CanvasGrid::getGridTypeFromSVGName(char const *typestr)
{
    if (!typestr) return GRID_RECTANGULAR;

    gint t = 0;
    for (t = GRID_MAXTYPENR; t >= 0; t--) {  //this automatically defaults to grid0 which is rectangular grid
        if (!strcmp(typestr, grid_svgname[t])) break;
    }
    return (GridType) t;
}

GridType
CanvasGrid::getGridTypeFromName(char const *typestr)
{
    if (!typestr) return GRID_RECTANGULAR;

    gint t = 0;
    for (t = GRID_MAXTYPENR; t >= 0; t--) {  //this automatically defaults to grid0 which is rectangular grid
        if (!strcmp(typestr, _(grid_name[t]))) break;
    }
    return (GridType) t;
}


/*
*  writes an <inkscape:grid> child to repr.
*/
void
CanvasGrid::writeNewGridToRepr(Inkscape::XML::Node * repr, SPDocument * doc, GridType gridtype)
{
    if (!repr) return;
    if (gridtype > GRID_MAXTYPENR) return;

    // first create the child xml node, then hook it to repr. This order is important, to not set off listeners to repr before the new node is complete.

    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    Inkscape::XML::Node *newnode;
    newnode = xml_doc->createElement("inkscape:grid");
    newnode->setAttribute("type", getSVGName(gridtype));

    repr->appendChild(newnode);
    Inkscape::GC::release(newnode);

    DocumentUndo::done(doc, SP_VERB_DIALOG_DOCPROPERTIES, _("Create new grid"));
}

/*
* Creates a new CanvasGrid object of type gridtype
*/
CanvasGrid*
CanvasGrid::NewGrid(SPNamedView * nv, Inkscape::XML::Node * repr, SPDocument * doc, GridType gridtype)
{
    if (!repr) return nullptr;
    if (!doc) {
        g_error("CanvasGrid::NewGrid - doc==NULL");
        return nullptr;
    }

    switch (gridtype) {
        case GRID_RECTANGULAR:
            return new CanvasXYGrid(nv, repr, doc);
        case GRID_AXONOMETRIC:
            return new CanvasAxonomGrid(nv, repr, doc);
    }

    return nullptr;
}


/**
*  creates a new grid canvasitem for the SPDesktop given as parameter. Keeps a link to this canvasitem in the canvas_item_grids list.
*/
Inkscape::CanvasItemGrid *
CanvasGrid::createCanvasItem(SPDesktop * desktop)
{
    if (!desktop) return nullptr;
//    Johan: I think for multiple desktops it is best if each has their own canvasitem,
//           but share the same CanvasGrid object; that is what this function is for.

    // check if there is already a canvasitem on this desktop linking to this grid
    for (auto grid : canvas_item_grids) {
        if ( desktop->getCanvasGrids() == grid->get_parent() ) {
            return nullptr;
        }
    }

    Inkscape::CanvasItemGrid *grid = new Inkscape::CanvasItemGrid(desktop->getCanvasGrids(), this);
    grid->show();
    canvas_item_grids.push_back(grid);
    return grid;
}

/**
 * Remove a CanvasGridItem from vector. Does NOT delete CanvasGridItem.
 * This is used by the CanvasGridItem destructor to ensure no dangling pointer is left.
 */
void
CanvasGrid::removeCanvasItem(Inkscape::CanvasItemGrid *item)
{
    auto it = std::find(canvas_item_grids.begin(), canvas_item_grids.end(), item);
    if (it != canvas_item_grids.end()) {
        canvas_item_grids.erase(it);
    }
}

Gtk::Widget *
CanvasGrid::newWidget()
{
    Gtk::Box * vbox = Gtk::manage( new Gtk::Box(Gtk::ORIENTATION_VERTICAL) );
    Gtk::Label * namelabel = Gtk::manage(new Gtk::Label("", Gtk::ALIGN_CENTER) );

    Glib::ustring str("<b>");
    str += getName();
    str += "</b>";
    namelabel->set_markup(str);
    vbox->pack_start(*namelabel, false, false);

    _rcb_enabled = Gtk::manage( new Inkscape::UI::Widget::RegisteredCheckButton(
            _("_Enabled"),
            _("Makes the grid available for working with on the canvas."),
            "enabled", _wr, false, repr, doc) );

    _rcb_snap_visible_only = Gtk::manage( new Inkscape::UI::Widget::RegisteredCheckButton(
            _("Snap to visible _grid lines only"),
            _("When zoomed out, not all grid lines will be displayed. Only the visible ones will be snapped to"),
            "snapvisiblegridlinesonly", _wr, false, repr, doc) );

    _rcb_visible = Gtk::manage( new Inkscape::UI::Widget::RegisteredCheckButton(
            _("_Visible"),
            _("Determines whether the grid is displayed or not. Objects are still snapped to invisible grids."),
            "visible", _wr, false, repr, doc) );

    _as_alignment = Gtk::manage( new Inkscape::UI::Widget::AlignmentSelector() );
    _as_alignment->on_alignmentClicked().connect(sigc::mem_fun(*this, &CanvasGrid::align_clicked));

    Gtk::Box *left = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    left->pack_start(*_rcb_enabled, false, false);
    left->pack_start(*_rcb_visible, false, false);
    left->pack_start(*_rcb_snap_visible_only, false, false);

    if (getGridType() == GRID_RECTANGULAR) {
        _rcb_dotted = Gtk::manage( new Inkscape::UI::Widget::RegisteredCheckButton(
                _("_Show dots instead of lines"), _("If set, displays dots at gridpoints instead of gridlines"),
                "dotted", _wr, false, repr, doc) );
        _rcb_dotted->setActive(render_dotted);
        left->pack_start(*_rcb_dotted, false, false);
    }

    left->pack_start(*Gtk::manage(new Gtk::Label(_("Align to page:"))), false, false);
    left->pack_start(*_as_alignment, false, false);

    auto right = newSpecificWidget();
    right->set_hexpand(false);

    Gtk::Box *inner = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    inner->pack_start(*left, true, true);
    inner->pack_start(*right, false, false);
    vbox->pack_start(*inner, false, false);
    vbox->set_border_width(4);

    std::list<Gtk::Widget*> slaves;
    for (auto &item : left->get_children()) {
        if (item != _rcb_enabled) {
            slaves.push_back(item);
        }
    }
    slaves.push_back(right);
    _rcb_enabled->setSlaveWidgets(slaves);

    // set widget values
    _wr.setUpdating (true);
    _rcb_visible->setActive(visible);
    if (snapper != nullptr) {
        _rcb_enabled->setActive(snapper->getEnabled());
        _rcb_snap_visible_only->setActive(snapper->getSnapVisibleOnly());
    }
    _wr.setUpdating (false);
    return dynamic_cast<Gtk::Widget *> (vbox);
}

void
CanvasGrid::on_repr_attr_changed(Inkscape::XML::Node *repr, gchar const *key, gchar const *oldval, gchar const *newval, bool is_interactive, void *data)
{
    if (!data)
        return;

    (static_cast<CanvasGrid*>(data))->onReprAttrChanged(repr, key, oldval, newval, is_interactive);
}

bool CanvasGrid::isEnabled() const
{
    if (snapper == nullptr) {
       return false;
    }

    return snapper->getEnabled();
}

// Used to shift origin when page size changed to fit drawing.
void CanvasGrid::setOrigin(Geom::Point const &origin_px)
{
    SPRoot *root = doc->getRoot();
    double scale_x = 1.0;
    double scale_y = 1.0;
    if( root->viewBox_set ) {
        scale_x = root->viewBox.width()  / root->width.computed;
        scale_y = root->viewBox.height() / root->height.computed;
    }

    // Write out in 'user-units'
    Inkscape::SVGOStringStream os_x, os_y;
    os_x << origin_px[Geom::X] * scale_x;
    os_y << origin_px[Geom::Y] * scale_y;
    repr->setAttribute("originx", os_x.str());
    repr->setAttribute("originy", os_y.str());
}

void CanvasGrid::align_clicked(int align)
{
    Geom::Point dimensions = doc->getDimensions();
    dimensions[Geom::X] *= align % 3 * 0.5;
    dimensions[Geom::Y] *= align / 3 * 0.5;
    dimensions *= doc->doc2dt();
    setOrigin(dimensions);
}



// ##########################################################
//   CanvasXYGrid

CanvasXYGrid::CanvasXYGrid (SPNamedView * nv, Inkscape::XML::Node * in_repr, SPDocument * in_doc)
    : CanvasGrid(nv, in_repr, in_doc, GRID_RECTANGULAR)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gridunit = unit_table.getUnit(prefs->getString("/options/grids/xy/units"));
    if (!gridunit) {
        gridunit = unit_table.getUnit("px");
    }
    origin[Geom::X] = Inkscape::Util::Quantity::convert(prefs->getDouble("/options/grids/xy/origin_x", 0.0), gridunit, "px");
    origin[Geom::Y] = Inkscape::Util::Quantity::convert(prefs->getDouble("/options/grids/xy/origin_y", 0.0), gridunit, "px");
    color = prefs->getInt("/options/grids/xy/color", GRID_DEFAULT_COLOR);
    empcolor = prefs->getInt("/options/grids/xy/empcolor", GRID_DEFAULT_EMPCOLOR);
    empspacing = prefs->getInt("/options/grids/xy/empspacing", 5);
    spacing[Geom::X] = Inkscape::Util::Quantity::convert(prefs->getDouble("/options/grids/xy/spacing_x", 0.0), gridunit, "px");
    spacing[Geom::Y] = Inkscape::Util::Quantity::convert(prefs->getDouble("/options/grids/xy/spacing_y", 0.0), gridunit, "px");
    render_dotted = prefs->getBool("/options/grids/xy/dotted", false);

    snapper = new CanvasXYGridSnapper(this, &namedview->snap_manager, 0);

    if (repr) readRepr();
}

CanvasXYGrid::~CanvasXYGrid ()
{
   if (snapper) delete snapper;
}

static gboolean sp_nv_read_opacity(gchar const *str, guint32 *color)
{
    if (!str) {
        return FALSE;
    }

    gchar *u;
    gdouble v = g_ascii_strtod(str, &u);
    if (!u) {
        return FALSE;
    }
    v = CLAMP(v, 0.0, 1.0);

    *color = (*color & 0xffffff00) | (guint32) floor(v * 255.9999);

    return TRUE;
}

/** If the passed int is invalid (<=0), then set the widget and the int
    to use the given old value.

    @param oldVal Old value to use if the new one is invalid.
    @param pTarget The int to validate.
    @param widget Widget associated with the int.
*/
static void validateInt(gint oldVal,
                        gint* pTarget)
{
    // Avoid nullness.
    if ( pTarget == nullptr )
        return;

    // Invalid new value?
    if ( *pTarget <= 0 ) {
        // If the old value is somehow invalid as well, then default to 1.
        if ( oldVal <= 0 )
            oldVal = 1;

        // Reset the int and associated widget to the old value.
        *pTarget = oldVal;
    } //if

} //validateInt

void
CanvasXYGrid::readRepr()
{
    SPRoot *root = doc->getRoot();
    double scale_x = 1.0;
    double scale_y = 1.0;
    if( root->viewBox_set ) {
        scale_x = root->width.computed  / root->viewBox.width();
        scale_y = root->height.computed / root->viewBox.height();
        if (Geom::are_near(scale_x / scale_y, 1.0, Geom::EPSILON)) {
            // scaling is uniform, try to reduce numerical error
            scale_x = (scale_x + scale_y)/2.0;
            double scale_none = Inkscape::Util::Quantity::convert(1, doc->getDisplayUnit(), "px");
            if (Geom::are_near(scale_x / scale_none, 1.0, Geom::EPSILON))
                scale_x = scale_none; // objects are same size, reduce numerical error
            scale_y = scale_x;
        }
    }

    gchar const *value;

    if ( (value = repr->attribute("originx")) ) {

        Inkscape::Util::Quantity q = unit_table.parseQuantity(value);

        if( q.unit->type == UNIT_TYPE_LINEAR ) {
            // Legacy grid not in 'user units'
            origin[Geom::X] = q.value("px");
            legacy = true;
            if (q.unit->abbr == "px" ) {
                pixel = true;
            }
        } else {
            // Grid in 'user units'
            origin[Geom::X] = q.quantity * scale_x;
        }
    }

    if ( (value = repr->attribute("originy")) ) {

        Inkscape::Util::Quantity q = unit_table.parseQuantity(value);

        if( q.unit->type == UNIT_TYPE_LINEAR ) {
            // Legacy grid not in 'user units'
            origin[Geom::Y] = q.value("px");
            legacy = true;
            if (q.unit->abbr == "px" ) {
                pixel = true;
            }
        } else {
            // Grid in 'user units'
            origin[Geom::Y] = q.quantity * scale_y;
        }
    }

    if ( (value = repr->attribute("spacingx")) ) {

        // Ensure a valid default value
        if( spacing[Geom::X] <= 0.0 )
            spacing[Geom::X] = 1.0;

        Inkscape::Util::Quantity q = unit_table.parseQuantity(value);
        // Ensure a valid new value
        if( q.quantity > 0 ) {
            if( q.unit->type == UNIT_TYPE_LINEAR ) {
                // Legacy grid not in 'user units'
                spacing[Geom::X] = q.value("px");
                legacy = true;
                if (q.unit->abbr == "px" ) {
                    pixel = true;
                }
            } else {
                // Grid in 'user units'
                spacing[Geom::X] = q.quantity * scale_x;
            }
        }
    }

    if ( (value = repr->attribute("spacingy")) ) {

        // Ensure a valid default value
        if( spacing[Geom::Y] <= 0.0 )
            spacing[Geom::Y] = 1.0;

        Inkscape::Util::Quantity q = unit_table.parseQuantity(value);
        // Ensure a valid new value
        if( q.quantity > 0 ) {
            if( q.unit->type == UNIT_TYPE_LINEAR ) {
                // Legacy grid not in 'user units'
                spacing[Geom::Y] = q.value("px");
                legacy = true;
                if (q.unit->abbr == "px" ) {
                    pixel = true;
                }
            } else {
                // Grid in 'user units'
                spacing[Geom::Y] = q.quantity * scale_y;
            }
        }
    }

    if ( (value = repr->attribute("color")) ) {
        color = (color & 0xff) | sp_svg_read_color(value, color);
    }

    if ( (value = repr->attribute("empcolor")) ) {
        empcolor = (empcolor & 0xff) | sp_svg_read_color(value, empcolor);
    }

    if ( (value = repr->attribute("opacity")) ) {
        sp_nv_read_opacity(value, &color);
    }
    if ( (value = repr->attribute("empopacity")) ) {
        sp_nv_read_opacity(value, &empcolor);
    }

    if ( (value = repr->attribute("empspacing")) ) {
        gint oldVal = empspacing;
        empspacing = atoi(value);
        validateInt( oldVal, &empspacing);
    }

    if ( (value = repr->attribute("dotted")) ) {
        render_dotted = (strcmp(value,"false") != 0 && strcmp(value, "0") != 0);
    }

    if ( (value = repr->attribute("visible")) ) {
        visible = (strcmp(value,"false") != 0 && strcmp(value, "0") != 0);
    }

    if ( (value = repr->attribute("enabled")) ) {
        g_assert(snapper != nullptr);
        snapper->setEnabled(strcmp(value,"false") != 0 && strcmp(value, "0") != 0);
    }

    if ( (value = repr->attribute("snapvisiblegridlinesonly")) ) {
        g_assert(snapper != nullptr);
        snapper->setSnapVisibleOnly(strcmp(value,"false") != 0 && strcmp(value, "0") != 0);
    }

    if ( (value = repr->attribute("units")) ) {
        gridunit = unit_table.getUnit(value); // Display unit identifier in grid menu
    }

    for (auto grid : canvas_item_grids) {
        grid->request_update();
    }

    return;
}

/**
 * Called when XML node attribute changed; updates dialog widgets if change was not done by widgets themselves.
 */
void
CanvasXYGrid::onReprAttrChanged(Inkscape::XML::Node */*repr*/, gchar const */*key*/, gchar const */*oldval*/, gchar const */*newval*/, bool /*is_interactive*/)
{
    readRepr();
    updateWidgets();
}


Gtk::Widget *
CanvasXYGrid::newSpecificWidget()
{
    _rumg = Gtk::manage( new Inkscape::UI::Widget::RegisteredUnitMenu(
            _("Grid _units:"), "units", _wr, repr, doc) );
    _rsu_ox = Gtk::manage( new Inkscape::UI::Widget::RegisteredScalarUnit(
            _("_Origin X:"), _("X coordinate of grid origin"), "originx",
            *_rumg, _wr, repr, doc, Inkscape::UI::Widget::RSU_x) );
    _rsu_oy = Gtk::manage( new Inkscape::UI::Widget::RegisteredScalarUnit(
            _("O_rigin Y:"), _("Y coordinate of grid origin"), "originy",
            *_rumg, _wr, repr, doc, Inkscape::UI::Widget::RSU_y) );
    _rsu_sx = Gtk::manage( new Inkscape::UI::Widget::RegisteredScalarUnit(
            _("Spacing _X:"), _("Distance between vertical grid lines"), "spacingx",
            *_rumg, _wr, repr, doc, Inkscape::UI::Widget::RSU_x) );
    _rsu_sy = Gtk::manage( new Inkscape::UI::Widget::RegisteredScalarUnit(
            _("Spacing _Y:"), _("Distance between horizontal grid lines"), "spacingy",
            *_rumg, _wr, repr, doc, Inkscape::UI::Widget::RSU_y) );

    _rcp_gcol = Gtk::manage( new Inkscape::UI::Widget::RegisteredColorPicker(
            _("Minor grid line _color:"), _("Minor grid line color"), _("Color of the minor grid lines"),
            "color", "opacity", _wr, repr, doc) );

    _rcp_gmcol = Gtk::manage( new Inkscape::UI::Widget::RegisteredColorPicker(
            _("Ma_jor grid line color:"), _("Major grid line color"),
            _("Color of the major (highlighted) grid lines"), "empcolor", "empopacity",
            _wr, repr, doc) );

    _rsi = Gtk::manage( new Inkscape::UI::Widget::RegisteredSuffixedInteger(
            _("_Major grid line every:"), "", _("lines"), "empspacing", _wr, repr, doc) );

    _rumg->set_hexpand();
    _rsu_ox->set_hexpand();
    _rsu_oy->set_hexpand();
    _rsu_sx->set_hexpand();
    _rsu_sy->set_hexpand();
    _rcp_gcol->set_hexpand();
    _rcp_gmcol->set_hexpand();
    _rsi->set_hexpand();

    // set widget values
    _wr.setUpdating (true);

    _rsu_ox->setDigits(5);
    _rsu_ox->setIncrements(0.1, 1.0);

    _rsu_oy->setDigits(5);
    _rsu_oy->setIncrements(0.1, 1.0);

    _rsu_sx->setDigits(5);
    _rsu_sx->setIncrements(0.1, 1.0);

    _rsu_sy->setDigits(5);
    _rsu_sy->setIncrements(0.1, 1.0);

    _rumg->setUnit (gridunit->abbr);

    gdouble val;
    val = origin[Geom::X];
    val = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_ox->setValue (val);
    val = origin[Geom::Y];
    val = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_oy->setValue (val);
    val = spacing[Geom::X];
    double gridx = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_sx->setValue (gridx);
    val = spacing[Geom::Y];
    double gridy = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_sy->setValue (gridy);

    _rcp_gcol->setRgba32 (color);
    _rcp_gmcol->setRgba32 (empcolor);
    _rsi->setValue (empspacing);

    _wr.setUpdating (false);

    _rsu_ox->setProgrammatically = false;
    _rsu_oy->setProgrammatically = false;
    _rsu_sx->setProgrammatically = false;
    _rsu_sy->setProgrammatically = false;

    Gtk::Box *column = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    column->pack_start(*_rumg, true, false);
    column->pack_start(*_rsu_ox, true, false);
    column->pack_start(*_rsu_oy, true, false);
    column->pack_start(*_rsu_sx, true, false);
    column->pack_start(*_rsu_sy, true, false);
    column->pack_start(*_rcp_gcol, true, false);
    column->pack_start(*_rcp_gmcol, true, false);
    column->pack_start(*_rsi, true, false);

    return column;
}


/**
 * Update dialog widgets from object's values.
 */
void
CanvasXYGrid::updateWidgets()
{
    if (_wr.isUpdating()) return;

    //no widgets (grid created with the document, not with the dialog)
    if (!_rcb_visible) return;

    _wr.setUpdating (true);

    _rcb_visible->setActive(visible);
    if (snapper != nullptr) {
        _rcb_enabled->setActive(snapper->getEnabled());
        _rcb_snap_visible_only->setActive(snapper->getSnapVisibleOnly());
    }

    _rumg->setUnit (gridunit->abbr);

    gdouble val;

    val = origin[Geom::X];
    val = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_ox->setValue (val);

    val = origin[Geom::Y];
    val = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_oy->setValue (val);

    val = spacing[Geom::X];
    val = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_sx->setValue (val);

    val = spacing[Geom::Y];
    val = Inkscape::Util::Quantity::convert(val, "px", gridunit);
    _rsu_sy->setValue (val);

    _rsu_ox->setProgrammatically = false;
    _rsu_oy->setProgrammatically = false;
    _rsu_sx->setProgrammatically = false;
    _rsu_sy->setProgrammatically = false;

    _rcp_gcol->setRgba32 (color);
    _rcp_gmcol->setRgba32 (empcolor);
    _rsi->setValue (empspacing);
    _rcb_dotted->setActive (render_dotted);

    _wr.setUpdating (false);
}

// For correcting old SVG Inkscape files
void
CanvasXYGrid::Scale (Geom::Scale const &scale ) {
    origin *= scale;
    spacing *= scale;

    // Write out in 'user-units'
    Inkscape::SVGOStringStream os_x, os_y, ss_x, ss_y;
    os_x << origin[Geom::X];
    os_y << origin[Geom::Y];
    ss_x << spacing[Geom::X];
    ss_y << spacing[Geom::Y];
    repr->setAttribute("originx",  os_x.str());
    repr->setAttribute("originy",  os_y.str());
    repr->setAttribute("spacingx", ss_x.str());
    repr->setAttribute("spacingy", ss_y.str());
}

void
CanvasXYGrid::Update (Geom::Affine const &affine, unsigned int /*flags*/)
{
    ow = origin * affine;
    sw[0] = Geom::Point(spacing[0], 0) * affine.withoutTranslation();
    sw[1] = Geom::Point(0, spacing[1]) * affine.withoutTranslation();

    // Find suitable grid spacing for display
    for(int dim = 0; dim < 2; dim++) {
        gint scaling_factor = empspacing;

        if (scaling_factor <= 1)
            scaling_factor = 5;

        scaled[dim] = false;
        while (fabs(sw[dim].length()) < 8.0) {
            scaled[dim] = true;
            sw[dim] *= scaling_factor;
            /* First pass, go up to the major line spacing, then
               keep increasing by two. */
            scaling_factor = 2;
        }
    }
}


// Find intersections of line with rectangle. There should be zero or two.
// If line is degenerate with rectangle side, two corner points are returned.
static std::vector<Geom::Point>
intersect_line_rectangle( Geom::Line const &line, Geom::Rect const &rect )
{
    std::vector<Geom::Point> intersections;
    for (unsigned i = 0; i < 4; ++i) {
        Geom::LineSegment side( rect.corner(i), rect.corner((i+1)%4) );
        try {
            Geom::OptCrossing oc = Geom::intersection(line, side);
            if (oc) {
                intersections.push_back( line.pointAt((*oc).ta));
            }
        } catch (Geom::InfiniteSolutions) {
            intersections.clear();
            intersections.push_back( side.pointAt(0) );
            intersections.push_back( side.pointAt(1) );
            return intersections;
        }
    }
    return intersections;
}

// Find the signed distance of a point to a line. The distance is negative if
// the point lies to the left of the line considering the line's versor.
static double
signed_distance( Geom::Point const &point, Geom::Line const &line ) {
    Geom::Coord t = line.nearestTime( point );
    Geom::Point p = line.pointAt(t);
    double distance = Geom::distance( p, point );
    if ( Geom::cross( Geom::Line( p, point ).versor(), line.versor() ) < 0.0 ) {
        distance = -distance;
    }
    return distance;
}

void
CanvasXYGrid::Render (Inkscape::CanvasItemBuffer *buf)
{

    // no_emphasize_when_zoomedout determines color (minor or major) when only major grid lines/dots shown.
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint32 _empcolor;
    guint32 _color = color;
    bool no_emp_when_zoomed_out = prefs->getBool("/options/grids/no_emphasize_when_zoomedout", false);
    if( (scaled[Geom::X] || scaled[Geom::Y]) && no_emp_when_zoomed_out ) {
        _empcolor = color;
    } else {
        _empcolor = empcolor;
    }

    bool xrayactive = prefs->getBool("/desktop/xrayactive", false);
    if (xrayactive) {
        guint32 bg = namedview->pagecolor;
        _color = SP_RGBA32_F_COMPOSE(
                CLAMP(((1 - SP_RGBA32_A_F(_color)) * SP_RGBA32_R_F(bg)) + (SP_RGBA32_A_F(_color) * SP_RGBA32_R_F(_color)), 0.0, 1.0),
                CLAMP(((1 - SP_RGBA32_A_F(_color)) * SP_RGBA32_G_F(bg)) + (SP_RGBA32_A_F(_color) * SP_RGBA32_G_F(_color)), 0.0, 1.0),
                CLAMP(((1 - SP_RGBA32_A_F(_color)) * SP_RGBA32_B_F(bg)) + (SP_RGBA32_A_F(_color) * SP_RGBA32_B_F(_color)), 0.0, 1.0),
                1.0);
        _empcolor = SP_RGBA32_F_COMPOSE(
                CLAMP(((1 - SP_RGBA32_A_F(_empcolor)) * SP_RGBA32_R_F(bg)) + (SP_RGBA32_A_F(_empcolor) * SP_RGBA32_R_F(_empcolor)), 0.0, 1.0),
                CLAMP(((1 - SP_RGBA32_A_F(_empcolor)) * SP_RGBA32_G_F(bg)) + (SP_RGBA32_A_F(_empcolor) * SP_RGBA32_G_F(_empcolor)), 0.0, 1.0),
                CLAMP(((1 - SP_RGBA32_A_F(_empcolor)) * SP_RGBA32_B_F(bg)) + (SP_RGBA32_A_F(_empcolor) * SP_RGBA32_B_F(_empcolor)), 0.0, 1.0),
                1.0);
    }

    buf->cr->save();
    buf->cr->translate(-buf->rect.left(), -buf->rect.top());
    buf->cr->set_line_width(1.0);
    buf->cr->set_line_cap(Cairo::LINE_CAP_SQUARE);

    // Adding a 2 px margin to the buffer rectangle to avoid missing intersections (in case of rounding errors, and due to adding 0.5 below)
    Geom::IntRect buf_rect_with_margin = buf->rect;
    buf_rect_with_margin.expandBy(2);

    for (unsigned dim = 0; dim < 2; ++dim) {

        // std::cout << "\n  " << (dim==0?"Horizontal":"Vertical") << "   ------------" << std::endl;

        // Construct an axis line through origin with direction normal to grid spacing.
        Geom::Line axis = Geom::Line::from_origin_and_vector( ow, sw[dim]       );
        Geom::Line orth = Geom::Line::from_origin_and_vector( ow, sw[(dim+1)%2] );

        double spacing = sw[(dim+1)%2].length();  // Spacing between grid lines.
        double dash    = sw[dim].length();        // Total length of dash pattern.

        // std::cout << "  axis: " << axis.origin() << ", " << axis.vector() << std::endl;
        // std::cout << "  spacing: " << spacing << std::endl;
        // std::cout << "  dash period: " << dash << std::endl;

        // Find the minimum and maximum distances of the buffer corners from axis.
        double min =  Geom::infinity();
        double max = -Geom::infinity();
        for (unsigned c = 0; c < 4; ++c) {

            // We need signed distance... lib2geom offers only positive distance.
            double distance = signed_distance( buf_rect_with_margin.corner(c), axis );

            // Correct it for coordinate flips (inverts handedness).
            if (Geom::cross( axis.vector(), orth.vector() ) > 0 ) {
                distance = -distance;
            }

            if (distance < min)
                min = distance;
            if (distance > max)
                max = distance;
        }
        int start = floor( min/spacing );
        int stop  = floor( max/spacing );

        // std::cout << "  rect: " << buf->rect << std::endl;
        // std::cout << "  min: " << min << "  max: " << max << std::endl;
        // std::cout << "  start: " << start << "  stop: " << stop << std::endl;

        // Loop over grid lines that intersected buf rectangle.
        for (int j = start+1; j <= stop; ++j) {

            Geom::Line grid_line = Geom::make_parallel_line( ow + j * sw[(dim+1)%2], axis );

            std::vector<Geom::Point> x = intersect_line_rectangle( grid_line, buf_rect_with_margin );

            // If we have two intersections, grid line intersects buffer rectangle.
            if (x.size() == 2 ) {
                // Make sure lines are always drawn in the same direction (or dashes misplaced).
                Geom::Line vector( x[0], x[1]);
                if (Geom::dot( vector.vector(), axis.vector() ) < 0.0) {
                    std::swap(x[0], x[1]);
                }

                // Set up line. Need to use floor()+0.5 such that Cairo will draw us lines with a width of a single pixel, without any aliasing.
                // For this we need to position the lines at exactly half pixels, see https://www.cairographics.org/FAQ/#sharp_lines
                // Must be consistent with the pixel alignment of the guide lines, see CanvasXYGrid::Render(), and the drawing of the rulers
                buf->cr->move_to(floor(x[0][Geom::X]) + 0.5, floor(x[0][Geom::Y]) + 0.5);
                buf->cr->line_to(floor(x[1][Geom::X]) + 0.5, floor(x[1][Geom::Y]) + 0.5);
                
                // Set dash pattern and color.
                if (render_dotted) {
                    // alpha needs to be larger than in the line case to maintain a similar
                    // visual impact but setting it to the maximal value makes the dots
                    // dominant in some cases. Solution, increase the alpha by a factor of
                    // 4. This then allows some user adjustment.
                    guint32 _empdot = (_empcolor & 0xff) << 2;
                    if (_empdot > 0xff)
                        _empdot = 0xff;
                    _empdot += (_empcolor & 0xffffff00);

                    guint32 _colordot = (_color & 0xff) << 2;
                    if (_colordot > 0xff)
                        _colordot = 0xff;
                    _colordot += (_color & 0xffffff00);

                    // Dash pattern must use spacing from orthogonal direction.
                    // Offset is to center dash on orthogonal lines.
                    double offset = fmod( signed_distance( x[0], orth ), sw[dim].length());
                    if (Geom::cross( axis.vector(), orth.vector() ) > 0 ) {
                        offset = -offset;
                    }

                    std::vector<double> dashes;
                    if (!scaled[dim] && (j % empspacing) != 0) {
                        // Minor lines
                        dashes.push_back(1.0);
                        dashes.push_back(dash - 1.0);
                        offset -= 0.5;
                        buf->cr->set_source_rgba(SP_RGBA32_R_F(_colordot), SP_RGBA32_G_F(_colordot),
                                                 SP_RGBA32_B_F(_colordot), SP_RGBA32_A_F(_colordot));
                    } else {
                        // Major lines
                        dashes.push_back(3.0);
                        dashes.push_back(dash - 3.0);
                        offset -= 1.5; // Center dash on intersection.
                        buf->cr->set_source_rgba(SP_RGBA32_R_F(_empdot), SP_RGBA32_G_F(_empdot),
                                                 SP_RGBA32_B_F(_empdot), SP_RGBA32_A_F(_empdot));
                    }

                    buf->cr->set_line_cap(Cairo::LINE_CAP_BUTT);
                    buf->cr->set_dash(dashes, -offset);

                } else {

                    // Solid lines

                    // Set color
                    if (!scaled[dim] && (j % empspacing) != 0) {
                        buf->cr->set_source_rgba(SP_RGBA32_R_F(_color), SP_RGBA32_G_F(_color),
                                                 SP_RGBA32_B_F(_color), SP_RGBA32_A_F(_color));
                    } else {
                        buf->cr->set_source_rgba(SP_RGBA32_R_F(_empcolor), SP_RGBA32_G_F(_empcolor),
                                                 SP_RGBA32_B_F(_empcolor), SP_RGBA32_A_F(_empcolor));
                    }
                }

                buf->cr->stroke();

            } else {
                std::cerr << "CanvasXYGrid::render: Grid line doesn't intersect!" << std::endl;
            }
        }
    }

    buf->cr->restore();
}

CanvasXYGridSnapper::CanvasXYGridSnapper(CanvasXYGrid *grid, SnapManager *sm, Geom::Coord const d) : LineSnapper(sm, d)
{
    this->grid = grid;
}

/**
 *  \return Snap tolerance (desktop coordinates); depends on current zoom so that it's always the same in screen pixels
 */
Geom::Coord CanvasXYGridSnapper::getSnapperTolerance() const
{
    SPDesktop const *dt = _snapmanager->getDesktop();
    double const zoom =  dt ? dt->current_zoom() : 1;
    return _snapmanager->snapprefs.getGridTolerance() / zoom;
}

bool CanvasXYGridSnapper::getSnapperAlwaysSnap() const
{
    return _snapmanager->snapprefs.getGridTolerance() == 10000; //TODO: Replace this threshold of 10000 by a constant; see also tolerance-slider.cpp
}

LineSnapper::LineList
CanvasXYGridSnapper::_getSnapLines(Geom::Point const &p) const
{
    LineList s;

    if ( grid == nullptr ) {
        return s;
    }

    for (unsigned int i = 0; i < 2; ++i) {

        double spacing;

        if (getSnapVisibleOnly()) {
            // Only snapping to visible grid lines
            spacing = grid->sw[i].length(); // this is the spacing of the visible grid lines measured in screen pixels
            // convert screen pixels to px
            // FIXME: after we switch to snapping dist in screen pixels, this will be unnecessary
            SPDesktop const *dt = _snapmanager->getDesktop();
            if (dt) {
                spacing /= dt->current_zoom();
            }
        } else {
            // Snapping to any grid line, whether it's visible or not
            spacing = grid->spacing[i];
        }

        Geom::Coord rounded;
        Geom::Point point_on_line;
        Geom::Point cvec(0.,0.);
        cvec[i] = 1.;

        rounded = Inkscape::Util::round_to_upper_multiple_plus(p[i], spacing, grid->origin[i]);
        point_on_line = i ? Geom::Point(0, rounded) : Geom::Point(rounded, 0);
        s.push_back(std::make_pair(cvec, point_on_line));

        rounded = Inkscape::Util::round_to_lower_multiple_plus(p[i], spacing, grid->origin[i]);
        point_on_line = i ? Geom::Point(0, rounded) : Geom::Point(rounded, 0);
        s.push_back(std::make_pair(cvec, point_on_line));
    }

    return s;
}

void CanvasXYGridSnapper::_addSnappedLine(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance,  SnapSourceType const &source, long source_num, Geom::Point const &normal_to_line, Geom::Point const &point_on_line) const
{
    SnappedLine dummy = SnappedLine(snapped_point, snapped_distance, source, source_num, Inkscape::SNAPTARGET_GRID, getSnapperTolerance(), getSnapperAlwaysSnap(), normal_to_line, point_on_line);
    isr.grid_lines.push_back(dummy);
}

void CanvasXYGridSnapper::_addSnappedPoint(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const
{
    SnappedPoint dummy = SnappedPoint(snapped_point, source, source_num, Inkscape::SNAPTARGET_GRID, snapped_distance, getSnapperTolerance(), getSnapperAlwaysSnap(), constrained_snap, true);
    isr.points.push_back(dummy);
}

void CanvasXYGridSnapper::_addSnappedLinePerpendicularly(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const
{
    SnappedPoint dummy = SnappedPoint(snapped_point, source, source_num, Inkscape::SNAPTARGET_GRID_PERPENDICULAR, snapped_distance, getSnapperTolerance(), getSnapperAlwaysSnap(), constrained_snap, true);
    isr.points.push_back(dummy);
}

/**
 *  \return true if this Snapper will snap at least one kind of point.
 */
bool CanvasXYGridSnapper::ThisSnapperMightSnap() const
{
    return _snap_enabled && _snapmanager->snapprefs.isTargetSnappable(Inkscape::SNAPTARGET_GRID);
}

} // namespace Inkscape


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
