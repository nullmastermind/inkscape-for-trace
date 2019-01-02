// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * <sodipodi:namedview> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006      Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 1999-2013 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include "event-log.h"
#include <2geom/transforms.h>

#include "display/canvas-grid.h"
#include "util/units.h"
#include "svg/svg-color.h"
#include "xml/repr.h"
#include "attributes.h"
#include "document.h"
#include "document-undo.h"
#include "desktop-events.h"
#include "enums.h"
#include "ui/monitor.h"

#include "sp-guide.h"
#include "sp-item-group.h"
#include "sp-namedview.h"
#include "preferences.h"
#include "desktop.h"
#include "conn-avoid-ref.h" // for defaultConnSpacing.
#include "sp-root.h"
#include <gtkmm/window.h>

using Inkscape::DocumentUndo;
using Inkscape::Util::unit_table;

#define DEFAULTGRIDCOLOR 0x3f3fff25
#define DEFAULTGRIDEMPCOLOR 0x3f3fff60
#define DEFAULTGRIDEMPSPACING 5
#define DEFAULTGUIDECOLOR 0x0000ff7f
#define DEFAULTGUIDEHICOLOR 0xff00007f
#define DEFAULTBORDERCOLOR 0x000000ff
#define DEFAULTPAGECOLOR 0xffffff00

static void sp_namedview_setup_guides(SPNamedView * nv);
static void sp_namedview_lock_guides(SPNamedView * nv);
static void sp_namedview_show_single_guide(SPGuide* guide, bool show);
static void sp_namedview_lock_single_guide(SPGuide* guide, bool show);

static gboolean sp_str_to_bool(const gchar *str);
static gboolean sp_nv_read_opacity(const gchar *str, guint32 *color);

SPNamedView::SPNamedView() : SPObjectGroup(), snap_manager(this) {

    this->zoom = 0;
    this->guidecolor = 0;
    this->guidehicolor = 0;
    this->views.clear();
    this->borderlayer = 0;
    this->page_size_units = nullptr;
    this->window_x = 0;
    this->cy = 0;
    this->window_y = 0;
    this->display_units = nullptr;
    this->page_size_units = nullptr;
    this->pagecolor = 0;
    this->cx = 0;
    this->pageshadow = 0;
    this->window_width = 0;
    this->window_height = 0;
    this->window_maximized = 0;
    this->bordercolor = 0;

    this->editable = TRUE;
    this->showguides = TRUE;
    this->lockguides = false;
    this->grids_visible = false;
    this->showborder = TRUE;
    this->pagecheckerboard = FALSE;
    this->showpageshadow = TRUE;

    this->guides.clear();
    this->viewcount = 0;
    this->grids.clear();

    this->default_layer_id = 0;

    this->connector_spacing = defaultConnSpacing;
}

SPNamedView::~SPNamedView() = default;

static void sp_namedview_generate_old_grid(SPNamedView * /*nv*/, SPDocument *document, Inkscape::XML::Node *repr) {
    bool old_grid_settings_present = false;

    // set old settings
    const char* gridspacingx    = "1px";
    const char* gridspacingy    = "1px";
    const char* gridoriginy     = "0px";
    const char* gridoriginx     = "0px";
    const char* gridempspacing  = "5";
    const char* gridcolor       = "#3f3fff";
    const char* gridempcolor    = "#3f3fff";
    const char* gridopacity     = "0.15";
    const char* gridempopacity  = "0.38";

    const char* value = nullptr;
    if ((value = repr->attribute("gridoriginx"))) {
        gridoriginx = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridoriginy"))) {
        gridoriginy = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridspacingx"))) {
        gridspacingx = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridspacingy"))) {
        gridspacingy = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridcolor"))) {
        gridcolor = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridempcolor"))) {
        gridempcolor = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridempspacing"))) {
        gridempspacing = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridopacity"))) {
        gridopacity = value;
        old_grid_settings_present = true;
    }
    if ((value = repr->attribute("gridempopacity"))) {
        gridempopacity = value;
        old_grid_settings_present = true;
    }

    if (old_grid_settings_present) {
        // generate new xy grid with the correct settings
        // first create the child xml node, then hook it to repr. This order is important, to not set off listeners to repr before the new node is complete.

        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *newnode = xml_doc->createElement("inkscape:grid");
        newnode->setAttribute("id", "GridFromPre046Settings");
        newnode->setAttribute("type", Inkscape::CanvasGrid::getSVGName(Inkscape::GRID_RECTANGULAR));
        newnode->setAttribute("originx", gridoriginx);
        newnode->setAttribute("originy", gridoriginy);
        newnode->setAttribute("spacingx", gridspacingx);
        newnode->setAttribute("spacingy", gridspacingy);
        newnode->setAttribute("color", gridcolor);
        newnode->setAttribute("empcolor", gridempcolor);
        newnode->setAttribute("opacity", gridopacity);
        newnode->setAttribute("empopacity", gridempopacity);
        newnode->setAttribute("empspacing", gridempspacing);

        repr->appendChild(newnode);
        Inkscape::GC::release(newnode);

        // remove all old settings
        repr->setAttribute("gridoriginx", nullptr);
        repr->setAttribute("gridoriginy", nullptr);
        repr->setAttribute("gridspacingx", nullptr);
        repr->setAttribute("gridspacingy", nullptr);
        repr->setAttribute("gridcolor", nullptr);
        repr->setAttribute("gridempcolor", nullptr);
        repr->setAttribute("gridopacity", nullptr);
        repr->setAttribute("gridempopacity", nullptr);
        repr->setAttribute("gridempspacing", nullptr);

//        SPDocumentUndo::done(doc, SP_VERB_DIALOG_NAMEDVIEW, _("Create new grid from pre0.46 grid settings"));
    }
}

void SPNamedView::build(SPDocument *document, Inkscape::XML::Node *repr) {
    SPObjectGroup::build(document, repr);

    this->readAttr( "inkscape:document-units" );
    this->readAttr( "units" );
    this->readAttr( "viewonly" );
    this->readAttr( "showguides" );
    this->readAttr( "showgrid" );
    this->readAttr( "gridtolerance" );
    this->readAttr( "guidetolerance" );
    this->readAttr( "objecttolerance" );
    this->readAttr( "guidecolor" );
    this->readAttr( "guideopacity" );
    this->readAttr( "guidehicolor" );
    this->readAttr( "guidehiopacity" );
    this->readAttr( "showborder" );
    this->readAttr( "inkscape:showpageshadow" );
    this->readAttr( "borderlayer" );
    this->readAttr( "bordercolor" );
    this->readAttr( "borderopacity" );
    this->readAttr( "pagecolor" );
    this->readAttr( "inkscape:pagecheckerboard" );
    this->readAttr( "inkscape:pageopacity" );
    this->readAttr( "inkscape:pageshadow" );
    this->readAttr( "inkscape:zoom" );
    this->readAttr( "inkscape:cx" );
    this->readAttr( "inkscape:cy" );
    this->readAttr( "inkscape:window-width" );
    this->readAttr( "inkscape:window-height" );
    this->readAttr( "inkscape:window-x" );
    this->readAttr( "inkscape:window-y" );
    this->readAttr( "inkscape:window-maximized" );
    this->readAttr( "inkscape:snap-global" );
    this->readAttr( "inkscape:snap-bbox" );
    this->readAttr( "inkscape:snap-nodes" );
    this->readAttr( "inkscape:snap-others" );
    this->readAttr( "inkscape:snap-from-guide" );
    this->readAttr( "inkscape:snap-center" );
    this->readAttr( "inkscape:snap-smooth-nodes" );
    this->readAttr( "inkscape:snap-midpoints" );
    this->readAttr( "inkscape:snap-object-midpoints" );
    this->readAttr( "inkscape:snap-text-baseline" );
    this->readAttr( "inkscape:snap-bbox-edge-midpoints" );
    this->readAttr( "inkscape:snap-bbox-midpoints" );
    this->readAttr( "inkscape:snap-to-guides" );
    this->readAttr( "inkscape:snap-grids" );
    this->readAttr( "inkscape:snap-intersection-paths" );
    this->readAttr( "inkscape:object-paths" );
    this->readAttr( "inkscape:snap-perpendicular" );
    this->readAttr( "inkscape:snap-tangential" );
    this->readAttr( "inkscape:snap-path-clip" );
    this->readAttr( "inkscape:snap-path-mask" );
    this->readAttr( "inkscape:object-nodes" );
    this->readAttr( "inkscape:bbox-paths" );
    this->readAttr( "inkscape:bbox-nodes" );
    this->readAttr( "inkscape:snap-page" );
    this->readAttr( "inkscape:current-layer" );
    this->readAttr( "inkscape:connector-spacing" );
    this->readAttr( "inkscape:lockguides" );

    /* Construct guideline list */
    for (auto& o: children) {
        if (SP_IS_GUIDE(&o)) {
            SPGuide * g = SP_GUIDE(&o);
            this->guides.push_back(g);
            //g_object_set(G_OBJECT(g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
            g->setColor(this->guidecolor);
            g->setHiColor(this->guidehicolor);
            g->readAttr( "inkscape:color" );
        }
    }

    // backwards compatibility with grid settings (pre 0.46)
    sp_namedview_generate_old_grid(this, document, repr);
}

void SPNamedView::release() {
        this->guides.clear();

    // delete grids:
    for(std::vector<Inkscape::CanvasGrid *>::const_iterator it=this->grids.begin();it!=this->grids.end();++it )
        delete *it;
    this->grids.clear();
    SPObjectGroup::release();
}

void SPNamedView::set(SPAttributeEnum key, const gchar* value) {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool global_snapping = prefs->getBool("/options/snapdefault/value", false);
    switch (key) {
    case SP_ATTR_VIEWONLY:
            this->editable = (!value);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWGUIDES:
            if (!value) { // show guides if not specified, for backwards compatibility
                this->showguides = TRUE;
            } else {
                this->showguides = sp_str_to_bool(value);
            }
            sp_namedview_setup_guides(this);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWGRIDS:
            if (!value) { // don't show grids if not specified, for backwards compatibility
                this->grids_visible = false;
            } else {
                this->grids_visible = sp_str_to_bool(value);
            }
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GRIDTOLERANCE:
            this->snap_manager.snapprefs.setGridTolerance(value ? g_ascii_strtod(value, nullptr) : 10000);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDETOLERANCE:
            this->snap_manager.snapprefs.setGuideTolerance(value ? g_ascii_strtod(value, nullptr) : 20);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_OBJECTTOLERANCE:
            this->snap_manager.snapprefs.setObjectTolerance(value ? g_ascii_strtod(value, nullptr) : 20);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDECOLOR:
            this->guidecolor = (this->guidecolor & 0xff) | (DEFAULTGUIDECOLOR & 0xffffff00);

            if (value) {
                this->guidecolor = (this->guidecolor & 0xff) | sp_svg_read_color(value, this->guidecolor);
            }

            for(std::vector<SPGuide *>::const_iterator it=this->guides.begin();it!=this->guides.end();++it ) {
                (*it)->setColor(this->guidecolor);
                (*it)->readAttr("inkscape:color");
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDEOPACITY:
            this->guidecolor = (this->guidecolor & 0xffffff00) | (DEFAULTGUIDECOLOR & 0xff);
            sp_nv_read_opacity(value, &this->guidecolor);

            for(std::vector<SPGuide *>::const_iterator it=this->guides.begin();it!=this->guides.end();++it ) {
                (*it)->setColor(this->guidecolor);
                (*it)->readAttr("inkscape:color");
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDEHICOLOR:
            this->guidehicolor = (this->guidehicolor & 0xff) | (DEFAULTGUIDEHICOLOR & 0xffffff00);

            if (value) {
                this->guidehicolor = (this->guidehicolor & 0xff) | sp_svg_read_color(value, this->guidehicolor);
            }
            for(std::vector<SPGuide *>::const_iterator it=this->guides.begin();it!=this->guides.end();++it ) {
            	(*it)->setHiColor(this->guidehicolor);
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_GUIDEHIOPACITY:
            this->guidehicolor = (this->guidehicolor & 0xffffff00) | (DEFAULTGUIDEHICOLOR & 0xff);
            sp_nv_read_opacity(value, &this->guidehicolor);
            for(std::vector<SPGuide *>::const_iterator it=this->guides.begin();it!=this->guides.end();++it ) {
            	(*it)->setHiColor(this->guidehicolor);
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWBORDER:
            this->showborder = (value) ? sp_str_to_bool (value) : TRUE;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_BORDERLAYER:
            this->borderlayer = SP_BORDER_LAYER_BOTTOM;
            if (value && !strcasecmp(value, "true")) this->borderlayer = SP_BORDER_LAYER_TOP;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_BORDERCOLOR:
            this->bordercolor = (this->bordercolor & 0xff) | (DEFAULTBORDERCOLOR & 0xffffff00);
            if (value) {
                this->bordercolor = (this->bordercolor & 0xff) | sp_svg_read_color (value, this->bordercolor);
            }
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_BORDEROPACITY:
            this->bordercolor = (this->bordercolor & 0xffffff00) | (DEFAULTBORDERCOLOR & 0xff);
            sp_nv_read_opacity(value, &this->bordercolor);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_PAGECOLOR:
            this->pagecolor = (this->pagecolor & 0xff) | (DEFAULTPAGECOLOR & 0xffffff00);
            if (value) {
                this->pagecolor = (this->pagecolor & 0xff) | sp_svg_read_color(value, this->pagecolor);
            }
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_PAGECHECKERBOARD:
            this->pagecheckerboard = (value) ? sp_str_to_bool (value) : false;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_PAGEOPACITY:
            this->pagecolor = (this->pagecolor & 0xffffff00) | (DEFAULTPAGECOLOR & 0xff);
            sp_nv_read_opacity(value, &this->pagecolor);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_PAGESHADOW:
            this->pageshadow = value? atoi(value) : 2; // 2 is the default
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_SHOWPAGESHADOW:
            this->showpageshadow = (value) ? sp_str_to_bool(value) : TRUE;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_ZOOM:
            this->zoom = value ? g_ascii_strtod(value, nullptr) : 0; // zero means not set
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CX:
            this->cx = value ? g_ascii_strtod(value, nullptr) : HUGE_VAL; // HUGE_VAL means not set
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CY:
            this->cy = value ? g_ascii_strtod(value, nullptr) : HUGE_VAL; // HUGE_VAL means not set
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_WIDTH:
            this->window_width = value? atoi(value) : -1; // -1 means not set
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_HEIGHT:
            this->window_height = value ? atoi(value) : -1; // -1 means not set
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_X:
            this->window_x = value ? atoi(value) : 0;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_Y:
            this->window_y = value ? atoi(value) : 0;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_WINDOW_MAXIMIZED:
            this->window_maximized = value ? atoi(value) : 0;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_GLOBAL:
            this->snap_manager.snapprefs.setSnapEnabledGlobally(value ? sp_str_to_bool(value) : global_snapping);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_BBOX:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_BBOX_CATEGORY, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_NODE:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_NODE_CATEGORY, value ? sp_str_to_bool(value) : TRUE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_OTHERS:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_OTHERS_CATEGORY, value ? sp_str_to_bool(value) : TRUE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_ROTATION_CENTER:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_GRID:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_GRID, value ? sp_str_to_bool(value) : TRUE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_GUIDE:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_GUIDE, value ? sp_str_to_bool(value) : TRUE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_NODE_SMOOTH:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_NODE_SMOOTH, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_LINE_MIDPOINT:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_LINE_MIDPOINT, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_OBJECT_MIDPOINT:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_OBJECT_MIDPOINT, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_TEXT_BASELINE:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_TEXT_BASELINE, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE_MIDPOINT:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_BBOX_EDGE_MIDPOINT, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_BBOX_MIDPOINT:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_BBOX_MIDPOINT, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_PATH_INTERSECTION:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_PATH_INTERSECTION, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_PATH:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_PATH, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_PERP:
            this->snap_manager.snapprefs.setSnapPerp(value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_TANG:
            this->snap_manager.snapprefs.setSnapTang(value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_PATH_CLIP:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_PATH_CLIP, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_PATH_MASK:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_PATH_MASK, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_NODE_CUSP:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_NODE_CUSP, value ? sp_str_to_bool(value) : TRUE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_BBOX_EDGE:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_BBOX_EDGE, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_BBOX_CORNER:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_BBOX_CORNER, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_SNAP_PAGE_BORDER:
            this->snap_manager.snapprefs.setTargetSnappable(Inkscape::SNAPTARGET_PAGE_BORDER, value ? sp_str_to_bool(value) : FALSE);
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CURRENT_LAYER:
            this->default_layer_id = value ? g_quark_from_string(value) : 0;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_CONNECTOR_SPACING:
            this->connector_spacing = value ? g_ascii_strtod(value, nullptr) :
                    defaultConnSpacing;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    case SP_ATTR_INKSCAPE_DOCUMENT_UNITS: {
            /* The default display unit if the document doesn't override this: e.g. for files saved as
             * `plain SVG', or non-inkscape files, or files created by an inkscape 0.40 &
             * earlier.
             *
             * Note that these units are not the same as the units used for the values in SVG!
             *
             * We default to `px'.
             */
            static Inkscape::Util::Unit const *px = unit_table.getUnit("px");
            Inkscape::Util::Unit const *new_unit = px;

            if (value && document->getRoot()->viewBox_set) {
                Inkscape::Util::Unit const *const req_unit = unit_table.getUnit(value);
                if ( !unit_table.hasUnit(value) ) {
                    g_warning("Unrecognized unit `%s'", value);
                    /* fixme: Document errors should be reported in the status bar or
                     * the like (e.g. as per
                     * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing); g_log
                     * should be only for programmer errors. */
                } else if ( req_unit->isAbsolute() ) {
                    new_unit = req_unit;
                } else {
                    g_warning("Document units must be absolute like `mm', `pt' or `px', but found `%s'",
                              value);
                    /* fixme: Don't use g_log (see above). */
                }
            }
            this->display_units = new_unit;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    }
    case SP_ATTR_UNITS: {
        // Only used in "Custom size" section of Document Properties dialog
            Inkscape::Util::Unit const *new_unit = nullptr;

            if (value) {
                Inkscape::Util::Unit const *const req_unit = unit_table.getUnit(value);
                if ( !unit_table.hasUnit(value) ) {
                    g_warning("Unrecognized unit `%s'", value);
                    /* fixme: Document errors should be reported in the status bar or
                     * the like (e.g. as per
                     * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing); g_log
                     * should be only for programmer errors. */
                } else if ( req_unit->isAbsolute() ) {
                    new_unit = req_unit;
                } else {
                    g_warning("Document units must be absolute like `mm', `pt' or `px', but found `%s'",
                              value);
                    /* fixme: Don't use g_log (see above). */
                }
            }
            this->page_size_units = new_unit;
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    }
    case SP_ATTR_INKSCAPE_LOCKGUIDES:
            this->lockguides = value ? sp_str_to_bool(value) : FALSE;
            this->lockGuides();
            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    default:
            SPObjectGroup::set(key, value);
            break;
    }
}

/**
* add a grid item from SVG-repr. Check if this namedview already has a gridobject for this one! If desktop=null, add grid-canvasitem to all desktops of this namedview,
* otherwise only add it to the specified desktop.
*/
static Inkscape::CanvasGrid*
sp_namedview_add_grid(SPNamedView *nv, Inkscape::XML::Node *repr, SPDesktop *desktop) {
    Inkscape::CanvasGrid* grid = nullptr;
    //check if namedview already has an object for this grid
    for(std::vector<Inkscape::CanvasGrid *>::const_iterator it=nv->grids.begin();it!=nv->grids.end();++it ) {
        if (repr == (*it)->repr) {
            grid = (*it);
            break;
        }
    }

    if (!grid) {
        //create grid object
        Inkscape::GridType gridtype = Inkscape::CanvasGrid::getGridTypeFromSVGName(repr->attribute("type"));
        if (!nv->document) {
            g_warning("sp_namedview_add_grid - how come doc is null here?!");
            return nullptr;
        }
        grid = Inkscape::CanvasGrid::NewGrid(nv, repr, nv->document, gridtype);
        nv->grids.push_back(grid);
    }

    if (!desktop) {
        //add canvasitem to all desktops
        for(std::vector<SPDesktop *>::const_iterator it=nv->views.begin();it!=nv->views.end();++it ) {
            grid->createCanvasItem(*it);
        }
    } else {
        //add canvasitem only for specified desktop
        grid->createCanvasItem(desktop);
    }

    return grid;
}

void SPNamedView::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
    SPObjectGroup::child_added(child, ref);

    if (!strcmp(child->name(), "inkscape:grid")) {
        sp_namedview_add_grid(this, child, nullptr);
    } else {
        SPObject *no = this->document->getObjectByRepr(child);
        if ( !SP_IS_OBJECT(no) ) {
            return;
        }

        if (SP_IS_GUIDE(no)) {
            SPGuide *g = (SPGuide *) no;
            this->guides.push_back(g);

            //g_object_set(G_OBJECT(g), "color", this->guidecolor, "hicolor", this->guidehicolor, NULL);
            g->setColor(this->guidecolor);
            g->setHiColor(this->guidehicolor);
            g->readAttr("inkscape:color");

            if (this->editable) {
                for(std::vector<SPDesktop *>::const_iterator it=this->views.begin();it!=this->views.end();++it ) {
                    g->SPGuide::showSPGuide((*it)->guides, (GCallback) sp_dt_guide_event);

                    if ((*it)->guides_active) {
                        g->sensitize((*it)->getCanvas(), TRUE);
                    }

                    sp_namedview_show_single_guide(SP_GUIDE(g), this->showguides);
                }
            }
        }
    }
}

void SPNamedView::remove_child(Inkscape::XML::Node *child) {
    if (!strcmp(child->name(), "inkscape:grid")) {
        for(std::vector<Inkscape::CanvasGrid *>::iterator it=this->grids.begin();it!=this->grids.end();++it ) {
            if ( (*it)->repr == child ) {
                delete (*it);
                this->grids.erase(it);
                break;
            }
        }
    } else {
        for(std::vector<SPGuide *>::iterator it=this->guides.begin();it!=this->guides.end();++it ) {
            if ( (*it)->getRepr() == child ) {
                this->guides.erase(it); 
                break;
            }   
        }
    }

    SPObjectGroup::remove_child(child);
}

Inkscape::XML::Node* SPNamedView::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ( ( flags & SP_OBJECT_WRITE_EXT ) &&
         repr != this->getRepr() )
    {
        if (repr) {
            repr->mergeFrom(this->getRepr(), "id");
        } else {
            repr = this->getRepr()->duplicate(xml_doc);
        }
    }

    return repr;
}

void SPNamedView::show(SPDesktop *desktop)
{
    for(std::vector<SPGuide *>::const_iterator it=this->guides.begin();it!=this->guides.end();++it ) {
        (*it)->showSPGuide( desktop->guides, (GCallback) sp_dt_guide_event);
        if (desktop->guides_active) {
            (*it)->sensitize(desktop->getCanvas(), TRUE);
        }
        sp_namedview_show_single_guide((*it), showguides);
    }

    views.push_back(desktop);

    // generate grids specified in SVG:
    Inkscape::XML::Node *repr = this->getRepr();
    if (repr) {
        for (Inkscape::XML::Node * child = repr->firstChild() ; child != nullptr; child = child->next() ) {
            if (!strcmp(child->name(), "inkscape:grid")) {
                sp_namedview_add_grid(this, child, desktop);
            }
        }
    }

    desktop->showGrids(grids_visible, false);
}

/*
 * Restores window geometry from the document settings or defaults in prefs
 */
void sp_namedview_window_from_document(SPDesktop *desktop)
{
    SPNamedView *nv = desktop->namedview;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int window_geometry = prefs->getInt("/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_NONE);
    int default_size = prefs->getInt("/options/defaultwindowsize/value", PREFS_WINDOW_SIZE_NATURAL);
    bool new_document = (nv->window_width <= 0) || (nv->window_height <= 0);
    bool show_dialogs = true;

    // restore window size and position stored with the document
    Gtk::Window *win = desktop->getToplevel();
    g_assert(win);
    if (window_geometry == PREFS_WINDOW_GEOMETRY_LAST) {
        // do nothing, as we already have code for that in interface.cpp
        // TODO: Probably should not do similar things in two places
    } else if ((window_geometry == PREFS_WINDOW_GEOMETRY_FILE && nv->window_maximized) ||
               (new_document && (default_size == PREFS_WINDOW_SIZE_MAXIMIZED))) {
        win->maximize();
    } else {
        const int MIN_WINDOW_SIZE = 600;
        int w = 0;
        int h = 0;
        bool move_to_screen = false;
        if (window_geometry == PREFS_WINDOW_GEOMETRY_FILE && !new_document) {
            Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_at_point(nv->window_x, nv->window_y);
            w = MIN(monitor_geometry.get_width(), nv->window_width);
            h = MIN(monitor_geometry.get_height(), nv->window_height);      
            move_to_screen = true;
        } else if (default_size == PREFS_WINDOW_SIZE_LARGE) {
            Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_at_window(win->get_window());
            w = MAX(0.75 * monitor_geometry.get_width(), MIN_WINDOW_SIZE);
            h = MAX(0.75 * monitor_geometry.get_height(), MIN_WINDOW_SIZE);
        } else if (default_size == PREFS_WINDOW_SIZE_SMALL) {
            w = h = MIN_WINDOW_SIZE;
        } else if (default_size == PREFS_WINDOW_SIZE_NATURAL) {
            // don't set size (i.e. keep the gtk+ default, which will be the natural size)
            // unless gtk+ decided it would be a good idea to show a window that is larger than the screen
            Gdk::Rectangle monitor_geometry = Inkscape::UI::get_monitor_geometry_at_window(win->get_window());
            int monitor_width =  monitor_geometry.get_width();
            int monitor_height = monitor_geometry.get_height();
            int window_width, window_height;
            win->get_size(window_width, window_height);
            if (window_width > monitor_width || window_height > monitor_height) {
                w = std::min(monitor_width, window_width);
                h = std::min(monitor_height, window_height);
            }
        }
        if ((w > 0) && (h > 0)) {
#ifndef _WIN32
            gint dx= 0;
            gint dy = 0;
            gint dw = 0;
            gint dh = 0;
            desktop->getWindowGeometry(dx, dy, dw, dh);
            if ((w != dw) || (h != dh)) {
                // Don't show dialogs when window is initially resized on OSX/Linux due to gdl dock bug
                // This will happen on sp_desktop_widget_size_allocate
                show_dialogs = FALSE;
            }
#endif
            desktop->setWindowSize(w, h);
            if (move_to_screen) {
                // Hiding window will close app if it's last window. If we really need to hide it
                // here, we need to up the reference count of the application before hiding, and lower after showing.
                // win->hide();
                desktop->setWindowPosition(Geom::Point(nv->window_x, nv->window_y));
                // win->show();
            }
        }
    }

    // Cancel any history of transforms up to this point (must be before call to zoom).
    desktop->clear_transform_history();

    // restore zoom and view
    if (nv->zoom != 0 && nv->zoom != HUGE_VAL && !IS_NAN(nv->zoom)
        && nv->cx != HUGE_VAL && !IS_NAN(nv->cx)
        && nv->cy != HUGE_VAL && !IS_NAN(nv->cy)) {
        desktop->zoom_absolute_center_point( Geom::Point(nv->cx, nv->cy), nv->zoom );
    } else if (desktop->getDocument()) { // document without saved zoom, zoom to its page
        desktop->zoom_page();
    }

    if (show_dialogs) {
        desktop->show_dialogs();
    }
}

void SPNamedView::writeNewGrid(SPDocument *document,int gridtype)
{
    g_assert(this->getRepr() != nullptr);
    Inkscape::CanvasGrid::writeNewGridToRepr(this->getRepr(),document,static_cast<Inkscape::GridType>(gridtype));
}

bool SPNamedView::getSnapGlobal() const
{
    return this->snap_manager.snapprefs.getSnapEnabledGlobally();
}

void SPNamedView::setSnapGlobal(bool v)
{
    g_assert(this->getRepr() != nullptr);
    sp_repr_set_boolean(this->getRepr(), "inkscape:snap-global", v);
}

void sp_namedview_update_layers_from_document (SPDesktop *desktop)
{
    SPObject *layer = nullptr;
    SPDocument *document = desktop->doc();
    SPNamedView *nv = desktop->namedview;
    if ( nv->default_layer_id != 0 ) {
        layer = document->getObjectById(g_quark_to_string(nv->default_layer_id));
    }
    // don't use that object if it's not at least group
    if ( !layer || !SP_IS_GROUP(layer) ) {
        layer = nullptr;
    }
    // if that didn't work out, look for the topmost layer
    if (!layer) {
        for (auto& iter: document->getRoot()->children) {
            if (desktop->isLayer(&iter)) {
                layer = &iter;
            }
        }
    }
    if (layer) {
        desktop->setCurrentLayer(layer);
    }

    // FIXME: find a better place to do this
    desktop->event_log->updateUndoVerbs();
}

void sp_namedview_document_from_window(SPDesktop *desktop)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int window_geometry = prefs->getInt("/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_NONE);
    bool save_geometry_in_file = window_geometry == PREFS_WINDOW_GEOMETRY_FILE;
    bool save_viewport_in_file = prefs->getBool("/options/savedocviewport/value", true);
    Inkscape::XML::Node *view = desktop->namedview->getRepr();
    Geom::Rect const r = desktop->get_display_area();

    // saving window geometry is not undoable
    bool saved = DocumentUndo::getUndoSensitive(desktop->getDocument());
    DocumentUndo::setUndoSensitive(desktop->getDocument(), false);

    if (save_viewport_in_file) {
        sp_repr_set_svg_double(view, "inkscape:zoom", desktop->current_zoom());
        sp_repr_set_svg_double(view, "inkscape:cx", r.midpoint()[Geom::X]);
        sp_repr_set_svg_double(view, "inkscape:cy", r.midpoint()[Geom::Y]);
    }

    if (save_geometry_in_file) {
        gint w, h, x, y;
        desktop->getWindowGeometry(x, y, w, h);
        sp_repr_set_int(view, "inkscape:window-width", w);
        sp_repr_set_int(view, "inkscape:window-height", h);
        sp_repr_set_int(view, "inkscape:window-x", x);
        sp_repr_set_int(view, "inkscape:window-y", y);
        sp_repr_set_int(view, "inkscape:window-maximized", desktop->is_maximized());
    }

    view->setAttribute("inkscape:current-layer", desktop->currentLayer()->getId());

    // restore undoability
    DocumentUndo::setUndoSensitive(desktop->getDocument(), saved);
}

void SPNamedView::hide(SPDesktop const *desktop)
{
    g_assert(desktop != nullptr);
    g_assert(std::find(views.begin(),views.end(),desktop)!=views.end());
    for(std::vector<SPGuide *>::iterator it=this->guides.begin();it!=this->guides.end();++it ) {
        (*it)->hideSPGuide(desktop->getCanvas());
    }
    views.erase(std::remove(views.begin(),views.end(),desktop),views.end());
}

void SPNamedView::activateGuides(void* desktop, bool active)
{
    g_assert(desktop != nullptr);
    g_assert(std::find(views.begin(),views.end(),desktop)!=views.end());

    SPDesktop *dt = static_cast<SPDesktop*>(desktop);
    for(std::vector<SPGuide *>::iterator it=this->guides.begin();it!=this->guides.end();++it ) {
        (*it)->sensitize(dt->getCanvas(), active);
    }
}

static void sp_namedview_setup_guides(SPNamedView *nv)
{
    for(std::vector<SPGuide *>::iterator it=nv->guides.begin();it!=nv->guides.end();++it ) {
        sp_namedview_show_single_guide(*it, nv->showguides);
    }
}

static void sp_namedview_lock_guides(SPNamedView *nv)
{
    for(std::vector<SPGuide *>::iterator it=nv->guides.begin();it!=nv->guides.end();++it ) {
        sp_namedview_lock_single_guide(*it, nv->lockguides);
    }
}

static void sp_namedview_show_single_guide(SPGuide* guide, bool show)
{
    if (show) {
        guide->showSPGuide();
    } else {
        guide->hideSPGuide();
    }
}

static void sp_namedview_lock_single_guide(SPGuide* guide, bool locked)
{
    guide->set_locked(locked, true);
}

void sp_namedview_toggle_guides(SPDocument *doc, Inkscape::XML::Node *repr)
{
    unsigned int v;
    unsigned int set = sp_repr_get_boolean(repr, "showguides", &v);
    if (!set) { // hide guides if not specified, for backwards compatibility
        v = FALSE;
    } else {
        v = !v;
    }

    bool saved = DocumentUndo::getUndoSensitive(doc);
    DocumentUndo::setUndoSensitive(doc, false);
    sp_repr_set_boolean(repr, "showguides", v);
    DocumentUndo::setUndoSensitive(doc, saved);

    doc->setModifiedSinceSave();
}

void sp_namedview_guides_toggle_lock(SPDocument *doc, SPNamedView * namedview)
{
    unsigned int v;
    Inkscape::XML::Node *repr = namedview->getRepr();
    unsigned int set = sp_repr_get_boolean(repr, "inkscape:lockguides", &v);
    if (!set) { // hide guides if not specified, for backwards compatibility
        v = true;
    } else {
        v = !v;
    }

    bool saved = DocumentUndo::getUndoSensitive(doc);
    DocumentUndo::setUndoSensitive(doc, false);
    sp_repr_set_boolean(repr, "inkscape:lockguides", v);
    sp_namedview_lock_guides(namedview);
    DocumentUndo::setUndoSensitive(doc, saved);
    doc->setModifiedSinceSave();
}

void sp_namedview_show_grids(SPNamedView * namedview, bool show, bool dirty_document)
{
    namedview->grids_visible = show;

    SPDocument *doc = namedview->document;
    Inkscape::XML::Node *repr = namedview->getRepr();

    bool saved = DocumentUndo::getUndoSensitive(doc);
    DocumentUndo::setUndoSensitive(doc, false);
    sp_repr_set_boolean(repr, "showgrid", namedview->grids_visible);
    DocumentUndo::setUndoSensitive(doc, saved);

    /* we don't want the document to get dirty on startup; that's when
       we call this function with dirty_document = false */
    if (dirty_document) {
        doc->setModifiedSinceSave();
    }
}

gchar const *SPNamedView::getName() const
{
    SPException ex;
    SP_EXCEPTION_INIT(&ex);
    return this->getAttribute("id", &ex);
}

guint SPNamedView::getViewCount()
{
    return ++viewcount;
}

std::vector<SPDesktop *> const SPNamedView::getViewList() const
{
    return views;
}

/* This should be moved somewhere */

static gboolean sp_str_to_bool(const gchar *str)
{
    if (str) {
        if (!g_ascii_strcasecmp(str, "true") ||
            !g_ascii_strcasecmp(str, "yes") ||
            !g_ascii_strcasecmp(str, "y") ||
            (atoi(str) != 0)) {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean sp_nv_read_opacity(const gchar *str, guint32 *color)
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

SPNamedView *sp_document_namedview(SPDocument *document, const gchar *id)
{
    g_return_val_if_fail(document != nullptr, NULL);

    SPObject *nv = sp_item_group_get_child_by_name(document->getRoot(), nullptr, "sodipodi:namedview");
    g_assert(nv != nullptr);

    if (id == nullptr) {
        return (SPNamedView *) nv;
    }

    while (nv && strcmp(nv->getId(), id)) {
        nv = sp_item_group_get_child_by_name(document->getRoot(), nv, "sodipodi:namedview");
    }

    return (SPNamedView *) nv;
}

SPNamedView const *sp_document_namedview(SPDocument const *document, const gchar *id)
{
    return sp_document_namedview(const_cast<SPDocument *>(document), id);  // use a const_cast here to avoid duplicating code
}

void SPNamedView::setGuides(bool v)
{
    g_assert(this->getRepr() != nullptr);
    sp_repr_set_boolean(this->getRepr(), "showguides", v);
    sp_repr_set_boolean(this->getRepr(), "inkscape:guide-bbox", v);
}

bool SPNamedView::getGuides()
{
    g_assert(this->getRepr() != nullptr);
    unsigned int v;
    unsigned int set = sp_repr_get_boolean(this->getRepr(), "showguides", &v);
    if (!set) { // hide guides if not specified, for backwards compatibility
        v = FALSE;
    }

    return v;
}

void SPNamedView::lockGuides()
{
    sp_namedview_lock_guides(this);
}

/**
 * Gets page fitting margin information from the namedview node in the XML.
 * \param nv_repr reference to this document's namedview
 * \param key the same key used by the RegisteredScalarUnit in
 *        ui/widget/page-sizer.cpp
 * \param margin_units units for the margin
 * \param return_units units to return the result in
 * \param width width in px (for percentage margins)
 * \param height height in px (for percentage margins)
 * \param use_width true if the this key is left or right margins, false
 *        otherwise.  Used for percentage margins.
 * \return the margin size in px, else 0.0 if anything is invalid.
 */
double SPNamedView::getMarginLength(gchar const * const key,
                             Inkscape::Util::Unit const * const margin_units,
                             Inkscape::Util::Unit const * const return_units,
                             double const width,
                             double const height,
                             bool const use_width)
{
    double value;
    static Inkscape::Util::Unit const *percent = unit_table.getUnit("%");
    if(!this->storeAsDouble(key,&value)) {
        return 0.0;
    }
    if (*margin_units == *percent) {
        return (use_width)? width * value : height * value; 
    }
    if (!margin_units->compatibleWith(return_units)) {
        return 0.0;
    }
    return value;
}

/**
 * Returns namedview's default unit.
 * If no default unit is set, "px" is returned
 */
Inkscape::Util::Unit const * SPNamedView::getDisplayUnit() const
{
    return display_units ? display_units : unit_table.getUnit("px");
}

/**
 * Returns the first grid it could find that isEnabled(). Returns NULL, if none is enabled
 */
Inkscape::CanvasGrid * sp_namedview_get_first_enabled_grid(SPNamedView *namedview)
{
    for(std::vector<Inkscape::CanvasGrid *>::const_iterator it=namedview->grids.begin();it!=namedview->grids.end();++it ) {
        if ((*it)->isEnabled())
            return (*it);
    }

    return nullptr;
}

void SPNamedView::translateGuides(Geom::Translate const &tr) {
    for(std::vector<SPGuide *>::iterator it=this->guides.begin();it!=this->guides.end();++it ) {
        SPGuide &guide = *(*it);
        Geom::Point point_on_line = guide.getPoint();
        point_on_line *= tr;
        guide.moveto(point_on_line, true);
    }
}

void SPNamedView::translateGrids(Geom::Translate const &tr) {
    for(std::vector<Inkscape::CanvasGrid *>::iterator it=this->grids.begin();it!=this->grids.end();++it ) {
        (*it)->setOrigin((*it)->origin * tr);
    }
}

void SPNamedView::scrollAllDesktops(double dx, double dy, bool is_scrolling) {
    for(std::vector<SPDesktop *>::iterator it=this->views.begin();it!=this->views.end();++it ) {
        (*it)->scroll_relative_in_svg_coords(dx, dy, is_scrolling);
    }
}


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
