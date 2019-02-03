// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Measure aux toolbar
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

#include "measure-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"
#include "inkscape.h"
#include "message-stack.h"

#include "ui/icon-names.h"
#include "ui/tools/measure-tool.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/label-tool-item.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::MeasureTool;

/** Temporary hack: Returns the node tool in the active desktop.
 * Will go away during tool refactoring. */
static MeasureTool *get_measure_tool()
{
    MeasureTool *tool = nullptr;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (SP_IS_MEASURE_CONTEXT(ec)) {
            tool = static_cast<MeasureTool*>(ec);
        }
    }
    return tool;
}



namespace Inkscape {
namespace UI {
namespace Toolbar {
MeasureToolbar::MeasureToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
    _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
{
    auto prefs = Inkscape::Preferences::get();
    _tracker->setActiveUnitByAbbr(prefs->getString("/tools/measure/unit").c_str());

    /* Font Size */
    {
        auto font_size_val = prefs->getDouble("/tools/measure/fontsize", 10.0);
        _font_size_adj = Gtk::Adjustment::create(font_size_val, 1.0, 36.0, 1.0, 4.0);
        auto font_size_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("measure-fontsize", _("Font Size:"), _font_size_adj, 0, 2));
        font_size_item->set_tooltip_text(_("The font size to be used in the measurement labels"));
        font_size_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _font_size_adj->signal_value_changed().connect(sigc::mem_fun(*this, &MeasureToolbar::fontsize_value_changed));
        add(*font_size_item);
    }

    /* Precision */
    {
        auto precision_val = prefs->getDouble("/tools/measure/precision", 2);
        _precision_adj = Gtk::Adjustment::create(precision_val, 0, 10, 1, 0);
        auto precision_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("measure-precision", _("Precision:"), _precision_adj, 0, 0));
        precision_item->set_tooltip_text(_("Decimal precision of measure"));
        precision_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _precision_adj->signal_value_changed().connect(sigc::mem_fun(*this, &MeasureToolbar::precision_value_changed));
        add(*precision_item);
    }

    /* Scale */
    {
        auto scale_val = prefs->getDouble("/tools/measure/scale", 100.0);
        _scale_adj = Gtk::Adjustment::create(scale_val, 0.0, 90000.0, 1.0, 4.0);
        auto scale_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("measure-scale", _("Scale %:"), _scale_adj, 0, 3));
        scale_item->set_tooltip_text(_("Scale the results"));
        scale_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _scale_adj->signal_value_changed().connect(sigc::mem_fun(*this, &MeasureToolbar::scale_value_changed));
        add(*scale_item);
    }

    /* units label */
    {
        auto unit_label = Gtk::manage(new UI::Widget::LabelToolItem(_("Units:")));
        unit_label->set_tooltip_text(_("The units to be used for the measurements"));
        unit_label->set_use_markup(true);
        add(*unit_label);
    }

    /* units menu */
    {
        auto ti = _tracker->create_tool_item(_("Units:"), _("The units to be used for the measurements") );
        ti->signal_changed().connect(sigc::mem_fun(*this, &MeasureToolbar::unit_changed));
        add(*ti);
    }

    add(*Gtk::manage(new Gtk::SeparatorToolItem()));

    /* measure only selected */
    {
        _only_selected_item = add_toggle_button(_("Measure only selected"),
                                                _("Measure only selected"));
        _only_selected_item->set_icon_name(INKSCAPE_ICON("snap-bounding-box-center"));
        _only_selected_item->set_active(prefs->getBool("/tools/measure/only_selected", false));
        _only_selected_item->signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_only_selected));
    }

    /* ignore_1st_and_last */
    {
        _ignore_1st_and_last_item = add_toggle_button(_("Ignore first and last"),
                                                      _("Ignore first and last"));
        _ignore_1st_and_last_item->set_icon_name(INKSCAPE_ICON("draw-geometry-line-segment"));
        _ignore_1st_and_last_item->set_active(prefs->getBool("/tools/measure/ignore_1st_and_last", true));
        _ignore_1st_and_last_item->signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_ignore_1st_and_last));
    }

    /* measure in betweens */
    {
        _inbetween_item = add_toggle_button(_("Show measures between items"),
                                            _("Show measures between items"));
        _inbetween_item->set_icon_name(INKSCAPE_ICON("distribute-randomize"));
        _inbetween_item->set_active(prefs->getBool("/tools/measure/show_in_between", true));
        _inbetween_item->signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_show_in_between));
    }

    /* only visible */
    {
        _show_hidden_item = add_toggle_button(_("Show hidden intersections"),
                                              _("Show hidden intersections"));
        _show_hidden_item->set_icon_name(INKSCAPE_ICON("object-hidden"));
        _show_hidden_item->set_active(prefs->getBool("/tools/measure/show_hidden", true));
        _show_hidden_item->signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_show_hidden)) ;
    }

    /* measure only current layer */
    {
        _all_layers_item = add_toggle_button(_("Measure all layers"),
                                             _("Measure all layers"));
        _all_layers_item->set_icon_name(INKSCAPE_ICON("dialog-layers"));
        _all_layers_item->set_active(prefs->getBool("/tools/measure/all_layers", true));
        _all_layers_item->signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolbar::toggle_all_layers));
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* toggle start end */
    {
        _reverse_item = Gtk::manage(new Gtk::ToolButton(_("Reverse measure")));
        _reverse_item->set_tooltip_text(_("Reverse measure"));
        _reverse_item->set_icon_name(INKSCAPE_ICON("draw-geometry-mirror"));
        _reverse_item->signal_clicked().connect(sigc::mem_fun(*this, &MeasureToolbar::reverse_knots));
        add(*_reverse_item);
    }

    /* phantom measure */
    {
        _to_phantom_item = Gtk::manage(new Gtk::ToolButton(_("Phantom measure")));
        _to_phantom_item->set_tooltip_text(_("Phantom measure"));
        _to_phantom_item->set_icon_name(INKSCAPE_ICON("selection-make-bitmap-copy"));
        _to_phantom_item->signal_clicked().connect(sigc::mem_fun(*this, &MeasureToolbar::to_phantom));
        add(*_to_phantom_item);
    }

    /* to guides */
    {
        _to_guides_item = Gtk::manage(new Gtk::ToolButton(_("To guides")));
        _to_guides_item->set_tooltip_text(_("To guides"));
        _to_guides_item->set_icon_name(INKSCAPE_ICON("guides"));
        _to_guides_item->signal_clicked().connect(sigc::mem_fun(*this, &MeasureToolbar::to_guides));
        add(*_to_guides_item);
    }

    /* to item */
    {
        _to_item_item = Gtk::manage(new Gtk::ToolButton(_("Convert to item")));
        _to_item_item->set_tooltip_text(_("Convert to item"));
        _to_item_item->set_icon_name(INKSCAPE_ICON("path-reverse"));
        _to_item_item->signal_clicked().connect(sigc::mem_fun(*this, &MeasureToolbar::to_item));
        add(*_to_item_item);
    }

    /* to mark dimensions */
    {
        _mark_dimension_item = Gtk::manage(new Gtk::ToolButton(_("Mark Dimension")));
        _mark_dimension_item->set_tooltip_text(_("Mark Dimension"));
        _mark_dimension_item->set_icon_name(INKSCAPE_ICON("tool-pointer"));
        _mark_dimension_item->signal_clicked().connect(sigc::mem_fun(*this, &MeasureToolbar::to_mark_dimension));
        add(*_mark_dimension_item);
    }

    /* Offset */
    {
        auto offset_val = prefs->getDouble("/tools/measure/offset", 5.0);
        _offset_adj = Gtk::Adjustment::create(offset_val, 0.0, 90000.0, 1.0, 4.0);
        auto offset_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("measure-offset", _("Offset:"), _offset_adj, 0, 2));
        offset_item->set_tooltip_text(_("Mark dimension offset"));
        offset_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
        _offset_adj->signal_value_changed().connect(sigc::mem_fun(*this, &MeasureToolbar::offset_value_changed));
        add(*offset_item);
    }

    show_all();
}

GtkWidget *
MeasureToolbar::create(SPDesktop * desktop)
{
    auto toolbar = new MeasureToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
} // MeasureToolbar::prep()

void
MeasureToolbar::fontsize_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/measure/fontsize"),
            _font_size_adj->get_value());
        MeasureTool *mt = get_measure_tool();
        if (mt) {
            mt->showCanvasItems();
        }
    }
}

void 
MeasureToolbar::unit_changed(int /* notUsed */)
{
    Glib::ustring const unit = _tracker->getActiveUnit()->abbr;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/measure/unit", unit);
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

void
MeasureToolbar::precision_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt(Glib::ustring("/tools/measure/precision"),
            _precision_adj->get_value());
        MeasureTool *mt = get_measure_tool();
        if (mt) {
            mt->showCanvasItems();
        }
    }
}

void
MeasureToolbar::scale_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/measure/scale"),
            _scale_adj->get_value());
        MeasureTool *mt = get_measure_tool();
        if (mt) {
            mt->showCanvasItems();
        }
    }
}

void
MeasureToolbar::offset_value_changed()
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/measure/offset"),
            _offset_adj->get_value());
        MeasureTool *mt = get_measure_tool();
        if (mt) {
            mt->showCanvasItems();
        }
    }
}

void 
MeasureToolbar::toggle_only_selected()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _only_selected_item->get_active();
    prefs->setBool("/tools/measure/only_selected", active);
    if ( active ) {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Measures only selected."));
    } else {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Measure all."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

void 
MeasureToolbar::toggle_ignore_1st_and_last()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _ignore_1st_and_last_item->get_active();
    prefs->setBool("/tools/measure/ignore_1st_and_last", active);
    if ( active ) {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Start and end measures inactive."));
    } else {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Start and end measures active."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

void 
MeasureToolbar::toggle_show_in_between()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _inbetween_item->get_active();
    prefs->setBool("/tools/measure/show_in_between", active);
    if ( active ) {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Compute all elements."));
    } else {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Compute max length."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

void 
MeasureToolbar::toggle_show_hidden()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _show_hidden_item->get_active();
    prefs->setBool("/tools/measure/show_hidden", active);
    if ( active ) {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Show all crossings."));
    } else {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Show visible crossings."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

void
MeasureToolbar::toggle_all_layers()
{
    auto prefs = Inkscape::Preferences::get();
    bool active = _all_layers_item->get_active();
    prefs->setBool("/tools/measure/all_layers", active);
    if ( active ) {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Use all layers in the measure."));
    } else {
        _desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Use current layer in the measure."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

void 
MeasureToolbar::reverse_knots()
{
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->reverseKnots();
    }
}

void 
MeasureToolbar::to_phantom()
{
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->toPhantom();
    }
}

void 
MeasureToolbar::to_guides()
{
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->toGuides();
    }
}

void 
MeasureToolbar::to_item()
{
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->toItem();
    }
}

void 
MeasureToolbar::to_mark_dimension()
{
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->toMarkDimension();
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
