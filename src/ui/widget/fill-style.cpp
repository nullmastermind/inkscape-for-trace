// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Fill style widget.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#define noSP_FS_VERBOSE

#include <glibmm/i18n.h>

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "fill-style.h"
#include "gradient-chemistry.h"
#include "inkscape.h"
#include "selection.h"
#include "verbs.h"

#include "object/sp-defs.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-pattern.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-text.h"
#include "style.h"

#include "ui/widget/canvas.h"  // Forced redraws

// These can be deleted once we sort out the libart dependence.

#define ART_WIND_RULE_NONZERO 0

/* Fill */

namespace Inkscape {
namespace UI {
namespace Widget {

FillNStroke::FillNStroke(FillOrStroke k)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , kind(k)
    , selectChangedConn()
    , subselChangedConn()
    , selectModifiedConn()
    , eventContextConn()
{
    // Add and connect up the paint selector widget:
    _psel = Gtk::manage(new UI::Widget::PaintSelector(kind));
    _psel->show();
    add(*_psel);
    _psel->signal_mode_changed().connect(sigc::mem_fun(*this, &FillNStroke::paintModeChangeCB));
    _psel->signal_dragged().connect(sigc::mem_fun(*this, &FillNStroke::dragFromPaint));
    _psel->signal_changed().connect(sigc::mem_fun(*this, &FillNStroke::paintChangedCB));

    if (kind == FILL) {
        _psel->signal_fillrule_changed().connect(sigc::mem_fun(*this, &FillNStroke::setFillrule));
    }

    performUpdate();
}

FillNStroke::~FillNStroke()
{
    if (_drag_id) {
        g_source_remove(_drag_id);
        _drag_id = 0;
    }

    _psel = nullptr;
    selectModifiedConn.disconnect();
    subselChangedConn.disconnect();
    selectChangedConn.disconnect();
    eventContextConn.disconnect();
}

/**
 * On signal modified, invokes an update of the fill or stroke style paint object.
 */
void FillNStroke::selectionModifiedCB(guint flags)
{
    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
#ifdef SP_FS_VERBOSE
        g_message("selectionModifiedCB(%d) on %p", flags, this);
#endif
        performUpdate();
    }
}

void FillNStroke::setDesktop(SPDesktop *desktop)
{
    if (_desktop != desktop) {
        if (_drag_id) {
            g_source_remove(_drag_id);
            _drag_id = 0;
        }
        if (_desktop) {
            selectModifiedConn.disconnect();
            subselChangedConn.disconnect();
            selectChangedConn.disconnect();
            eventContextConn.disconnect();
        }
        _desktop = desktop;
        if (desktop && desktop->selection) {
            selectChangedConn =
                desktop->selection->connectChanged(sigc::hide(sigc::mem_fun(*this, &FillNStroke::performUpdate)));
            subselChangedConn =
                desktop->connectToolSubselectionChanged(sigc::hide(sigc::mem_fun(*this, &FillNStroke::performUpdate)));
            eventContextConn = desktop->connectEventContextChanged(sigc::hide(sigc::bind(
                sigc::mem_fun(*this, &FillNStroke::eventContextCB), (Inkscape::UI::Tools::ToolBase *)nullptr)));

            // Must check flags, so can't call performUpdate() directly.
            selectModifiedConn = desktop->selection->connectModified(
                sigc::hide<0>(sigc::mem_fun(*this, &FillNStroke::selectionModifiedCB)));
        }
        performUpdate();
    }
}

/**
 * Listen to this "change in tool" event, in case a subselection tool (such as Gradient or Node) selection
 * is changed back to a selection tool - especially needed for selected gradient stops.
 */
void FillNStroke::eventContextCB(SPDesktop * /*desktop*/, Inkscape::UI::Tools::ToolBase * /*eventcontext*/)
{
    performUpdate();
}


/**
 * Gets the active fill or stroke style property, then sets the appropriate
 * color, alpha, gradient, pattern, etc. for the paint-selector.
 *
 * @param sel Selection to use, or NULL.
 */
void FillNStroke::performUpdate()
{
    if (_update || !_desktop) {
        return;
    }

    if (_drag_id) {
        // local change; do nothing, but reset the flag
        g_source_remove(_drag_id);
        _drag_id = 0;
        return;
    }

    _update = true;

    // create temporary style
    SPStyle query(_desktop->doc());

    // query style from desktop into it. This returns a result flag and fills query with the style of subselection, if
    // any, or selection
    int result = sp_desktop_query_style(_desktop, &query,
                                        (kind == FILL) ? QUERY_STYLE_PROPERTY_FILL : QUERY_STYLE_PROPERTY_STROKE);

    SPIPaint &targPaint = *query.getFillOrStroke(kind == FILL);
    SPIScale24 &targOpacity = *(kind == FILL ? query.fill_opacity.upcast() : query.stroke_opacity.upcast());

    switch (result) {
        case QUERY_STYLE_NOTHING: {
            /* No paint at all */
            _psel->setMode(UI::Widget::PaintSelector::MODE_EMPTY);
            break;
        }

        case QUERY_STYLE_SINGLE:
        case QUERY_STYLE_MULTIPLE_AVERAGED: // TODO: treat this slightly differently, e.g. display "averaged" somewhere
                                            // in paint selector
        case QUERY_STYLE_MULTIPLE_SAME: {
            auto pselmode = UI::Widget::PaintSelector::getModeForStyle(query, kind);
            _psel->setMode(pselmode);

            if (kind == FILL) {
                _psel->setFillrule(query.fill_rule.computed == ART_WIND_RULE_NONZERO
                                   ? UI::Widget::PaintSelector::FILLRULE_NONZERO
                                   : UI::Widget::PaintSelector::FILLRULE_EVENODD);
            }

            if (targPaint.set && targPaint.isColor()) {
                _psel->setColorAlpha(targPaint.value.color, SP_SCALE24_TO_FLOAT(targOpacity.value));
            } else if (targPaint.set && targPaint.isPaintserver()) {

                SPPaintServer *server = (kind == FILL) ? query.getFillPaintServer() : query.getStrokePaintServer();

                if (server) {
                    if (SP_IS_GRADIENT(server) && SP_GRADIENT(server)->getVector()->isSwatch()) {
                        SPGradient *vector = SP_GRADIENT(server)->getVector();
                        _psel->setSwatch(vector);
                    } else if (SP_IS_LINEARGRADIENT(server)) {
                        SPGradient *vector = SP_GRADIENT(server)->getVector();
                        _psel->setGradientLinear(vector);

                        SPLinearGradient *lg = SP_LINEARGRADIENT(server);
                        _psel->setGradientProperties(lg->getUnits(), lg->getSpread());
                    } else if (SP_IS_RADIALGRADIENT(server)) {
                        SPGradient *vector = SP_GRADIENT(server)->getVector();
                        _psel->setGradientRadial(vector);

                        SPRadialGradient *rg = SP_RADIALGRADIENT(server);
                        _psel->setGradientProperties(rg->getUnits(), rg->getSpread());
#ifdef WITH_MESH
                    } else if (SP_IS_MESHGRADIENT(server)) {
                        SPGradient *array = SP_GRADIENT(server)->getArray();
                        _psel->setGradientMesh(SP_MESHGRADIENT(array));
                        _psel->updateMeshList(SP_MESHGRADIENT(array));
#endif
                    } else if (SP_IS_PATTERN(server)) {
                        SPPattern *pat = SP_PATTERN(server)->rootPattern();
                        _psel->updatePatternList(pat);
                    }
                }
            }
            break;
        }

        case QUERY_STYLE_MULTIPLE_DIFFERENT: {
            _psel->setMode(UI::Widget::PaintSelector::MODE_MULTIPLE);
            break;
        }
    }

    _update = false;
}

/**
 * When the mode is changed, invoke a regular changed handler.
 */
void FillNStroke::paintModeChangeCB(UI::Widget::PaintSelector::Mode /*mode*/)
{
#ifdef SP_FS_VERBOSE
    g_message("paintModeChangeCB()");
#endif
    if (!_update) {
        updateFromPaint();
    }
}

void FillNStroke::setFillrule(UI::Widget::PaintSelector::FillRule mode)
{
    if (!_update && _desktop) {
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, "fill-rule",
                                 (mode == UI::Widget::PaintSelector::FILLRULE_EVENODD) ? "evenodd" : "nonzero");

        sp_desktop_set_style(_desktop, css);

        sp_repr_css_attr_unref(css);
        css = nullptr;

        DocumentUndo::done(_desktop->doc(), SP_VERB_DIALOG_FILL_STROKE, _("Change fill rule"));
    }
}

static gchar const *undo_F_label_1 = "fill:flatcolor:1";
static gchar const *undo_F_label_2 = "fill:flatcolor:2";

static gchar const *undo_S_label_1 = "stroke:flatcolor:1";
static gchar const *undo_S_label_2 = "stroke:flatcolor:2";

static gchar const *undo_F_label = undo_F_label_1;
static gchar const *undo_S_label = undo_S_label_1;

gboolean FillNStroke::dragDelayCB(gpointer data)
{
    gboolean keepGoing = TRUE;
    if (data) {
        FillNStroke *self = reinterpret_cast<FillNStroke *>(data);
        if (!self->_update) {
            if (self->_drag_id) {
                g_source_remove(self->_drag_id);
                self->_drag_id = 0;

                self->dragFromPaint();
                self->performUpdate();
            }
            keepGoing = FALSE;
        }
    } else {
        keepGoing = FALSE;
    }
    return keepGoing;
}

/**
 * This is called repeatedly while you are dragging a color slider, only for flat color
 * modes. Previously it set the color in style but did not update the repr for efficiency, however
 * this was flakey and didn't buy us almost anything. So now it does the same as _changed, except
 * lumps all its changes for undo.
 */
void FillNStroke::dragFromPaint()
{
    if (!_desktop || _update) {
        return;
    }

    guint32 when = gtk_get_current_event_time();

    // Don't attempt too many updates per second.
    // Assume a base 15.625ms resolution on the timer.
    if (!_drag_id && _last_drag && when && ((when - _last_drag) < 32)) {
        // local change, do not update from selection
        _drag_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 33, dragDelayCB, this, nullptr);
    }

    if (_drag_id) {
        // previous local flag not cleared yet;
        // this means dragged events come too fast, so we better skip this one to speed up display
        // (it's safe to do this in any case)
        return;
    }
    _last_drag = when;

    _update = true;

    switch (_psel->get_mode()) {
        case UI::Widget::PaintSelector::MODE_SOLID_COLOR: {
            // local change, do not update from selection
            _drag_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, dragDelayCB, this, nullptr);
            _psel->setFlatColor(_desktop,
                                (kind == FILL) ? "fill" : "stroke",
                                (kind == FILL) ? "fill-opacity" : "stroke-opacity");
            DocumentUndo::maybeDone(_desktop->doc(), (kind == FILL) ? undo_F_label : undo_S_label,
                                    SP_VERB_DIALOG_FILL_STROKE,
                                    (kind == FILL) ? _("Set fill color") : _("Set stroke color"));
            break;
        }

        default:
            g_warning("file %s: line %d: Paint %d should not emit 'dragged'", __FILE__, __LINE__, _psel->get_mode());
            break;
    }
    _update = false;
}

/**
This is called (at least) when:
1  paint selector mode is switched (e.g. flat color -> gradient)
2  you finished dragging a gradient node and released mouse
3  you changed a gradient selector parameter (e.g. spread)
Must update repr.
 */
void FillNStroke::paintChangedCB()
{
#ifdef SP_FS_VERBOSE
    g_message("paintChangedCB()");
#endif
    if (!_update) {
        updateFromPaint();
    }
}

void FillNStroke::updateFromPaint()
{
    if (!_desktop) {
        return;
    }
    _update = true;

    auto document  = _desktop->getDocument();
    auto selection = _desktop->getSelection();

    std::vector<SPItem *> const items(selection->items().begin(), selection->items().end());

    switch (_psel->get_mode()) {
        case UI::Widget::PaintSelector::MODE_EMPTY:
            // This should not happen.
            g_warning("file %s: line %d: Paint %d should not emit 'changed'", __FILE__, __LINE__, _psel->get_mode());
            break;
        case UI::Widget::PaintSelector::MODE_MULTIPLE:
            // This happens when you switch multiple objects with different gradients to flat color;
            // nothing to do here.
            break;

        case UI::Widget::PaintSelector::MODE_NONE: {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property(css, (kind == FILL) ? "fill" : "stroke", "none");

            sp_desktop_set_style(_desktop, css);

            sp_repr_css_attr_unref(css);
            css = nullptr;

            DocumentUndo::done(document, SP_VERB_DIALOG_FILL_STROKE,
                               (kind == FILL) ? _("Remove fill") : _("Remove stroke"));
            break;
        }

        case UI::Widget::PaintSelector::MODE_SOLID_COLOR: {
            if (kind == FILL) {
                // FIXME: fix for GTK breakage, see comment in SelectedStyle::on_opacity_changed; here it results in
                // losing release events
                _desktop->getCanvas()->forced_redraws_start(0);
            }

            _psel->setFlatColor(_desktop, (kind == FILL) ? "fill" : "stroke",
                                (kind == FILL) ? "fill-opacity" : "stroke-opacity");
            DocumentUndo::maybeDone(_desktop->getDocument(), (kind == FILL) ? undo_F_label : undo_S_label,
                                    SP_VERB_DIALOG_FILL_STROKE,
                                    (kind == FILL) ? _("Set fill color") : _("Set stroke color"));

            if (kind == FILL) {
                // resume interruptibility
                _desktop->getCanvas()->forced_redraws_stop();
            }

            // on release, toggle undo_label so that the next drag will not be lumped with this one
            if (undo_F_label == undo_F_label_1) {
                undo_F_label = undo_F_label_2;
                undo_S_label = undo_S_label_2;
            } else {
                undo_F_label = undo_F_label_1;
                undo_S_label = undo_S_label_1;
            }

            break;
        }

        case UI::Widget::PaintSelector::MODE_GRADIENT_LINEAR:
        case UI::Widget::PaintSelector::MODE_GRADIENT_RADIAL:
        case UI::Widget::PaintSelector::MODE_SWATCH:
            if (!items.empty()) {
                SPGradientType const gradient_type =
                    (_psel->get_mode() != UI::Widget::PaintSelector::MODE_GRADIENT_RADIAL ? SP_GRADIENT_TYPE_LINEAR
                                                                                          : SP_GRADIENT_TYPE_RADIAL);
                bool createSwatch = (_psel->get_mode() == UI::Widget::PaintSelector::MODE_SWATCH);

                SPCSSAttr *css = nullptr;
                if (kind == FILL) {
                    // HACK: reset fill-opacity - that 0.75 is annoying; BUT remove this when we have an opacity slider
                    // for all tabs
                    css = sp_repr_css_attr_new();
                    sp_repr_css_set_property(css, "fill-opacity", "1.0");
                }

                auto vector = _psel->getGradientVector();
                if (!vector) {
                    /* No vector in paint selector should mean that we just changed mode */

                    SPStyle query(_desktop->doc());
                    int result = objects_query_fillstroke(items, &query, kind == FILL);
                    if (result == QUERY_STYLE_MULTIPLE_SAME) {
                        SPIPaint &targPaint = *query.getFillOrStroke(kind == FILL);
                        SPColor common;
                        if (!targPaint.isColor()) {
                            common = sp_desktop_get_color(_desktop, kind == FILL);
                        } else {
                            common = targPaint.value.color;
                        }
                        vector = sp_document_default_gradient_vector(document, common, createSwatch);
                        if (vector && createSwatch) {
                            vector->setSwatch();
                        }
                    }

                    for (auto item : items) {
                        // FIXME: see above
                        if (kind == FILL) {
                            sp_repr_css_change_recursive(item->getRepr(), css, "style");
                        }

                        if (!vector) {
                            auto gr = sp_gradient_vector_for_object(
                                document, _desktop, item,
                                (kind == FILL) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE, createSwatch);
                            if (gr && createSwatch) {
                                gr->setSwatch();
                            }
                            sp_item_set_gradient(item, gr, gradient_type,
                                                 (kind == FILL) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE);
                        } else {
                            sp_item_set_gradient(item, vector, gradient_type,
                                                 (kind == FILL) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE);
                        }
                    }
                } else {
                    // We have changed from another gradient type, or modified spread/units within
                    // this gradient type.
                    vector = sp_gradient_ensure_vector_normalized(vector);
                    for (auto item : items) {
                        // FIXME: see above
                        if (kind == FILL) {
                            sp_repr_css_change_recursive(item->getRepr(), css, "style");
                        }

                        SPGradient *gr = sp_item_set_gradient(
                            item, vector, gradient_type, (kind == FILL) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE);
                        _psel->pushAttrsToGradient(gr);
                    }
                }

                if (css) {
                    sp_repr_css_attr_unref(css);
                    css = nullptr;
                }

                DocumentUndo::done(document, SP_VERB_DIALOG_FILL_STROKE,
                                   (kind == FILL) ? _("Set gradient on fill") : _("Set gradient on stroke"));
            }
            break;

#ifdef WITH_MESH
        case UI::Widget::PaintSelector::MODE_GRADIENT_MESH:

            if (!items.empty()) {
                SPCSSAttr *css = nullptr;
                if (kind == FILL) {
                    // HACK: reset fill-opacity - that 0.75 is annoying; BUT remove this when we have an opacity slider
                    // for all tabs
                    css = sp_repr_css_attr_new();
                    sp_repr_css_set_property(css, "fill-opacity", "1.0");
                }

                Inkscape::XML::Document *xml_doc = document->getReprDoc();
                SPDefs *defs = document->getDefs();

                auto mesh = _psel->getMeshGradient();

                for (auto item : items) {

                    // FIXME: see above
                    if (kind == FILL) {
                        sp_repr_css_change_recursive(item->getRepr(), css, "style");
                    }

                    // Check if object already has mesh.
                    bool has_mesh = false;
                    SPStyle *style = item->style;
                    if (style) {
                        SPPaintServer *server =
                            (kind == FILL) ? style->getFillPaintServer() : style->getStrokePaintServer();
                        if (server && SP_IS_MESHGRADIENT(server))
                            has_mesh = true;
                    }

                    if (!mesh || !has_mesh) {
                        // No mesh in document or object does not already have mesh ->
                        // Create new mesh.

                        // Create mesh element
                        Inkscape::XML::Node *repr = xml_doc->createElement("svg:meshgradient");

                        // privates are garbage-collectable
                        repr->setAttribute("inkscape:collect", "always");

                        // Attach to document
                        defs->getRepr()->appendChild(repr);
                        Inkscape::GC::release(repr);

                        // Get corresponding object
                        SPMeshGradient *mg = static_cast<SPMeshGradient *>(document->getObjectByRepr(repr));
                        mg->array.create(mg, item, (kind == FILL) ? item->geometricBounds() : item->visualBounds());

                        bool isText = SP_IS_TEXT(item);
                        sp_style_set_property_url(item, ((kind == FILL) ? "fill" : "stroke"), mg, isText);

                        // (*i)->requestModified(SP_OBJECT_MODIFIED_FLAG|SP_OBJECT_STYLE_MODIFIED_FLAG);

                    } else {
                        // Using found mesh

                        // Duplicate
                        Inkscape::XML::Node *mesh_repr = mesh->getRepr();
                        Inkscape::XML::Node *copy_repr = mesh_repr->duplicate(xml_doc);

                        // privates are garbage-collectable
                        copy_repr->setAttribute("inkscape:collect", "always");

                        // Attach to document
                        defs->getRepr()->appendChild(copy_repr);
                        Inkscape::GC::release(copy_repr);

                        // Get corresponding object
                        SPMeshGradient *mg = static_cast<SPMeshGradient *>(document->getObjectByRepr(copy_repr));
                        // std::cout << "  " << (mg->getId()?mg->getId():"null") << std::endl;
                        mg->array.read(mg);

                        Geom::OptRect item_bbox = (kind == FILL) ? item->geometricBounds() : item->visualBounds();
                        mg->array.fill_box(item_bbox);

                        bool isText = SP_IS_TEXT(item);
                        sp_style_set_property_url(item, ((kind == FILL) ? "fill" : "stroke"), mg, isText);
                    }
                }

                if (css) {
                    sp_repr_css_attr_unref(css);
                    css = nullptr;
                }

                DocumentUndo::done(document, SP_VERB_DIALOG_FILL_STROKE,
                                   (kind == FILL) ? _("Set mesh on fill") : _("Set mesh on stroke"));
            }
            break;
#endif

        case UI::Widget::PaintSelector::MODE_PATTERN:

            if (!items.empty()) {

                auto pattern = _psel->getPattern();
                if (!pattern) {

                    /* No Pattern in paint selector should mean that we just
                     * changed mode - don't do jack.
                     */

                } else {
                    Inkscape::XML::Node *patrepr = pattern->getRepr();
                    SPCSSAttr *css = sp_repr_css_attr_new();
                    gchar *urltext = g_strdup_printf("url(#%s)", patrepr->attribute("id"));
                    sp_repr_css_set_property(css, (kind == FILL) ? "fill" : "stroke", urltext);

                    // HACK: reset fill-opacity - that 0.75 is annoying; BUT remove this when we have an opacity slider
                    // for all tabs
                    if (kind == FILL) {
                        sp_repr_css_set_property(css, "fill-opacity", "1.0");
                    }

                    // cannot just call sp_desktop_set_style, because we don't want to touch those
                    // objects who already have the same root pattern but through a different href
                    // chain. FIXME: move this to a sp_item_set_pattern
                    for (auto item : items) {
                        Inkscape::XML::Node *selrepr = item->getRepr();
                        if ((kind == STROKE) && !selrepr) {
                            continue;
                        }
                        SPObject *selobj = item;

                        SPStyle *style = selobj->style;
                        if (style && ((kind == FILL) ? style->fill.isPaintserver() : style->stroke.isPaintserver())) {
                            SPPaintServer *server = (kind == FILL) ? selobj->style->getFillPaintServer()
                                                                   : selobj->style->getStrokePaintServer();
                            if (SP_IS_PATTERN(server) && SP_PATTERN(server)->rootPattern() == pattern)
                                // only if this object's pattern is not rooted in our selected pattern, apply
                                continue;
                        }

                        if (kind == FILL) {
                            sp_desktop_apply_css_recursive(selobj, css, true);
                        } else {
                            sp_repr_css_change_recursive(selrepr, css, "style");
                        }
                    }

                    sp_repr_css_attr_unref(css);
                    css = nullptr;
                    g_free(urltext);

                } // end if

                DocumentUndo::done(document, SP_VERB_DIALOG_FILL_STROKE,
                                   (kind == FILL) ? _("Set pattern on fill") : _("Set pattern on stroke"));
            } // end if

            break;

        case UI::Widget::PaintSelector::MODE_UNSET:
            if (!items.empty()) {
                SPCSSAttr *css = sp_repr_css_attr_new();
                if (kind == FILL) {
                    sp_repr_css_unset_property(css, "fill");
                } else {
                    sp_repr_css_unset_property(css, "stroke");
                    sp_repr_css_unset_property(css, "stroke-opacity");
                    sp_repr_css_unset_property(css, "stroke-width");
                    sp_repr_css_unset_property(css, "stroke-miterlimit");
                    sp_repr_css_unset_property(css, "stroke-linejoin");
                    sp_repr_css_unset_property(css, "stroke-linecap");
                    sp_repr_css_unset_property(css, "stroke-dashoffset");
                    sp_repr_css_unset_property(css, "stroke-dasharray");
                }

                sp_desktop_set_style(_desktop, css);
                sp_repr_css_attr_unref(css);
                css = nullptr;

                DocumentUndo::done(document, SP_VERB_DIALOG_FILL_STROKE,
                                   (kind == FILL) ? _("Unset fill") : _("Unset stroke"));
            }
            break;

        default:
            g_warning("file %s: line %d: Paint selector should not be in "
                      "mode %d",
                      __FILE__, __LINE__, _psel->get_mode());
            break;
    }

    _update = false;
}

} // namespace Widget
} // namespace UI
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
