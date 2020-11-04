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

#include "display/curve.h"
#include "display/control/canvas-item-bpath.h"

#include "include/macros.h"

#include "live_effects/lpe-bendpath.h"
#include "live_effects/lpe-patternalongpath.h"
#include "live_effects/lpe-simplify.h"
#include "live_effects/lpe-powerstroke.h"

#include "svg/svg-color.h"
#include "svg/svg.h"

#include "id-clash.h"
#include "object/sp-item-group.h"
#include "object/sp-path.h"
#include "object/sp-rect.h"
#include "object/sp-use.h"
#include "style.h"

#include "ui/clipboard.h"
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

FreehandBase::FreehandBase(const std::string& cursor_filename)
    : ToolBase(cursor_filename)
    , selection(nullptr)
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
    ungrabCanvasEvents();

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
    this->red_bpath = new Inkscape::CanvasItemBpath(desktop->getCanvasSketch());
    this->red_bpath->set_stroke(this->red_color);
    this->red_bpath->set_fill(0x0, SP_WIND_RULE_NONZERO);

    // Create red curve
    this->red_curve.reset(new SPCurve());

    // Create blue bpath
    this->blue_bpath = new Inkscape::CanvasItemBpath(desktop->getCanvasSketch());
    this->blue_bpath->set_stroke(this->blue_color);
    this->blue_bpath->set_fill(0x0, SP_WIND_RULE_NONZERO);

    // Create blue curve
    this->blue_curve.reset(new SPCurve());

    // Create green curve
    this->green_curve.reset(new SPCurve());

    // No green anchor by default
    this->green_anchor = nullptr;
    this->green_closed = FALSE;

    // Create start anchor alternative curve
    this->sa_overwrited.reset(new SPCurve());

    this->attach = TRUE;
    spdc_attach_selection(this, this->selection);
}

void FreehandBase::finish() {
    this->sel_changed_connection.disconnect();
    this->sel_modified_connection.disconnect();

    ungrabCanvasEvents();

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

boost::optional<Geom::Point> FreehandBase::red_curve_get_last_point()
{
    boost::optional<Geom::Point> p;
    if (!red_curve->is_empty()) {
        p = red_curve->last_point();
    }
    return p;
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
    SPDocument *document = dc->getDesktop()->doc();
    bool saved = DocumentUndo::getUndoSensitive(document);
    DocumentUndo::setUndoSensitive(document, false);
    Effect::createAndApply(PATTERN_ALONG_PATH, document, item);
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
    static_cast<LPEPatternAlongPath*>(lpe)->pattern.set_new_value(newpath,true);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double scale = prefs->getDouble("/live_effects/skeletal/width", 1);
    if (!scale) {
        scale = 1;
    }
    Inkscape::SVGOStringStream os;
    os << scale;
    lpe->getRepr()->setAttribute("prop_scale", os.str());
    DocumentUndo::setUndoSensitive(document, saved);
}

void spdc_apply_style(SPObject *obj)
{
    SPCSSAttr *css = sp_repr_css_attr_new();
    if (obj->style) {
        if (obj->style->stroke.isPaintserver()) {
            SPPaintServer *server = obj->style->getStrokePaintServer();
            if (server) {
                Glib::ustring str;
                str += "url(#";
                str += server->getId();
                str += ")";
                sp_repr_css_set_property(css, "fill", str.c_str());
            }
        } else if (obj->style->stroke.isColor()) {
            gchar c[64];
            sp_svg_write_color(
                c, sizeof(c),
                obj->style->stroke.value.color.toRGBA32(SP_SCALE24_TO_FLOAT(obj->style->stroke_opacity.value)));
            sp_repr_css_set_property(css, "fill", c);
        } else {
            sp_repr_css_set_property(css, "fill", "none");
        }
    } else {
        sp_repr_css_unset_property(css, "fill");
    }

    sp_repr_css_set_property(css, "fill-rule", "nonzero");
    sp_repr_css_set_property(css, "stroke", "none");

    sp_desktop_apply_css_recursive(obj, css, true);
    sp_repr_css_attr_unref(css);
}
static void spdc_apply_powerstroke_shape(std::vector<Geom::Point> points, FreehandBase *dc, SPItem *item,
                                         gint maxrecursion = 0)
{
    using namespace Inkscape::LivePathEffect;
    SPDesktop *desktop = dc->getDesktop();
    SPDocument *document = desktop->getDocument();
    if (!document || !desktop) {
        return;
    }
    if (SP_IS_PENCIL_CONTEXT(dc)) {
        if (dc->tablet_enabled) {
            SPObject *elemref = nullptr;
            if ((elemref = document->getObjectById("power_stroke_preview"))) {
                elemref->getRepr()->removeAttribute("style");
                SPItem *successor = dynamic_cast<SPItem *>(elemref);
                sp_desktop_apply_style_tool(desktop, successor->getRepr(),
                                            Glib::ustring("/tools/freehand/pencil").data(), false);
                spdc_apply_style(successor);
                sp_object_ref(item);
                item->deleteObject(false);
                item->setSuccessor(successor);
                sp_object_unref(item);
                item = dynamic_cast<SPItem *>(successor);
                dc->selection->set(item);
                item->setLocked(false);
                dc->white_item = item;
                rename_id(SP_OBJECT(item), "path-1");
            }
            return;
        }
    }
    bool saved = DocumentUndo::getUndoSensitive(document);
    DocumentUndo::setUndoSensitive(document, false);
    Effect::createAndApply(POWERSTROKE, document, item);
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();

    static_cast<LPEPowerStroke*>(lpe)->offset_points.param_set_and_write_new_value(points);

    // write powerstroke parameters:
    lpe->getRepr()->setAttribute("start_linecap_type", "zerowidth");
    lpe->getRepr()->setAttribute("end_linecap_type", "zerowidth");
    lpe->getRepr()->setAttribute("sort_points", "true");
    lpe->getRepr()->setAttribute("not_jump", "false");
    lpe->getRepr()->setAttribute("interpolator_type", "CubicBezierJohan");
    lpe->getRepr()->setAttribute("interpolator_beta", "0.2");
    lpe->getRepr()->setAttribute("miter_limit", "4");
    lpe->getRepr()->setAttribute("scale_width", "1");
    lpe->getRepr()->setAttribute("linejoin_type", "extrp_arc");
    DocumentUndo::setUndoSensitive(document, saved);
}

static void spdc_apply_bend_shape(gchar const *svgd, FreehandBase *dc, SPItem *item)
{
    using namespace Inkscape::LivePathEffect;
    SPUse *use = dynamic_cast<SPUse *>(item);
    if ( use ) {
        return;
    }
    SPDesktop *desktop = dc->getDesktop();
    SPDocument *document = desktop->getDocument();
    if (!document || !desktop) {
        return;
    }
    bool saved = DocumentUndo::getUndoSensitive(document);
    DocumentUndo::setUndoSensitive(document, false);
    if(!SP_IS_LPE_ITEM(item) || !SP_LPE_ITEM(item)->hasPathEffectOfType(BEND_PATH)){
        Effect::createAndApply(BEND_PATH, document, item);
    }
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();

    // write bend parameters:
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double scale = prefs->getDouble("/live_effects/bend_path/width", 1);
    if (!scale) {
        scale = 1;
    }
    Inkscape::SVGOStringStream os;
    os << scale;
    lpe->getRepr()->setAttribute("prop_scale", os.str());
    lpe->getRepr()->setAttribute("scale_y_rel", "false");
    lpe->getRepr()->setAttribute("vertical", "false");
    static_cast<LPEBendPath*>(lpe)->bend_path.paste_param_path(svgd);
    DocumentUndo::setUndoSensitive(document, saved);
}

static void spdc_apply_simplify(std::string threshold, FreehandBase *dc, SPItem *item)
{
    const SPDesktop *desktop = dc->getDesktop();
    SPDocument *document = desktop->getDocument();
    if (!document || !desktop) {
        return;
    }
    bool saved = DocumentUndo::getUndoSensitive(document);
    DocumentUndo::setUndoSensitive(document, false);
    using namespace Inkscape::LivePathEffect;

    Effect::createAndApply(SIMPLIFY, document, item);
    Effect* lpe = SP_LPE_ITEM(item)->getCurrentLPE();
    // write simplify parameters:
    lpe->getRepr()->setAttribute("steps", "1");
    lpe->getRepr()->setAttributeOrRemoveIfEmpty("threshold", threshold);
    lpe->getRepr()->setAttribute("smooth_angles", "360");
    lpe->getRepr()->setAttribute("helper_size", "0");
    lpe->getRepr()->setAttribute("simplify_individual_paths", "false");
    lpe->getRepr()->setAttribute("simplify_just_coalesce", "false");
    DocumentUndo::setUndoSensitive(document, saved);
}

static shapeType previous_shape_type = NONE;

static void spdc_check_for_and_apply_waiting_LPE(FreehandBase *dc, SPItem *item, SPCurve *curve, bool is_bend)
{
    using namespace Inkscape::LivePathEffect;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    auto *desktop = dc->getDesktop();

    if (item && SP_IS_LPE_ITEM(item)) {
        double defsize = 10 / (0.265 * dc->getDesktop()->getDocument()->getDocumentScale()[0]);
#define SHAPE_LENGTH defsize
#define SHAPE_HEIGHT defsize
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
            cm->paste(desktop, true))
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
            Effect::createAndApply(SPIRO, dc->getDesktop()->getDocument(), item);
        }

        if (prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 2) {
            Effect::createAndApply(BSPLINE, dc->getDesktop()->getDocument(), item);
        }
        SPShape *sp_shape = dynamic_cast<SPShape *>(item);
        if (sp_shape) {
            curve = sp_shape->curve();
        }
        auto curveref = curve->ref();
        SPCSSAttr *css_item = sp_css_attr_from_object(item, SP_STYLE_FLAG_ALWAYS);
        const char *cstroke = sp_repr_css_property(css_item, "stroke", "none");
        const char *cfill = sp_repr_css_property(css_item, "fill", "none");
        const char *stroke_width = sp_repr_css_property(css_item, "stroke-width", "0");
        double swidth;
        sp_svg_number_read_d(stroke_width, &swidth);
        swidth = prefs->getDouble("/live_effects/powerstroke/width", SHAPE_HEIGHT / 2);
        if (!swidth) {
            swidth = swidth/2;
        }
        swidth = std::abs(swidth);
        if (SP_IS_PENCIL_CONTEXT(dc)) {
            if (dc->tablet_enabled) {
                std::vector<Geom::Point> points;
                spdc_apply_powerstroke_shape(points, dc, item);
                shape_applied = true;
                shape = NONE;
                previous_shape_type = NONE;
            }
        }

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
                guint curve_length = curveref->get_segment_count();
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
                auto c = std::make_unique<SPCurve>();
                const double C1 = 0.552;
                c->moveto(0, SHAPE_HEIGHT/2);
                c->curveto(0, (1 - C1) * SHAPE_HEIGHT/2, (1 - C1) * SHAPE_LENGTH/2, 0, SHAPE_LENGTH/2, 0);
                c->curveto((1 + C1) * SHAPE_LENGTH/2, 0, SHAPE_LENGTH, (1 - C1) * SHAPE_HEIGHT/2, SHAPE_LENGTH, SHAPE_HEIGHT/2);
                c->curveto(SHAPE_LENGTH, (1 + C1) * SHAPE_HEIGHT/2, (1 + C1) * SHAPE_LENGTH/2, SHAPE_HEIGHT, SHAPE_LENGTH/2, SHAPE_HEIGHT);
                c->curveto((1 - C1) * SHAPE_LENGTH/2, SHAPE_HEIGHT, 0, (1 + C1) * SHAPE_HEIGHT/2, 0, SHAPE_HEIGHT/2);
                c->closepath();
                spdc_paste_curve_as_freehand_shape(c->get_pathvector(), dc, item);

                shape_applied = true;
                break;
            }
            case CLIPBOARD:
            {
                // take shape from clipboard;
                Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
                if(cm->paste(desktop,true)){
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
            sp_desktop_apply_css_recursive(dc->white_item, css, true);
            sp_repr_css_attr_unref(css);
            return;
        }
        if (dc->waiting_LPE_type != INVALID_LPE) {
            Effect::createAndApply(dc->waiting_LPE_type, dc->getDesktop()->getDocument(), item);
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
        auto path = static_cast<SPPath *>(item);
        auto norm = SPCurve::copy(path->curveForEdit());
        norm->transform((dc->white_item)->i2dt_affine());
        g_return_if_fail( norm != nullptr );
        dc->white_curves = norm->split();

        // Anchor list
        for (auto const &c_smart_ptr : dc->white_curves) {
            auto *c = c_smart_ptr.get();
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


void spdc_endpoint_snap_rotation(ToolBase* const ec, Geom::Point &p, Geom::Point const &o,
                                 guint state)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    unsigned const snaps = abs(prefs->getInt("/options/rotationsnapsperpi/value", 12));

    SnapManager &m = ec->getDesktop()->namedview->snap_manager;
    m.setup(ec->getDesktop());

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


void spdc_endpoint_snap_free(ToolBase* const ec, Geom::Point& p, boost::optional<Geom::Point> &start_of_line, guint const /*state*/)
{
    const SPDesktop *dt = ec->getDesktop();
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

void spdc_concat_colors_and_flush(FreehandBase *dc, gboolean forceclosed)
{
    // Concat RBG
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Green
    auto c = std::make_unique<SPCurve>();
    std::swap(c, dc->green_curve);
    for (auto path : dc->green_bpaths) {
        delete path;
    }
    dc->green_bpaths.clear();

    // Blue
    c->append_continuous(*dc->blue_curve);
    dc->blue_curve->reset();
    dc->blue_bpath->set_bpath(nullptr);

    // Red
    if (dc->red_curve_is_valid) {
        c->append_continuous(*(dc->red_curve));
    }
    dc->red_curve->reset();
    dc->red_bpath->set_bpath(nullptr);

    if (c->is_empty()) {
        return;
    }

    // Step A - test, whether we ended on green anchor
    if ( (forceclosed && 
         (!dc->sa || (dc->sa && dc->sa->curve->is_empty()))) || 
         ( dc->green_anchor && dc->green_anchor->active)) 
    {
        // We hit green anchor, closing Green-Blue-Red
        dc->getDesktop()->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Path is closed."));
        c->closepath_current();
        // Closed path, just flush
        spdc_flush_white(dc, c.get());
        return;
    }

    // Step B - both start and end anchored to same curve
    if ( dc->sa && dc->ea
         && ( dc->sa->curve == dc->ea->curve )
         && ( ( dc->sa != dc->ea )
              || dc->sa->curve->is_closed() ) )
    {
        // We hit bot start and end of single curve, closing paths
        dc->getDesktop()->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Closing path."));
        dc->sa_overwrited->append_continuous(*c);
        dc->sa_overwrited->closepath_current();
        if (!dc->white_curves.empty()) {
            dc->white_curves.erase(std::find(dc->white_curves.begin(),dc->white_curves.end(), dc->sa->curve));
        }
        dc->white_curves.push_back(std::move(dc->sa_overwrited));
        spdc_flush_white(dc, nullptr);
        return;
    }
    // Step C - test start
    if (dc->sa) {
        if (!dc->white_curves.empty()) {
            dc->white_curves.erase(std::find(dc->white_curves.begin(),dc->white_curves.end(), dc->sa->curve));
        }
        dc->sa_overwrited->append_continuous(*c);
        c = std::move(dc->sa_overwrited);
    } else /* Step D - test end */ if (dc->ea) {
        auto e = std::move(dc->ea->curve);
        if (!dc->white_curves.empty()) {
            dc->white_curves.erase(std::find(dc->white_curves.begin(),dc->white_curves.end(), e));
        }
        if (!dc->ea->start) {
            e = e->create_reverse();
        }
        if(prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 1 || 
            prefs->getInt(tool_name(dc) + "/freehand-mode", 0) == 2){
                e = e->create_reverse();
                Geom::CubicBezier const * cubic = dynamic_cast<Geom::CubicBezier const*>(&*e->last_segment());
                if(cubic){
                    auto lastSeg = std::make_unique<SPCurve>();
                    lastSeg->moveto((*cubic)[0]);
                    lastSeg->curveto((*cubic)[1],(*cubic)[3],(*cubic)[3]);
                    if( e->get_segment_count() == 1){
                        e = std::move(lastSeg);
                    }else{
                        //we eliminate the last segment
                        e->backspace();
                        //and we add it again with the recreation
                        e->append_continuous(*lastSeg);
                    }
                }
                e = e->create_reverse();
        }
        c->append_continuous(*e);
    }
    if (forceclosed) 
    {
        dc->getDesktop()->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Path is closed."));
        c->closepath_current();
    }
    spdc_flush_white(dc, c.get());
}

static void spdc_flush_white(FreehandBase *dc, SPCurve *gc)
{
    std::unique_ptr<SPCurve> c;

    if (! dc->white_curves.empty()) {
        g_assert(dc->white_item);

        // c = concat(white_curves)
        c = std::make_unique<SPCurve>();
        for (auto const &wc : dc->white_curves) {
            c->append(*wc);
        }

        dc->white_curves.clear();
        if (gc) {
            c->append(*gc);
        }
    } else if (gc) {
        c = gc->ref();
    } else {
        return;
    }

    SPDesktop *desktop = dc->getDesktop();
    SPDocument *doc = desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // Now we have to go back to item coordinates at last
    c->transform( dc->white_item
                ? (dc->white_item)->dt2i_affine()
                :  desktop->dt2doc() );

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

        auto str = sp_svg_write_path(c->get_pathvector());
        if (has_lpe)
            repr->setAttribute("inkscape:original-d", str);
        else
            repr->setAttribute("d", str);

        if (SP_IS_PENCIL_CONTEXT(dc) && dc->tablet_enabled) {
            if (!dc->white_item) {
                dc->white_item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
            }
            spdc_check_for_and_apply_waiting_LPE(dc, dc->white_item, c.get(), false);
        }
        if (!dc->white_item) {
            // Attach repr
            SPItem *item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
            dc->white_item = item;
            //Bend needs the transforms applied after, Other effects best before
            spdc_check_for_and_apply_waiting_LPE(dc, item, c.get(), true);
            Inkscape::GC::release(repr);
            item->transform = SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
            item->updateRepr();
            item->doWriteTransform(item->transform, nullptr, true);
            spdc_check_for_and_apply_waiting_LPE(dc, item, c.get(), false);
            if(previous_shape_type == BEND_CLIPBOARD){
                repr->parent()->removeChild(repr);
            } else {
                dc->selection->set(repr);
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
    dc->white_curves.clear();
    for (auto i:dc->white_anchors)
        sp_draw_anchor_destroy(i);
    dc->white_anchors.clear();
}

static void spdc_free_colors(FreehandBase *dc)
{
    // Red
    if (dc->red_bpath) {
        delete dc->red_bpath;
        dc->red_bpath = nullptr;
    }
    dc->red_curve.reset();

    // Blue
    if (dc->blue_bpath) {
        delete dc->blue_bpath;
        dc->blue_bpath = nullptr;
    }
    dc->blue_curve.reset();
    
    // Overwrite start anchor curve
    dc->sa_overwrited.reset();
    // Green
    for (auto path : dc->green_bpaths) {
        delete path;
    }
    dc->green_bpaths.clear();
    dc->green_curve.reset();
    if (dc->green_anchor) {
        dc->green_anchor = sp_draw_anchor_destroy(dc->green_anchor);
    }

    // White
    if (dc->white_item) {
        // We do not hold refcount
        dc->white_item = nullptr;
    }
    dc->white_curves.clear();
    for (auto i : dc->white_anchors)
        sp_draw_anchor_destroy(i);
    dc->white_anchors.clear();
}

void spdc_create_single_dot(ToolBase *ec, Geom::Point const &pt, char const *tool, guint event_state) {
    g_return_if_fail(!strcmp(tool, "/tools/freehand/pen") || !strcmp(tool, "/tools/freehand/pencil") 
            || !strcmp(tool, "/tools/calligraphic") );
    Glib::ustring tool_path = tool;

    SPDesktop *desktop = ec->getDesktop();
    Inkscape::XML::Document *xml_doc = desktop->doc()->getReprDoc();
    Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
    repr->setAttribute("sodipodi:type", "arc");
    SPItem *item = SP_ITEM(desktop->currentLayer()->appendChildRepr(repr));
    item->transform = SP_ITEM(desktop->currentLayer())->i2doc_affine().inverse();
    Inkscape::GC::release(repr);

    // apply the tool's current style
    sp_desktop_apply_style_tool(desktop, repr, tool, false);

    // find out stroke width (TODO: is there an easier way??)
    double stroke_width = 3.0;
    gchar const *style_str = repr->attribute("style");
    if (style_str) {
        SPStyle style(desktop->doc());
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
    item->doWriteTransform(item->transform, nullptr, true);

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
