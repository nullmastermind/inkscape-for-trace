// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Pencil and pen toolbars
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "pencil-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "desktop.h"
#include "display/curve.h"
#include "live_effects/lpe-bendpath.h"
#include "live_effects/lpe-bspline.h"
#include "live_effects/lpe-patternalongpath.h"
#include "live_effects/lpe-powerstroke.h"
#include "live_effects/lpe-simplify.h"
#include "live_effects/lpe-spiro.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "object/sp-shape.h"
#include "selection.h"
#include "ui/icon-names.h"
#include "ui/tools-switch.h"
#include "ui/tools/freehand-base.h"
#include "ui/tools/pen-tool.h"
#include "ui/tools/pencil-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/canvas.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"

using Inkscape::UI::UXManager;

namespace Inkscape {
namespace UI {
namespace Toolbar {
PencilToolbar::PencilToolbar(SPDesktop *desktop,
                             bool       pencil_mode)
    : Toolbar(desktop),
    _repr(nullptr),
    _freeze(false),
    _flatten_simplify(nullptr),
    _simplify(nullptr)
{
    auto prefs = Inkscape::Preferences::get();

    add_freehand_mode_toggle(pencil_mode);

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    if (pencil_mode) {
        /* Use pressure */
        {
            _pressure_item = add_toggle_button(_("Use pressure input"), _("Use pressure input"));
            _pressure_item->set_icon_name(INKSCAPE_ICON("draw-use-pressure"));
            bool pressure = prefs->getBool(freehand_tool_name() + "/pressure", false);
            _pressure_item->set_active(pressure);
            _pressure_item->signal_toggled().connect(sigc::mem_fun(*this, &PencilToolbar::use_pencil_pressure));
        }
        /* min pressure */
        {
            auto minpressure_val = prefs->getDouble("/tools/freehand/pencil/minpressure", 0);
            _minpressure_adj = Gtk::Adjustment::create(minpressure_val, 0, 100, 1, 0);
            _minpressure =
                Gtk::manage(new UI::Widget::SpinButtonToolItem("pencil-minpressure", _("Min:"), _minpressure_adj, 0, 0));
            _minpressure->set_tooltip_text(_("Min percent of pressure"));
            _minpressure->set_focus_widget(desktop->canvas);
            _minpressure_adj->signal_value_changed().connect(
                sigc::mem_fun(*this, &PencilToolbar::minpressure_value_changed));
            add(*_minpressure);
        }
        /* max pressure */
        {
            auto maxpressure_val = prefs->getDouble("/tools/freehand/pencil/maxpressure", 30);
            _maxpressure_adj = Gtk::Adjustment::create(maxpressure_val, 0, 100, 1, 0);
            _maxpressure =
                Gtk::manage(new UI::Widget::SpinButtonToolItem("pencil-maxpressure", _("Max:"), _maxpressure_adj, 0, 0));
            _maxpressure->set_tooltip_text(_("Max percent of pressure"));
            _maxpressure->set_focus_widget(desktop->canvas);
            _maxpressure_adj->signal_value_changed().connect(
                sigc::mem_fun(*this, &PencilToolbar::maxpressure_value_changed));
            add(*_maxpressure);
        }

        /* powerstoke */
        add_powerstroke_cap(pencil_mode);

        add(*Gtk::manage(new Gtk::SeparatorToolItem()));

        /* Tolerance */
        {
            std::vector<Glib::ustring> labels = { _("(many nodes, rough)"), _("(default)"), "", "", "", "",
                                                  _("(few nodes, smooth)") };
            std::vector<double> values = { 1, 10, 20, 30, 50, 75, 100 };
            auto tolerance_val = prefs->getDouble("/tools/freehand/pencil/tolerance", 3.0);
            _tolerance_adj = Gtk::Adjustment::create(tolerance_val, 0, 100.0, 0.5, 1.0);
            auto tolerance_item =
                Gtk::manage(new UI::Widget::SpinButtonToolItem("pencil-tolerance", _("Smoothing:"), _tolerance_adj, 1, 2));
            tolerance_item->set_tooltip_text(_("How much smoothing (simplifying) is applied to the line"));
            tolerance_item->set_custom_numeric_menu_data(values, labels);
            tolerance_item->set_focus_widget(desktop->canvas);
            _tolerance_adj->signal_value_changed().connect(sigc::mem_fun(*this, &PencilToolbar::tolerance_value_changed));
            // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
            add(*tolerance_item);
        }

        /* LPE simplify based tolerance */
        {
            _simplify = add_toggle_button(_("LPE based interactive simplify"), _("LPE based interactive simplify"));
            _simplify->set_icon_name(INKSCAPE_ICON("interactive_simplify"));
            _simplify->set_active(prefs->getInt("/tools/freehand/pencil/simplify", 0));
            _simplify->signal_toggled().connect(sigc::mem_fun(*this, &PencilToolbar::simplify_lpe));
        }

        /* LPE simplify flatten */
        {
            _flatten_simplify = Gtk::manage(new Gtk::ToolButton(_("LPE simplify flatten")));
            _flatten_simplify->set_tooltip_text(_("LPE simplify flatten"));
            _flatten_simplify->set_icon_name(INKSCAPE_ICON("flatten"));
            _flatten_simplify->signal_clicked().connect(sigc::mem_fun(*this, &PencilToolbar::simplify_flatten));
            add(*_flatten_simplify);
        }

        add(*Gtk::manage(new Gtk::SeparatorToolItem()));
    }

    /* advanced shape options */
    add_advanced_shape_options(pencil_mode);

    show_all();

    // Elements must be hidden after show_all() is called
    guint freehandMode = prefs->getInt(( pencil_mode ?
                                         "/tools/freehand/pencil/freehand-mode" :
                                         "/tools/freehand/pen/freehand-mode" ), 0);
    if (freehandMode != 1 && freehandMode != 2) {
        _flatten_spiro_bspline->set_visible(false);
    }
    if (pencil_mode) {
        use_pencil_pressure();
    }
}

GtkWidget *
PencilToolbar::create_pencil(SPDesktop *desktop)
{
    auto toolbar = new PencilToolbar(desktop, true);
    return GTK_WIDGET(toolbar->gobj());
}

PencilToolbar::~PencilToolbar()
{
    if(_repr) {
        _repr->removeListenerByData(this);
        GC::release(_repr);
        _repr = nullptr;
    }
}

void
PencilToolbar::mode_changed(int mode)
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setInt(freehand_tool_name() + "/freehand-mode", mode);

    if (mode == 1 || mode == 2) {
        _flatten_spiro_bspline->set_visible(true);
    } else {
        _flatten_spiro_bspline->set_visible(false);
    }

    bool visible = (mode != 2);

    if (_simplify) {
        _simplify->set_visible(visible);
        if (_flatten_simplify) {
            _flatten_simplify->set_visible(visible && _simplify->get_active());
        }
    }

    // Recall, the PencilToolbar is also used as the PenToolbar with minor changes.
    auto *pt = dynamic_cast<Inkscape::UI::Tools::PenTool *>(_desktop->event_context);
    if (pt) {
        pt->setPolylineMode();
    }
}

/* This is used in generic functions below to share large portions of code between pen and pencil tool */
Glib::ustring const
PencilToolbar::freehand_tool_name()
{
    auto *pt = dynamic_cast<Inkscape::UI::Tools::PencilTool *>(_desktop->event_context);
    return ( pt ? "/tools/freehand/pencil" : "/tools/freehand/pen");
}

void
PencilToolbar::add_freehand_mode_toggle(bool tool_is_pencil)
{
    auto label = Gtk::manage(new UI::Widget::LabelToolItem(_("Mode:")));
    label->set_tooltip_text(_("Mode of new lines drawn by this tool"));
    add(*label);
    /* Freehand mode toggle buttons */
    Gtk::RadioToolButton::Group mode_group;
    auto bezier_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Bezier")));
    bezier_mode_btn->set_tooltip_text(_("Create regular Bezier path"));
    bezier_mode_btn->set_icon_name(INKSCAPE_ICON("path-mode-bezier"));
    _mode_buttons.push_back(bezier_mode_btn);

    auto spiro_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Spiro")));
    spiro_mode_btn->set_tooltip_text(_("Create Spiro path"));
    spiro_mode_btn->set_icon_name(INKSCAPE_ICON("path-mode-spiro"));
    _mode_buttons.push_back(spiro_mode_btn);

    auto bspline_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("BSpline")));
    bspline_mode_btn->set_tooltip_text(_("Create BSpline path"));
    bspline_mode_btn->set_icon_name(INKSCAPE_ICON("path-mode-bspline"));
    _mode_buttons.push_back(bspline_mode_btn);

    if (!tool_is_pencil) {
        auto zigzag_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Zigzag")));
        zigzag_mode_btn->set_tooltip_text(_("Create a sequence of straight line segments"));
        zigzag_mode_btn->set_icon_name(INKSCAPE_ICON("path-mode-polyline"));
        _mode_buttons.push_back(zigzag_mode_btn);

        auto paraxial_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("Paraxial")));
        paraxial_mode_btn->set_tooltip_text(_("Create a sequence of paraxial line segments"));
        paraxial_mode_btn->set_icon_name(INKSCAPE_ICON("path-mode-polyline-paraxial"));
        _mode_buttons.push_back(paraxial_mode_btn);
    }

    int btn_idx = 0;
    for (auto btn : _mode_buttons) {
        btn->set_sensitive(true);
        add(*btn);
        btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PencilToolbar::mode_changed), btn_idx++));
    }

    auto prefs = Inkscape::Preferences::get();

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* LPE bspline spiro flatten */
    _flatten_spiro_bspline = Gtk::manage(new Gtk::ToolButton(_("Flatten Spiro or BSpline LPE")));
    _flatten_spiro_bspline->set_tooltip_text(_("Flatten Spiro or BSpline LPE"));
    _flatten_spiro_bspline->set_icon_name(INKSCAPE_ICON("flatten"));
    _flatten_spiro_bspline->signal_clicked().connect(sigc::mem_fun(*this, &PencilToolbar::flatten_spiro_bspline));
    add(*_flatten_spiro_bspline);

    guint freehandMode = prefs->getInt(( tool_is_pencil ?
                                         "/tools/freehand/pencil/freehand-mode" :
                                         "/tools/freehand/pen/freehand-mode" ), 0);
    // freehandMode range is (0,5] for the pen tool, (0,3] for the pencil tool
    // freehandMode = 3 is an old way of signifying pressure, set it to 0.
    _mode_buttons[(freehandMode < _mode_buttons.size()) ? freehandMode : 0]->set_active();
}

void
PencilToolbar::minpressure_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/freehand/pencil/minpressure", _minpressure_adj->get_value());
}

void
PencilToolbar::maxpressure_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble( "/tools/freehand/pencil/maxpressure", _maxpressure_adj->get_value());
}

void
PencilToolbar::shapewidth_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Inkscape::Selection *selection = _desktop->getSelection();
    SPItem *item = selection->singleItem();
    SPLPEItem *lpeitem = nullptr;
    if (item) {
        lpeitem = dynamic_cast<SPLPEItem *>(item);
    }
    using namespace Inkscape::LivePathEffect;
    double width = _shapescale_adj->get_value();
    switch (_shape_item->get_active()) {
        case Inkscape::UI::Tools::TRIANGLE_IN:
        case Inkscape::UI::Tools::TRIANGLE_OUT:
            prefs->setDouble("/live_effects/powerstroke/width", width);
            if (lpeitem) {
                LPEPowerStroke *effect = dynamic_cast<LPEPowerStroke *>(lpeitem->getPathEffectOfType(POWERSTROKE));
                if (effect) {
                    std::vector<Geom::Point> points = effect->offset_points.data();
                    if (points.size() == 1) {
                        points[0][Geom::Y] = width;
                        effect->offset_points.param_set_and_write_new_value(points);
                    }
                }
            }
            break;
        case Inkscape::UI::Tools::ELLIPSE:
        case Inkscape::UI::Tools::CLIPBOARD:
            // The scale of the clipboard isn't known, so getting it to the right size isn't possible.
            prefs->setDouble("/live_effects/skeletal/width", width);
            if (lpeitem) {
                LPEPatternAlongPath *effect =
                    dynamic_cast<LPEPatternAlongPath *>(lpeitem->getPathEffectOfType(PATTERN_ALONG_PATH));
                if (effect) {
                    effect->prop_scale.param_set_value(width);
                    sp_lpe_item_update_patheffect(lpeitem, false, true);
                }
            }
            break;
        case Inkscape::UI::Tools::BEND_CLIPBOARD:
            prefs->setDouble("/live_effects/bend_path/width", width);
            if (lpeitem) {
                LPEBendPath *effect = dynamic_cast<LPEBendPath *>(lpeitem->getPathEffectOfType(BEND_PATH));
                if (effect) {
                    effect->prop_scale.param_set_value(width);
                    sp_lpe_item_update_patheffect(lpeitem, false, true);
                }
            }
            break;
        case Inkscape::UI::Tools::NONE:
        case Inkscape::UI::Tools::LAST_APPLIED:
        default:
            break;
    }
}

void
PencilToolbar::use_pencil_pressure() {
    // assumes called by pencil toolbar (and all these widgets exist)
    bool pressure = _pressure_item->get_active();
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool(freehand_tool_name() + "/pressure", pressure);
    if (pressure) {
        _minpressure->set_visible(true);
        _maxpressure->set_visible(true);
        _cap_item->set_visible(true);
        _shape_item->set_visible(false);
        _shapescale->set_visible(false);
        _simplify->set_visible(false);
        _flatten_spiro_bspline->set_visible(false);
        _flatten_simplify->set_visible(false);
        for (auto button : _mode_buttons) {
            button->set_sensitive(false);
        }
    } else {
        guint freehandMode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);

        _minpressure->set_visible(false);
        _maxpressure->set_visible(false);
        _cap_item->set_visible(false);
        _shape_item->set_visible(true);
        _shapescale->set_visible(true);
        bool simplify_visible = freehandMode != 2;
        _simplify->set_visible(simplify_visible);
        _flatten_simplify->set_visible(simplify_visible && _simplify->get_active());
        if (freehandMode == 1 || freehandMode == 2) {
            _flatten_spiro_bspline->set_visible(true);
        }
        for (auto button : _mode_buttons) {
            button->set_sensitive(true);
        }
    }
}

void
PencilToolbar::add_advanced_shape_options(bool tool_is_pencil)
{
    /*advanced shape options */
    UI::Widget::ComboToolItemColumns columns;

    Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

    std::vector<gchar*> freehand_shape_dropdown_items_list = {
        const_cast<gchar *>(C_("Freehand shape", "None")),
        _("Triangle in"),
        _("Triangle out"),
        _("Ellipse"),
        _("From clipboard"),
        _("Bend from clipboard"),
        _("Last applied")
    };

    for (auto item:freehand_shape_dropdown_items_list) {
        Gtk::TreeModel::Row row = *(store->append());
        row[columns.col_label    ] = item;
        row[columns.col_sensitive] = true;
    }

    _shape_item = Gtk::manage(UI::Widget::ComboToolItem::create(_("Shape"), _("Shape of new paths drawn by this tool"), "Not Used", store));
    _shape_item->use_group_label(true);

    auto prefs = Inkscape::Preferences::get();
    int shape = prefs->getInt((tool_is_pencil ?
                               "/tools/freehand/pencil/shape" :
                               "/tools/freehand/pen/shape" ), 0);
    _shape_item->set_active(shape);

    _shape_item->signal_changed().connect(sigc::mem_fun(*this, &PencilToolbar::change_shape));
    add(*_shape_item);

    /* power width setting */
    {
        _shapescale_adj = Gtk::Adjustment::create(2.0, 0.0, 1000.0, 0.5, 1.0);
        _shapescale =
            Gtk::manage(new UI::Widget::SpinButtonToolItem("pencil-maxpressure", _("Scale:"), _shapescale_adj, 1, 2));
        _shapescale->set_tooltip_text(_("Scale of the width of the power stroke shape."));
        _shapescale->set_focus_widget(_desktop->canvas);
        _shapescale_adj->signal_value_changed().connect(sigc::mem_fun(*this, &PencilToolbar::shapewidth_value_changed));
        update_width_value(shape);
        add(*_shapescale);

        _desktop->_tool_changed.connect(sigc::mem_fun(*this, &PencilToolbar::desktop_tool_changed));
    }
}

void
PencilToolbar::desktop_tool_changed(int tool) {
    int shape = 0;
    auto prefs = Inkscape::Preferences::get();
    switch (tool) {
        case TOOLS_FREEHAND_PENCIL:
            shape = prefs->getInt("/tools/freehand/pencil/shape", 0);
            break;
        case TOOLS_FREEHAND_PEN:
            shape = prefs->getInt("/tools/freehand/pen/shape", 0);
            break;
        default:
            return; // Bail out!
    }
    update_width_value(shape);
}

void
PencilToolbar::change_shape(int shape) {
    auto prefs = Inkscape::Preferences::get();
    prefs->setInt(freehand_tool_name() + "/shape", shape);
    update_width_value(shape);
}

void
PencilToolbar::update_width_value(int shape) {
    /* Update shape width with correct width */
    auto prefs = Inkscape::Preferences::get();
    double width = 1.0;
    _shapescale->set_sensitive(true);
    double powerstrokedefsize = 10 / (0.265 * _desktop->getDocument()->getDocumentScale()[0] * 2.0);
    switch (shape) {
        case Inkscape::UI::Tools::TRIANGLE_IN:
        case Inkscape::UI::Tools::TRIANGLE_OUT:
            width = prefs->getDouble("/live_effects/powerstroke/width", powerstrokedefsize);
            break;
        case Inkscape::UI::Tools::ELLIPSE:
        case Inkscape::UI::Tools::CLIPBOARD:
            width = prefs->getDouble("/live_effects/skeletal/width", 1.0);
            break;
        case Inkscape::UI::Tools::BEND_CLIPBOARD:
            width = prefs->getDouble("/live_effects/bend_path/width", 1.0);
            break;
        case Inkscape::UI::Tools::NONE: // Apply width from style?
        case Inkscape::UI::Tools::LAST_APPLIED:
        default:
            _shapescale->set_sensitive(false);
            break;
    }
    _shapescale_adj->set_value(width);
}

void PencilToolbar::add_powerstroke_cap(bool tool_is_pencil)
{
    /* Powerstroke cap */
    UI::Widget::ComboToolItemColumns columns;

    Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

    std::vector<gchar *> powerstroke_cap_items_list = { const_cast<gchar *>(C_("Cap", "Butt")), _("Square"), _("Round"),
                                                        _("Peak"), _("Zero width") };
    for (auto item : powerstroke_cap_items_list) {
        Gtk::TreeModel::Row row = *(store->append());
        row[columns.col_label] = item;
        row[columns.col_sensitive] = true;
    }

    _cap_item = Gtk::manage(UI::Widget::ComboToolItem::create(_("Caps"), _("Line endings when drawing with pressure-sensitive PowerPencil"), "Not Used", store));

    auto prefs = Inkscape::Preferences::get();

    int cap = prefs->getInt("/live_effects/powerstroke/powerpencilcap", 2);
    _cap_item->set_active(cap);
    _cap_item->use_group_label(true);

    _cap_item->signal_changed().connect(sigc::mem_fun(*this, &PencilToolbar::change_cap));

    add(*_cap_item);
}

void PencilToolbar::change_cap(int cap)
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setInt("/live_effects/powerstroke/powerpencilcap", cap);
}

void
PencilToolbar::simplify_lpe()
{
    bool simplify = _simplify->get_active();
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool(freehand_tool_name() + "/simplify", simplify);
    _flatten_simplify->set_visible(simplify);
}

void
PencilToolbar::simplify_flatten()
{
    auto selected = _desktop->getSelection()->items();
    SPLPEItem* lpeitem = nullptr;
    for (auto it(selected.begin()); it != selected.end(); ++it){
        lpeitem = dynamic_cast<SPLPEItem*>(*it);
        if (lpeitem && lpeitem->hasPathEffect()){
            PathEffectList lpelist = lpeitem->getEffectList();
            PathEffectList::iterator i;
            for (i = lpelist.begin(); i != lpelist.end(); ++i) {
                LivePathEffectObject *lpeobj = (*i)->lpeobject;
                if (lpeobj) {
                    Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                    if (dynamic_cast<Inkscape::LivePathEffect::LPESimplify *>(lpe)) {
                        SPShape * shape = dynamic_cast<SPShape *>(lpeitem);
                        if(shape){
                            auto c = SPCurve::copy(shape->curveForEdit());
                            lpe->doEffect(c.get());
                            lpeitem->setCurrentPathEffect(*i);
                            if (lpelist.size() > 1){
                                lpeitem->removeCurrentPathEffect(true);
                                shape->setCurveBeforeLPE(std::move(c));
                            } else {
                                lpeitem->removeCurrentPathEffect(false);
                                shape->setCurve(std::move(c));
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    if (lpeitem) {
        _desktop->getSelection()->remove(lpeitem->getRepr());
        _desktop->getSelection()->add(lpeitem->getRepr());
        sp_lpe_item_update_patheffect(lpeitem, false, false);
    }
}

void
PencilToolbar::flatten_spiro_bspline()
{
    auto selected = _desktop->getSelection()->items();
    SPLPEItem* lpeitem = nullptr;

    for (auto it(selected.begin()); it != selected.end(); ++it){
        lpeitem = dynamic_cast<SPLPEItem*>(*it);
        if (lpeitem && lpeitem->hasPathEffect()){
            PathEffectList lpelist = lpeitem->getEffectList();
            PathEffectList::iterator i;
            for (i = lpelist.begin(); i != lpelist.end(); ++i) {
                LivePathEffectObject *lpeobj = (*i)->lpeobject;
                if (lpeobj) {
                    Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                    if (dynamic_cast<Inkscape::LivePathEffect::LPEBSpline *>(lpe) || 
                        dynamic_cast<Inkscape::LivePathEffect::LPESpiro *>(lpe)) 
                    {
                        SPShape * shape = dynamic_cast<SPShape *>(lpeitem);
                        if(shape){
                            auto c = SPCurve::copy(shape->curveForEdit());
                            lpe->doEffect(c.get());
                            lpeitem->setCurrentPathEffect(*i);
                            if (lpelist.size() > 1){
                                lpeitem->removeCurrentPathEffect(true);
                                shape->setCurveBeforeLPE(std::move(c));
                            } else {
                                lpeitem->removeCurrentPathEffect(false);
                                shape->setCurve(std::move(c));
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    if (lpeitem) {
        _desktop->getSelection()->remove(lpeitem->getRepr());
        _desktop->getSelection()->add(lpeitem->getRepr());
        sp_lpe_item_update_patheffect(lpeitem, false, false);
    }
}

GtkWidget *
PencilToolbar::create_pen(SPDesktop *desktop)
{
    auto toolbar = new PencilToolbar(desktop, false);
    return GTK_WIDGET(toolbar->gobj());
}

void
PencilToolbar::tolerance_value_changed()
{
    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _freeze = true;
    prefs->setDouble("/tools/freehand/pencil/tolerance",
                     _tolerance_adj->get_value());
    _freeze = false;
    auto selected = _desktop->getSelection()->items();
    for (auto it(selected.begin()); it != selected.end(); ++it){
        SPLPEItem* lpeitem = dynamic_cast<SPLPEItem*>(*it);
        if (lpeitem && lpeitem->hasPathEffect()){
            Inkscape::LivePathEffect::Effect* simplify = lpeitem->getPathEffectOfType(Inkscape::LivePathEffect::SIMPLIFY);
            if(simplify){
                Inkscape::LivePathEffect::LPESimplify *lpe_simplify = dynamic_cast<Inkscape::LivePathEffect::LPESimplify*>(simplify->getLPEObj()->get_lpe());
                if (lpe_simplify) {
                    double tol = prefs->getDoubleLimited("/tools/freehand/pencil/tolerance", 10.0, 1.0, 100.0);
                    tol = tol/(100.0*(102.0-tol));
                    std::ostringstream ss;
                    ss << tol;
                    Inkscape::LivePathEffect::Effect* powerstroke = lpeitem->getPathEffectOfType(Inkscape::LivePathEffect::POWERSTROKE);
                    bool simplified = false;
                    if(powerstroke){
                        Inkscape::LivePathEffect::LPEPowerStroke *lpe_powerstroke = dynamic_cast<Inkscape::LivePathEffect::LPEPowerStroke*>(powerstroke->getLPEObj()->get_lpe());
                        if(lpe_powerstroke){
                            lpe_powerstroke->getRepr()->setAttribute("is_visible", "false");
                            sp_lpe_item_update_patheffect(lpeitem, false, false);
                            SPShape *sp_shape = dynamic_cast<SPShape *>(lpeitem);
                            if (sp_shape) {
                                guint previous_curve_length = sp_shape->curve()->get_segment_count();
                                lpe_simplify->getRepr()->setAttribute("threshold", ss.str());
                                sp_lpe_item_update_patheffect(lpeitem, false, false);
                                simplified = true;
                                guint curve_length = sp_shape->curve()->get_segment_count();
                                std::vector<Geom::Point> ts = lpe_powerstroke->offset_points.data();
                                double factor = (double)curve_length/ (double)previous_curve_length;
                                for (auto & t : ts) {
                                    t[Geom::X] = t[Geom::X] * factor;
                                }
                                lpe_powerstroke->offset_points.param_setValue(ts);
                            }
                            lpe_powerstroke->getRepr()->setAttribute("is_visible", "true");
                            sp_lpe_item_update_patheffect(lpeitem, false, false);
                        }
                    }
                    if(!simplified){
                        lpe_simplify->getRepr()->setAttribute("threshold", ss.str());
                    }
                }
            }
        }
    }
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
