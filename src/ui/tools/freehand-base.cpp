// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Generic drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2012 Johan Engelen
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#define DRAW_VERBOSE

#include "desktop-style.h"
#include "message-stack.h"
#include "selection-chemistry.h"

#include "display/canvas-bpath.h"
#include "display/curve.h"

#include "include/macros.h"

#include "live_effects/lpe-bendpath.h"
#include "live_effects/lpe-patternalongpath.h"
#include "live_effects/lpe-simplify.h"
#include "live_effects/lpe-powerstroke.h"

#include "svg/svg.h"

#include "object/sp-item-group.h"
#include "object/sp-path.h"
#include "object/sp-rect.h"
#include "object/sp-use.h"
#include "style.h"

#include "ui/clipboard.h"
#include "ui/control-manager.h"
#include "ui/draw-anchor.h"
#include "ui/tools/lpe-tool.h"
#include "ui/tools/pen-tool.h"
#include "ui/tools/pencil-tool.h"

#define MIN_PRESSURE      0.0
#define MAX_PRESSURE      1.0
#define DEFAULT_PRESSURE  1.0

using Inkscape::DocumentUndo;

namespace Inkscape {
namespace UI {
namespace Tools {

static void spdc_selection_changed(Inkscape::Selection *sel, FreehandBase *dc);
static void spdc_selection_modified(Inkscape::Selection *sel, guint flags, FreehandBase *dc);

static void spdc_attach_selection(FreehandBase *dc, Inkscape::Selection *sel);

/**
 * Flushes white curve(s) and additional curve into object.
 *
 * No cleaning of colored curves - this has to be done by caller
 * No rereading of white data, so if you cannot rely on ::modified, do it in caller
 */
static void spdc_flush_white(FreehandBase *dc, SPCurve *gc);

static void spdc_reset_white(FreehandBase *dc);
static void spdc_free_colors(FreehandBase *dc);

FreehandBase::FreehandBase(gchar const *const *cursor_shape)
    : ToolBase(cursor_shape)
    , selection(nullptr)
    , grab(nullptr)
    , attach(false)
    , red_color(0xff00007f)
    , blue_color(0x0000ff7f)
    , green_color(0x00ff007f)
    , highlight_color(0x0000007f)
    , red_bpath(nullptr)
    , red_curve(nullptr)
    , blue_bpath(nullptr)
    , blue_curve(nullptr)
    , green_curve(nullptr)
    , green_anchor(nullptr)
    , green_closed(false)
    , white_item(nullptr)
    , sa_overwrited(nullptr)
    , sa(nullptr)
    , ea(nullptr)
    , waiting_LPE_type(Inkscape::LivePathEffect::INVALID_LPE)
    , red_curve_is_valid(false)
    , anchor_statusbar(false)
    , tablet_enabled(false)
    , is_tablet(false)
    , pressure(DEFAULT_PRESSURE)
{
}

FreehandBase::~FreehandBase() {
    if (this->grab) {
        sp_canvas_item_ungrab(this->grab);
        this->grab = nullptr;
    }

    if (this->selection) {
        this->selection = nullptr;
    }

    spdc_free_colors(this);
}

void FreehandBase::setup() {
    ToolBase::setup();

    this->selection = desktop->getSelection();

    // Connect signals to track selection changes
    this->sel_changed_connection = this->selection->connectChanged(
        sigc::bind(sigc::ptr_fun(&spdc_selection_changed), this)
    );
    this->sel_modified_connection = this->selection->connectModified(
        sigc::bind(sigc::ptr_fun(&spdc_selection_modified), this)
    );

    // Create red bpath
    this->red_bpath = sp_canvas_bpath_new(this->desktop->getSketch(), nullptr);
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(this->red_bpath), this->red_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);

    // Create red curve
    this->red_curve = new SPCurve();

    // Create blue bpath
    this->blue_bpath = sp_canvas_bpath_new(this->desktop->getSketch(), nullptr);
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(this->blue_bpath), this->blue_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);

    // Create blue curve
    this->blue_curve = new SPCurve();

    // Create green curve
    this->green_curve = new SPCurve();

    // No green anchor by default
    this->green_anchor = nullptr;
    this->green_closed = FALSE;

    // Create start anchor alternative curve
    this->sa_overwrited = new SPCurve();

    this->attach = TRUE;
    spdc_attach_selection(this, this->selection);
}

void FreehandBase::finish() {
    this->sel_changed_connection.disconnect();
    this->sel_modified_connection.disconnect();

    if (this->grab) {
        sp_canvas_item_ungrab(this->grab);
    }

    if (this->selection) {
        this->selection = nullptr;
    }

    spdc_free_colors(this);

    ToolBase::finish();
}

void FreehandBase::set(const Inkscape::Preferences::Entry& /*value*/) {
}

bool FreehandBase::root_handler(GdkEvent* event) {
    gint ret = FALSE;

    switch (event->type) {
        case GDK_KEY_PRESS:
            switch (get_latin_keyval (&event->key)) {
                case GDK_KEY_Up:
                case GDK_KEY_Down:
                case GDK_KEY_KP_Up:
                case GDK_KEY_KP_Down:
                    // prevent the zoom field from activation
                    if (!MOD__CTRL_ONLY(event)) {
                        ret = TRUE;
                    }
                    break;
                default:
            break;
        }
        break;
    default:
        break;
    }

    if (!ret) {
    	ret = ToolBase::root_handler(event);
    }

    return ret;
}

static Glib::ustring const tool_name(FreehandBase *dc)
{
    return ( SP_IS_PEN_CONTEXT(dc)
             ? "/tools/freehand/pen"
             : "/tools/freehand/pencil" );
}

static void spdc_paste_curve_as_freehand_shape(Geom::PathVector const &newpath, FreehandBase *dc, SPItem *item)
{
    using namespace Inkscape::LivePathEffect;

    // TODO: Don't paste path if nothing is on the clipboard

    Effect::createAndApply(PATTERN_ALONG_PATH, dc->desktop->doc(), item);
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
    static_cast<LPEPatternAlongPath*>(lpe)->pattern.set_new_value(newpath,true);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double scale = prefs->getDouble("/live_effect/pap/width", 1);
    if (!scale) {
        scale = 1 / dc->desktop->doc()->getDocumentScale()[0];
    }
    Inkscape::SVGOStringStream os;
    os << scale;
    lpe->getRepr()->setAttribute("prop_scale", os.str().c_str());
}

static void spdc_apply_powerstroke_shape(std::vector<Geom::Point> points, FreehandBase *dc, SPItem *item)
{
    using namespace Inkscape::LivePathEffect;

    if (SP_IS_PENCIL_CONTEXT(dc)) {
        PencilTool *pt = SP_PENCIL_CONTEXT(dc);
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (dc->tablet_enabled) {
            Geom::Affine transform_coordinate = SP_ITEM(SP_ACTIVE_DESKTOP->currentLayer())->i2dt_affine().inverse();
            item->transform = transform_coordinate;
            SPShape *sp_shape = dynamic_cast<SPShape *>(item);
            if (sp_shape) {
                SPCurve * c = sp_shape->getCurve();
                if (!c) {
                    return;
                }
                if(pt->points.empty()){
                    SPCSSAttr *css_item = sp_css_attr_from_object(item, SP_STYLE_FLAG_ALWAYS);
                    const char *stroke_width = sp_repr_css_property(css_item, "stroke-width", "0");
                    double swidth;
                    sp_svg_number_read_d(stroke_width, &swidth);
                    swidth = prefs->getDouble("/live_effect/power_stroke/width", swidth/2);
                    if (!swidth) {
                        swidth = swidth/2;
                    }
                    pt->points.emplace_back(0, swidth);
                }
                Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
                LPEPowerStroke* ps = nullptr;
                Glib::ustring pref_path_pp = "/live_effects/powerstroke/powerpencil";
                prefs->setBool(pref_path_pp, true);
                if (lpe) {
                    ps = static_cast<LPEPowerStroke*>(lpe);
                } 
                if (!lpe || !ps) {
                    Effect::createAndApply(POWERSTROKE, SP_ACTIVE_DESKTOP->doc(), item);
                    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
                    ps = static_cast<LPEPowerStroke*>(lpe);
                }
                if (ps) {
                    if (pt->sa) {
                        std::vector<Geom::Point> sa_points;
                        if (pt->sa->start) {
                            sa_points = ps->offset_points.reverse_controlpoints(false);
                        } else {
                            sa_points = ps->offset_points.data();
                        }
                        sa_points.insert(sa_points.end(), pt->points.begin(), pt->points.end());
                        pt->points = sa_points;
                        sa_points.clear();
                    }
                    Geom::Path path = c->get_pathvector()[0];
                    if (!path.empty()) {
                        pt->powerStrokeInterpolate(path);
                    } 
                    ps->offset_points.param_set_and_write_new_value(pt->points);
                }
                prefs->setBool(pref_path_pp, false);
                return;
            }
        }
    }
    Effect::createAndApply(POWERSTROKE, dc->desktop->doc(), item);
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();

    static_cast<LPEPowerStroke*>(lpe)->offset_points.param_set_and_write_new_value(points);

    // write powerstroke parameters:
    lpe->getRepr()->setAttribute("start_linecap_type", "zerowidth");
    lpe->getRepr()->setAttribute("end_linecap_type", "zerowidth");
    lpe->getRepr()->setAttribute("sort_points", "true");
    lpe->getRepr()->setAttribute("interpolator_type", "CubicBezierJohan");
    lpe->getRepr()->setAttribute("interpolator_beta", "0.2");
    lpe->getRepr()->setAttribute("miter_limit", "4");
    lpe->getRepr()->setAttribute("linejoin_type", "extrp_arc");
}

static void spdc_apply_bend_shape(gchar const *svgd, FreehandBase *dc, SPItem *item)
{
    using namespace Inkscape::LivePathEffect;
    SPUse *use = dynamic_cast<SPUse *>(item);
    if ( use ) {
        return;
    }
    if(!SP_IS_LPE_ITEM(item) || !SP_LPE_ITEM(item)->hasPathEffectOfType(BEND_PATH)){
        Effect::createAndApply(BEND_PATH, dc->desktop->doc(), item);
    }
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();

    // write bend parameters:
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double scale = prefs->getDouble("/live_effect/bend/width", 1);
    if (!scale) {
        scale = 1;
    }
    Inkscape::SVGOStringStream os;
    os << scale;
    lpe->getRepr()->setAttribute("prop_scale", os.str().c_str());
    lpe->getRepr()->setAttribute("scale_y_rel", "false");
    lpe->getRepr()->setAttribute("vertical", "false");
    static_cast<LPEBendPath*>(lpe)->bend_path.paste_param_path(svgd);
}

static void spdc_apply_simplify(std::string threshold, FreehandBase *dc, SPItem *item)
{
    using namespace Inkscape::LivePathEffect;

    Effect::createAndApply(SIMPLIFY, dc->desktop->doc(), item);
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
    // write simplify parameters:
    lpe->getRepr()->setAttribute("steps", "1");
    lpe->getRepr()->setAttribute("threshold", threshold);
    lpe->getRepr()->setAttribute("smooth_angles", "360");
    lpe->getRepr()->setAttribute("helper_size", "0");
    lpe->getRepr()->setAttribute("simplifyindividualpaths", "false");
    lpe->getRepr()->setAttribute("simplifyJustCoalesce", "false");
}

enum shapeType { NONE, TRIANGLE_IN, TRIANGLE_OUT, ELLIPSE, CLIPBOARD, BEND_CLIPBOARD, LAST_APPLIED };
static shapeType previous_shape_type = NONE;

static void spdc_check_for_and_apply_waiting_LPE(FreehandBase *dc, SPItem *item, SPCurve *curve, bool is_bend)
{
    using namespace Inkscape::LivePathEffect;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (item && SP_IS_LPE_ITEM(item)) {
        //Store the clipboard path to apply in the future without the use of clipboard
        static Geom::PathVector previous_shape_pathv;
        static SPItem *bend_item;
        shapeType shape = (shapeType)prefs->getInt(tool_name(dc) + "/shape", 0);
        if (previous_shape_type == NONE) {
            previous_shape_type = shape;
        }
        if(shape == LAST_APPLIED){
            shape = previous_shape_type;
            if(shape == CLIPBOARD || shape == BEND_CLIPBOARD){
                shape = LAST_APPLIED;
            }
        }
        Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
        if (is_bend && 
           (shape == BEND_CLIPBOARD || (shape == LAST_APPLIED && previous_shape_type != CLIPBOARD)) && 
            cm->paste(SP_ACTIVE_DESKTOP,true))
        {
            bend_item = dc->selection->singleItem();
            if(!bend_item || (!SP_IS_SHAPE(bend_item) && !SP_IS_GROUP(bend_item))){
                previous_shape_type = NONE;
                return;
            }
        } else if(is_bend) {
            return;
        }
        if (!is_bend && previous_shape_type == BEND_CLIPBOARD && shape == BEND_CLIPBOARD) {
            return;
        }
        bool shape_applied = false;
        bool simplify = prefs->getInt(tool_name(dc) + "/simplify", 0);
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        guint mode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        if(simplify && mode != 2){
            double tol = prefs->getDoubleLimited("/tools/freehand/pencil/tolerance", 10.0, 1.0, 100.0);
            tol = tol/(100.0*(102.0-tol));
            std::ostringstream ss;
            ss << tol;
            spdc_apply_simplify(ss.str(), dc, item);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
        }
        if (prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 1) {
            Effect::createAndApply(SPIRO, dc->desktop->doc(), item);
        }

        if (prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 2) {
            Effect::createAndApply(BSPLINE, dc->desktop->doc(), item);
        }
        SPShape *sp_shape = dynamic_cast<SPShape *>(item);
        if (sp_shape) {
            curve = sp_shape->getCurve();
        }
        SPCSSAttr *css_item = sp_css_attr_from_object(item, SP_STYLE_FLAG_ALWAYS);
        const char *cstroke = sp_repr_css_property(css_item, "stroke", "none");
        const char *cfill = sp_repr_css_property(css_item, "fill", "none");
        const char *stroke_width = sp_repr_css_property(css_item, "stroke-width", "0");
        double swidth;
        sp_svg_number_read_d(stroke_width, &swidth);
        swidth = prefs->getDouble("/live_effect/power_stroke/width", swidth/2);
        if (!swidth) {
            swidth = swidth/2;
        }
        if (SP_IS_PENCIL_CONTEXT(dc)) {
            if (dc->tablet_enabled) {
                std::vector<Geom::Point> points;
                spdc_apply_powerstroke_shape(points, dc, item);
                shape_applied = true;
                shape = NONE;
                previous_shape_type = NONE;
            }
        }
        
#define SHAPE_LENGTH 10
#define SHAPE_HEIGHT 10

        switch (shape) {
            case NONE:
                // don't apply any shape
                break;
            case TRIANGLE_IN:
            {
                // "triangle in"
                std::vector<Geom::Point> points(1);
                
                points[0] = Geom::Point(0., swidth);
                //points[0] *= i2anc_affine(static_cast<SPItem *>(item->parent), NULL).inverse();
                spdc_apply_powerstroke_shape(points, dc, item);

                shape_applied = true;
                break;
            }
            case TRIANGLE_OUT:
            {
                // "triangle out"
                guint curve_length = curve->get_segment_count();
                std::vector<Geom::Point> points(1);
                points[0] = Geom::Point(0, swidth);
                //points[0] *= i2anc_affine(static_cast<SPItem *>(item->parent), NULL).inverse();
                points[0][Geom::X] = (double)curve_length;
                spdc_apply_powerstroke_shape(points, dc, item);

                shape_applied = true;
                break;
            }
            case ELLIPSE:
            {
                // "ellipse"
                SPCurve *c = new SPCurve();
                const double C1 = 0.552;
                c->moveto(0, SHAPE_HEIGHT/2);
                c->curveto(0, (1 - C1) * SHAPE_HEIGHT/2, (1 - C1) * SHAPE_LENGTH/2, 0, SHAPE_LENGTH/2, 0);
                c->curveto((1 + C1) * SHAPE_LENGTH/2, 0, SHAPE_LENGTH, (1 - C1) * SHAPE_HEIGHT/2, SHAPE_LENGTH, SHAPE_HEIGHT/2);
                c->curveto(SHAPE_LENGTH, (1 + C1) * SHAPE_HEIGHT/2, (1 + C1) * SHAPE_LENGTH/2, SHAPE_HEIGHT, SHAPE_LENGTH/2, SHAPE_HEIGHT);
                c->curveto((1 - C1) * SHAPE_LENGTH/2, SHAPE_HEIGHT, 0, (1 + C1) * SHAPE_HEIGHT/2, 0, SHAPE_HEIGHT/2);
                c->closepath();
                spdc_paste_curve_as_freehand_shape(c->get_pathvector(), dc, item);
                c->unref();

                shape_applied = true;
                break;
            }
            case CLIPBOARD:
            {
                // take shape from clipboard;
                Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
                if(cm->paste(SP_ACTIVE_DESKTOP,true)){
                    SPItem * pasted_clipboard = dc->selection->singleItem();
                    dc->selection->toCurves();
                    pasted_clipboard = dc->selection->singleItem();
                    if(pasted_clipboard){
                        Inkscape::XML::Node *pasted_clipboard_root = pasted_clipboard->getRepr();
                        Inkscape::XML::Node *path = sp_repr_lookup_name(pasted_clipboard_root, "svg:path", -1); // unlimited search depth
                        if ( path != nullptr ) {
                            gchar const *svgd = path->attribute("d");
                            dc->selection->remove(SP_OBJECT(pasted_clipboard));
                            previous_shape_pathv =  sp_svg_read_pathv(svgd);
                            previous_shape_pathv *= pasted_clipboard->transform;
                            spdc_paste_curve_as_freehand_shape(previous_shape_pathv, dc, item);

                            shape = CLIPBOARD;
                            shape_applied = true;
                            pasted_clipboard->deleteObject();
                        } else {
                            shape = NONE;
                        }
                    } else {
                        shape = NONE;
                    }
                } else {
                    shape = NONE;
                }
                break;
            }
            case BEND_CLIPBOARD:
            {
                gchar const *svgd = item->getRepr()->attribute("d");
                if(bend_item && (SP_IS_SHAPE(bend_item) || SP_IS_GROUP(bend_item))){
                    // If item is a SPRect, convert it to path first:
                    if ( dynamic_cast<SPRect *>(bend_item) ) {
                        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
                        if (desktop) {
                            Inkscape::Selection *sel = desktop->getSelection();
                            if ( sel && !sel->isEmpty() ) {
                                sel->clear();
                                sel->add(bend_item);
                                sel->toCurves();
                                bend_item = sel->singleItem();
                            }
                        }
                    }
                    bend_item->moveTo(item,false);
                    bend_item->transform.setTranslation(Geom::Point());
                    spdc_apply_bend_shape(svgd, dc, bend_item);
                    dc->selection->add(SP_OBJECT(bend_item));

                    shape = BEND_CLIPBOARD;
                } else {
                    bend_item = nullptr;
                    shape = NONE;
                }
                break;
            }
            case LAST_APPLIED:
            {
                if(previous_shape_type == CLIPBOARD){
                    if(previous_shape_pathv.size() != 0){
                        spdc_paste_curve_as_freehand_shape(previous_shape_pathv, dc, item);
                        shape_applied = true;
                        shape = CLIPBOARD;
                    } else{
                        shape = NONE;
                    }
                } else {
                    if(bend_item != nullptr && bend_item->getRepr() != nullptr){
                        gchar const *svgd = item->getRepr()->attribute("d");
                        dc->selection->add(SP_OBJECT(bend_item));
                        dc->selection->duplicate();
                        dc->selection->remove(SP_OBJECT(bend_item));
                        bend_item = dc->selection->singleItem();
                        if(bend_item){
                            bend_item->moveTo(item,false);
                            Geom::Coord expansion_X = bend_item->transform.expansionX();
                            Geom::Coord expansion_Y = bend_item->transform.expansionY();
                            bend_item->transform = Geom::Affine(1,0,0,1,0,0);
                            bend_item->transform.setExpansionX(expansion_X);
                            bend_item->transform.setExpansionY(expansion_Y);
                            spdc_apply_bend_shape(svgd, dc, bend_item);
                            dc->selection->add(SP_OBJECT(bend_item));

                            shape = BEND_CLIPBOARD;
                        } else {
                            shape = NONE;
                        }
                    } else {
                        shape = NONE;
                    }
                }
                break;
            }
            default:
                break;
        }
        previous_shape_type = shape;

        if (shape_applied) {
            // apply original stroke color as fill and unset stroke; then return
            SPCSSAttr *css = sp_repr_css_attr_new();
            if (!strcmp(cfill, "none")) {
                sp_repr_css_set_property (css, "fill", cstroke);
            } else {
                sp_repr_css_set_property (css, "fill", cfill);
            }
            sp_repr_css_set_property (css, "stroke", "none");
            sp_desktop_apply_css_recursive(item, css, true);
            sp_repr_css_attr_unref(css);
            return;
        }
        if (dc->waiting_LPE_type != INVALID_LPE) {
            Effect::createAndApply(dc->waiting_LPE_type, dc->desktop->doc(), item);
            dc->waiting_LPE_type = INVALID_LPE;

            if (SP_IS_LPETOOL_CONTEXT(dc)) {
                // since a geometric LPE was applied, we switch back to "inactive" mode
                lpetool_context_switch_mode(SP_LPETOOL_CONTEXT(dc), INVALID_LPE);
            }
        }
        if (SP_IS_PEN_CONTEXT(dc)) {
            SP_PEN_CONTEXT(dc)->setPolylineMode();
        }
    }
}

/*
 * Selection handlers
 */

static void spdc_selection_changed(Inkscape::Selection *sel, FreehandBase *dc)
{
    if (dc->attach) {
        spdc_attach_selection(dc, sel);
    }
}

/* fixme: We have to ensure this is not delayed (Lauris) */

static void spdc_selection_modified(Inkscape::Selection *sel, guint /*flags*/, FreehandBase *dc)
{
    if (dc->attach) {
        spdc_attach_selection(dc, sel);
    }
}

static void spdc_attach_selection(FreehandBase *dc, Inkscape::Selection */*sel*/)
{
    if (SP_IS_PENCIL_CONTEXT(dc) && dc->sa) {
        return;
    }
    // We reset white and forget white/start/end anchors
    spdc_reset_white(dc);
    dc->sa = nullptr;
    dc->ea = nullptr;

    SPItem *item = dc->selection ? dc->selection->singleItem() : nullptr;

    if ( item && SP_IS_PATH(item) ) {
        // Create new white data
        // Item
        dc->white_item = item;

        // Curve list
        // We keep it in desktop coordinates to eliminate calculation errors
        SPCurve *norm = SP_PATH(item)->getCurveForEdit();
        norm->transform((dc->white_item)->i2dt_affine());
        g_return_if_fail( norm != nullptr );
        dc->white_curves = norm->split();
        norm->unref();

        // Anchor list
        for (auto c:dc->white_curves) {
            g_return_if_fail( c->get_segment_count() > 0 );
            if ( !c->is_closed() ) {
                SPDrawAnchor *a;
                a = sp_draw_anchor_new(dc, c, TRUE, *(c->first_point()));
                if (a)
                    dc->white_anchors.push_back(a);
                a = sp_draw_anchor_new(dc, c, FALSE, *(c->last_point()));
                if (a)
                    dc->white_anchors.push_back(a);
            }
        }
        // fixme: recalculate active anchor?
    }
}


void spdc_endpoint_snap_rotation(ToolBase const *const ec, Geom::Point &p, Geom::Point const &o,
                                 guint state)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    unsigned const snaps = abs(prefs->getInt("/options/rotationsnapsperpi/value", 12));

    SnapManager &m = ec->desktop->namedview->snap_manager;
    m.setup(ec->desktop);

    bool snap_enabled = m.snapprefs.getSnapEnabledGlobally();
    if (state & GDK_SHIFT_MASK) {
        // SHIFT disables all snapping, except the angular snapping. After all, the user explicitly asked for angular
        // snapping by pressing CTRL, otherwise we wouldn't have arrived here. But although we temporarily disable
        // the snapping here, we must still call for a constrained snap in order to apply the constraints (i.e. round
        // to the nearest angle increment)
        m.snapprefs.setSnapEnabledGlobally(false);
    }

    Inkscape::SnappedPoint dummy = m.constrainedAngularSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_NODE_HANDLE), boost::optional<Geom::Point>(), o, snaps);
    p = dummy.getPoint();

    if (state & GDK_SHIFT_MASK) {
        m.snapprefs.setSnapEnabledGlobally(snap_enabled); // restore the original setting
    }

    m.unSetup();
}


void spdc_endpoint_snap_free(ToolBase const * const ec, Geom::Point& p, boost::optional<Geom::Point> &start_of_line, guint const /*state*/)
{
    SPDesktop *dt = ec->desktop;
    SnapManager &m = dt->namedview->snap_manager;
    Inkscape::Selection *selection = dt->getSelection();

    // selection->singleItem() is the item that is currently being drawn. This item will not be snapped to (to avoid self-snapping)
    // TODO: Allow snapping to the stationary parts of the item, and only ignore the last segment

    m.setup(dt, true, selection->singleItem());
    Inkscape::SnapCandidatePoint scp(p, Inkscape::SNAPSOURCE_NODE_HANDLE);
    if (start_of_line) {
        scp.addOrigin(*start_of_line);
    }

    Inkscape::SnappedPoint sp = m.freeSnap(scp);
    p = sp.getPoint();

    m.unSetup();
}

static SPCurve *reverse_then_unref(SPCurve *orig)
{
    SPCurve *ret = orig->create_reverse();
    orig->unref();
    return ret;
}

void spdc_concat_colors_and_flush(FreehandBase *dc, gboolean forceclosed)
{
    // Concat RBG
    SPCurve *c = dc->green_curve;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    // Green
    dc->green_curve = new SPCurve();
    for (auto i : dc->green_bpaths)
        sp_canvas_item_destroy(i);
    dc->green_bpaths.clear();

    // Blue
    c->append_continuous(dc->blue_curve, 0.0625);
    dc->blue_curve->reset();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->blue_bpath), nullptr);

    // Red
    if (dc->red_curve_is_valid) {
        c->append_continuous(dc->red_curve, 0.0625);
    }
    dc->red_curve->reset();
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->red_bpath), nullptr);

    if (c->is_empty()) {
        c->unref();
        return;
    }

    // Step A - test, whether we ended on green anchor
    if ( (forceclosed && 
         (!dc->sa || (dc->sa && dc->sa->curve->is_empty()))) || 
         ( dc->green_anchor && dc->green_anchor->active)) 
    {
        // We hit green anchor, closing Green-Blue-Red
        dc->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Path is closed."));
        c->closepath_current();
        // Closed path, just flush
        spdc_flush_white(dc, c);
        c->unref();
        return;
    }

    // Step B - both start and end anchored to same curve
    if ( dc->sa && dc->ea
         && ( dc->sa->curve == dc->ea->curve )
         && ( ( dc->sa != dc->ea )
              || dc->sa->curve->is_closed() ) )
    {
        // We hit bot start and end of single curve, closing paths
        dc->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Closing path."));
        dc->sa_overwrited->append_continuous(c, 0.0625);
        c->unref();
        dc->sa_overwrited->closepath_current();
        if (!dc->white_curves.empty()) {
            dc->white_curves.erase(std::find(dc->white_curves.begin(),dc->white_curves.end(), dc->sa->curve));
        }
        dc->white_curves.push_back(dc->sa_overwrited);
        spdc_flush_white(dc, nullptr);
        return;
    }
    // Step C - test start
    if (dc->sa) {
        if (!dc->white_curves.empty()) {
            dc->white_curves.erase(std::find(dc->white_curves.begin(),dc->white_curves.end(), dc->sa->curve));
        }
        SPCurve *s = dc->sa_overwrited;
        s->append_continuous(c, 0.0625);
        c->unref();
        c = s;
    } else /* Step D - test end */ if (dc->ea) {
        SPCurve *e = dc->ea->curve;
        if (!dc->white_curves.empty()) {
            dc->white_curves.erase(std::find(dc->white_curves.begin(),dc->white_curves.end(), e));
        }
        if (!dc->ea->start) {
            e = reverse_then_unref(e);
        }
        if(prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 1 || 
            prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 2){
                e = reverse_then_unref(e);
                Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*e->last_segment());
                SPCurve *lastSeg = new SPCurve();
                if(cubic){
                    lastSeg->moveto((*cubic)[0]);
                    lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
                    if( e->get_segment_count() == 1){
                        e = lastSeg;
                    }else{
                        //we eliminate the last segment
                        e->backspace();
                        //and we add it again with the recreation
                        e->append_continuous(lastSeg, 0.0625);
                    }
                }
                e = reverse_then_unref(e);
        }
        c->append_continuous(e, 0.0625);
        e->unref();
    }
    if (forceclosed) 
    {
        dc->desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Path is closed."));
        c->closepath_current();
    }
    spdc_flush_white(dc, c);

    c->unref();
}

static void spdc_flush_white(FreehandBase *dc, SPCurve *gc)
{
    SPCurve *c;
    if (! dc->white_curves.empty()) {
        g_assert(dc->white_item);
        c = SPCurve::concat(dc->white_curves);
        dc->white_curves.clear();
        if (gc) {
            c->append(gc, FALSE);
        }
    } else if (gc) {
        c = gc;
        c->ref();
    } else {
        return;
    }

    // Now we have to go back to item coordinates at last
    c->transform( dc->white_item
                ? (dc->white_item)->dt2i_affine()
                : dc->desktop->dt2doc() );

    SPDesktop *desktop = dc->desktop;
    SPDocument *doc = desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    if ( c && !c->is_empty() ) {
        // We actually have something to write

        bool has_lpe = false;
        Inkscape::XML::Node *repr;

        if (dc->white_item) {
            repr = dc->white_item->getRepr();
            has_lpe = SP_LPE_ITEM(dc->white_item)->hasPathEffectRecursive();
        } else {
            repr = xml_doc->createElement("svg:path");
            // Set style
            sp_desktop_apply_style_tool(desktop, repr, tool_name(dc).data(), false);
        }

        gchar *str = sp_svg_write_path( c->get_pathvector() );
        g_assert( str != nullptr );
        if (has_lpe)
            repr->setAttribute("inkscape:original-d", str);
        else
            repr->setAttribute("d", str);
        g_free(str);

        if (SP_IS_PENCIL_CONTEXT(dc)) {
            if (dc->tablet_enabled) {
                if (!dc->white_item) {
                     dc->white_item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
                }
                spdc_check_for_and_apply_waiting_LPE(dc, dc->white_item, c, false);
                dc->selection->set(dc->white_item);
            }
        }
        if (!dc->white_item) {
            // Attach repr
            SPItem *item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
            //Bend needs the transforms applied after, Other effects best before
            spdc_check_for_and_apply_waiting_LPE(dc, item, c, true);
            Inkscape::GC::release(repr);
            item->transform = SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
            item->updateRepr();
            item->doWriteTransform(item->transform, nullptr, true);
            spdc_check_for_and_apply_waiting_LPE(dc, item, c, false);
            dc->selection->set(repr);
            if(previous_shape_type == BEND_CLIPBOARD){
                repr->parent()->removeChild(repr);
            }
        }
        DocumentUndo::done(doc, SP_IS_PEN_CONTEXT(dc)? SP_VERB_CONTEXT_PEN : SP_VERB_CONTEXT_PENCIL,
                         _("Draw path"));

        // When quickly drawing several subpaths with Shift, the next subpath may be finished and
        // flushed before the selection_modified signal is fired by the previous change, which
        // results in the tool losing all of the selected path's curve except that last subpath. To
        // fix this, we force the selection_modified callback now, to make sure the tool's curve is
        // in sync immediately.
        spdc_selection_modified(desktop->getSelection(), 0, dc);
    }

    c->unref();

    // Flush pending updates
    doc->ensureUpToDate();
}

SPDrawAnchor *spdc_test_inside(FreehandBase *dc, Geom::Point p)
{
    SPDrawAnchor *active = nullptr;

    // Test green anchor
    if (dc->green_anchor) {
        active = sp_draw_anchor_test(dc->green_anchor, p, TRUE);
    }

    for (auto i:dc->white_anchors) {
        SPDrawAnchor *na = sp_draw_anchor_test(i, p, !active);
        if ( !active && na ) {
            active = na;
        }
    }
    return active;
}

static void spdc_reset_white(FreehandBase *dc)
{
    if (dc->white_item) {
        // We do not hold refcount
        dc->white_item = nullptr;
    }
    for (auto i: dc->white_curves)
        i->unref();
    dc->white_curves.clear();
    for (auto i:dc->white_anchors)
        sp_draw_anchor_destroy(i);
    dc->white_anchors.clear();
}

static void spdc_free_colors(FreehandBase *dc)
{
    // Red
    if (dc->red_bpath) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(dc->red_bpath));
        dc->red_bpath = nullptr;
    }
    if (dc->red_curve) {
        dc->red_curve = dc->red_curve->unref();
    }

    // Blue
    if (dc->blue_bpath) {
        sp_canvas_item_destroy(SP_CANVAS_ITEM(dc->blue_bpath));
        dc->blue_bpath = nullptr;
    }
    if (dc->blue_curve) {
        dc->blue_curve = dc->blue_curve->unref();
    }
    
    // Overwrite start anchor curve
    if (dc->sa_overwrited) {
        dc->sa_overwrited = dc->sa_overwrited->unref();
    }
    // Green
    for (auto i : dc->green_bpaths)
        sp_canvas_item_destroy(i);
    dc->green_bpaths.clear();
    if (dc->green_curve) {
        dc->green_curve = dc->green_curve->unref();
    }
    if (dc->green_anchor) {
        dc->green_anchor = sp_draw_anchor_destroy(dc->green_anchor);
    }

    // White
    if (dc->white_item) {
        // We do not hold refcount
        dc->white_item = nullptr;
    }
    for (auto i: dc->white_curves)
        i->unref();
    dc->white_curves.clear();
    for (auto i:dc->white_anchors)
        sp_draw_anchor_destroy(i);
    dc->white_anchors.clear();
}

void spdc_create_single_dot(ToolBase *ec, Geom::Point const &pt, char const *tool, guint event_state) {
    g_return_if_fail(!strcmp(tool, "/tools/freehand/pen") || !strcmp(tool, "/tools/freehand/pencil") 
            || !strcmp(tool, "/tools/calligraphic") );
    Glib::ustring tool_path = tool;

    SPDesktop *desktop = ec->desktop;
    Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
    repr->setAttribute("sodipodi:type", "arc");
    SPItem *item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
    Inkscape::GC::release(repr);

    // apply the tool's current style
    sp_desktop_apply_style_tool(desktop, repr, tool, false);

    // find out stroke width (TODO: is there an easier way??)
    double stroke_width = 3.0;
    gchar const *style_str = repr->attribute("style");
    if (style_str) {
        SPStyle style(SP_ACTIVE_DOCUMENT);
        style.mergeString(style_str);
        stroke_width = style.stroke_width.computed;
    }

    // unset stroke and set fill color to former stroke color
    gchar * str;
    str = strcmp(tool, "/tools/calligraphic") ? g_strdup_printf("fill:#%06x;stroke:none;", sp_desktop_get_color_tool(desktop, tool, false) >> 8)
        : g_strdup_printf("fill:#%06x;stroke:#%06x;", sp_desktop_get_color_tool(desktop, tool, true) >> 8, sp_desktop_get_color_tool(desktop, tool, false) >> 8);
    repr->setAttribute("style", str);
    g_free(str);

    // put the circle where the mouse click occurred and set the diameter to the
    // current stroke width, multiplied by the amount specified in the preferences
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Geom::Affine const i2d (item->i2dt_affine ());
    Geom::Point pp = pt * i2d.inverse();
    double rad = 0.5 * prefs->getDouble(tool_path + "/dot-size", 3.0);
    if (!strcmp(tool, "/tools/calligraphic"))
        rad = 0.0333 * prefs->getDouble(tool_path + "/width", 3.0) / desktop->current_zoom() / desktop->getDocument()->getDocumentScale()[Geom::X];
    if (event_state & GDK_MOD1_MASK) {
        // TODO: We vary the dot size between 0.5*rad and 1.5*rad, where rad is the dot size
        // as specified in prefs. Very simple, but it might be sufficient in practice. If not,
        // we need to devise something more sophisticated.
        double s = g_random_double_range(-0.5, 0.5);
        rad *= (1 + s);
    }
    if (event_state & GDK_SHIFT_MASK) {
        // double the point size
        rad *= 2;
    }

    sp_repr_set_svg_double (repr, "sodipodi:cx", pp[Geom::X]);
    sp_repr_set_svg_double (repr, "sodipodi:cy", pp[Geom::Y]);
    sp_repr_set_svg_double (repr, "sodipodi:rx", rad * stroke_width);
    sp_repr_set_svg_double (repr, "sodipodi:ry", rad * stroke_width);
    item->updateRepr();

    desktop->getSelection()->set(item);

    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Creating single dot"));
    DocumentUndo::done(desktop->getDocument(), SP_VERB_NONE, _("Create single dot"));
}

}
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
