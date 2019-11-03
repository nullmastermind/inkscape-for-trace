// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * LPE aux toolbar
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

#include "lpe-toolbar.h"

#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "live_effects/lpe-line_segment.h"

#include "helper/action-context.h"
#include "helper/action.h"

#include "ui/icon-names.h"
#include "ui/tools-switch.h"
#include "ui/tools/lpe-tool.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::ToolBase;
using Inkscape::UI::Tools::LpeTool;

namespace Inkscape {
namespace UI {
namespace Toolbar {
LPEToolbar::LPEToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
      _tracker(new UnitTracker(Util::UNIT_TYPE_LINEAR)),
      _freeze(false),
      _currentlpe(nullptr),
      _currentlpeitem(nullptr)
{
    _tracker->setActiveUnit(_desktop->getNamedView()->display_units);

    auto unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);

    auto prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/lpetool/unit", unit->abbr);

    /* Automatically create a list of LPEs that get added to the toolbar **/
    {
        Gtk::RadioToolButton::Group mode_group;

        // The first toggle button represents the state that no subtool is active.
        auto inactive_mode_btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, _("All inactive")));
        inactive_mode_btn->set_tooltip_text(_("No geometric tool is active"));
        inactive_mode_btn->set_icon_name(INKSCAPE_ICON("draw-geometry-inactive"));
        _mode_buttons.push_back(inactive_mode_btn);

        Inkscape::LivePathEffect::EffectType type;
        for (int i = 1; i < num_subtools; ++i) { // i == 0 ia INVALIDE_LPE.

            type =  lpesubtools[i].type;

            auto btn = Gtk::manage(new Gtk::RadioToolButton(mode_group, Inkscape::LivePathEffect::LPETypeConverter.get_label(type)));
            btn->set_tooltip_text(_(Inkscape::LivePathEffect::LPETypeConverter.get_label(type).c_str()));
            btn->set_icon_name(lpesubtools[i].icon_name);
            _mode_buttons.push_back(btn);
        }

        int btn_idx = 0;
        for (auto btn : _mode_buttons) {
            btn->set_sensitive(true);
            add(*btn);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &LPEToolbar::mode_changed), btn_idx++));
        }

        int mode = prefs->getInt("/tools/lpetool/mode", 0);
        _mode_buttons[mode]->set_active();
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Show limiting bounding box */
    {
        _show_bbox_item = add_toggle_button(_("Show limiting bounding box"),
                                            _("Show bounding box (used to cut infinite lines)"));
        _show_bbox_item->set_icon_name(INKSCAPE_ICON("show-bounding-box"));
        _show_bbox_item->signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::toggle_show_bbox));
        _show_bbox_item->set_active(prefs->getBool( "/tools/lpetool/show_bbox", true ));
    }

    /* Set limiting bounding box to bbox of current selection */
    {
        // TODO: Shouldn't this just be a button (not toggle button)?
        _bbox_from_selection_item = add_toggle_button(_("Get limiting bounding box from selection"),
                                                      _("Set limiting bounding box (used to cut infinite lines) to the bounding box of current selection"));
        _bbox_from_selection_item->set_icon_name(INKSCAPE_ICON("draw-geometry-set-bounding-box"));
        _bbox_from_selection_item->signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::toggle_set_bbox));
        _bbox_from_selection_item->set_active(false);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Combo box to choose line segment type */
    {
        UI::Widget::ComboToolItemColumns columns;
        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        std::vector<gchar*> line_segment_dropdown_items_list = {
            _("Closed"),
            _("Open start"),
            _("Open end"),
            _("Open both")
        };

        for (auto item: line_segment_dropdown_items_list) {
            Gtk::TreeModel::Row row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_sensitive] = true;
        }

        _line_segment_combo = Gtk::manage(UI::Widget::ComboToolItem::create(_("Line Type"), _("Choose a line segment type"), "Not Used", store));
        _line_segment_combo->use_group_label(false);

        _line_segment_combo->set_active(0);

        _line_segment_combo->signal_changed().connect(sigc::mem_fun(*this, &LPEToolbar::change_line_segment_type));
        add(*_line_segment_combo);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Display measuring info for selected items */
    {
        _measuring_item = add_toggle_button(_("Display measuring info"),
                                            _("Display measuring info for selected items"));
        _measuring_item->set_icon_name(INKSCAPE_ICON("draw-geometry-show-measuring-info"));
        _measuring_item->signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::toggle_show_measuring_info));
        _measuring_item->set_active( prefs->getBool( "/tools/lpetool/show_measuring_info", true ) );
    }

    // Add the units menu
    {
        _units_item = _tracker->create_tool_item(_("Units"), ("") );
        add(*_units_item);
        _units_item->signal_changed_after().connect(sigc::mem_fun(*this, &LPEToolbar::unit_changed));
        _units_item->set_sensitive( prefs->getBool("/tools/lpetool/show_measuring_info", true));
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Open LPE dialog (to adapt parameters numerically) */
    {
        // TODO: Shouldn't this be a regular Gtk::ToolButton (not toggle)?
        _open_lpe_dialog_item = add_toggle_button(_("Open LPE dialog"),
                                                  _("Open LPE dialog (to adapt parameters numerically)"));
        _open_lpe_dialog_item->set_icon_name(INKSCAPE_ICON("dialog-geometry"));
        _open_lpe_dialog_item->signal_toggled().connect(sigc::mem_fun(*this, &LPEToolbar::open_lpe_dialog));
        _open_lpe_dialog_item->set_active(false);
    }

    desktop->connectEventContextChanged(sigc::mem_fun(*this, &LPEToolbar::watch_ec));

    show_all();
}

void
LPEToolbar::set_mode(int mode)
{
    _mode_buttons[mode]->set_active();
}

GtkWidget *
LPEToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new LPEToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

// this is called when the mode is changed via the toolbar (i.e., one of the subtool buttons is pressed)
void
LPEToolbar::mode_changed(int mode)
{
    using namespace Inkscape::LivePathEffect;

    ToolBase *ec = _desktop->event_context;
    if (!SP_IS_LPETOOL_CONTEXT(ec)) {
        return;
    }

    // only take action if run by the attr_changed listener
    if (!_freeze) {
        // in turn, prevent listener from responding
        _freeze = true;

        EffectType type = lpesubtools[mode].type;

        LpeTool *lc = SP_LPETOOL_CONTEXT(_desktop->event_context);
        bool success = lpetool_try_construction(lc, type);
        if (success) {
            // since the construction was already performed, we set the state back to inactive
            _mode_buttons[0]->set_active();
            mode = 0;
        } else {
            // switch to the chosen subtool
            SP_LPETOOL_CONTEXT(_desktop->event_context)->mode = type;
        }

        if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setInt( "/tools/lpetool/mode", mode );
        }

        _freeze = false;
    }
}

void
LPEToolbar::toggle_show_bbox() {
    auto prefs = Inkscape::Preferences::get();

    bool show = _show_bbox_item->get_active();
    prefs->setBool("/tools/lpetool/show_bbox",  show);

    if (tools_isactive(_desktop, TOOLS_LPETOOL)) {
        LpeTool *lc = SP_LPETOOL_CONTEXT(_desktop->event_context);
        lpetool_context_reset_limiting_bbox(lc);
    }
}

void
LPEToolbar::toggle_set_bbox()
{
    auto selection = _desktop->selection;

    auto bbox = selection->visualBounds();

    if (bbox) {
        Geom::Point A(bbox->min());
        Geom::Point B(bbox->max());

        A *= _desktop->doc2dt();
        B *= _desktop->doc2dt();

        // TODO: should we provide a way to store points in prefs?
        auto prefs = Inkscape::Preferences::get();
        prefs->setDouble("/tools/lpetool/bbox_upperleftx", A[Geom::X]);
        prefs->setDouble("/tools/lpetool/bbox_upperlefty", A[Geom::Y]);
        prefs->setDouble("/tools/lpetool/bbox_lowerrightx", B[Geom::X]);
        prefs->setDouble("/tools/lpetool/bbox_lowerrighty", B[Geom::Y]);

        lpetool_context_reset_limiting_bbox(SP_LPETOOL_CONTEXT(_desktop->event_context));
    }

    _bbox_from_selection_item->set_active(false);
}

void
LPEToolbar::change_line_segment_type(int mode)
{
    using namespace Inkscape::LivePathEffect;

    // quit if run by the attr_changed listener
    if (_freeze) {
        return;
    }

    // in turn, prevent listener from responding
    _freeze = true;
    auto line_seg = dynamic_cast<LPELineSegment *>(_currentlpe);

    if (_currentlpeitem && line_seg) {
        line_seg->end_type.param_set_value(static_cast<Inkscape::LivePathEffect::EndType>(mode));
        sp_lpe_item_update_patheffect(_currentlpeitem, true, true);
    }

    _freeze = false;
}

void
LPEToolbar::toggle_show_measuring_info()
{
    if (!tools_isactive(_desktop, TOOLS_LPETOOL)) {
        return;
    }

    bool show = _measuring_item->get_active();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/lpetool/show_measuring_info",  show);

    LpeTool *lc = SP_LPETOOL_CONTEXT(_desktop->event_context);
    lpetool_show_measuring_info(lc, show);

    _units_item->set_sensitive( show );
}

void
LPEToolbar::unit_changed(int /* NotUsed */)
{
    Unit const *unit = _tracker->getActiveUnit();
    g_return_if_fail(unit != nullptr);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/lpetool/unit", unit->abbr);

    if (SP_IS_LPETOOL_CONTEXT(_desktop->event_context)) {
        LpeTool *lc = SP_LPETOOL_CONTEXT(_desktop->event_context);
        lpetool_delete_measuring_items(lc);
        lpetool_create_measuring_items(lc);
    }
}

void
LPEToolbar::open_lpe_dialog()
{
    if (tools_isactive(_desktop, TOOLS_LPETOOL)) {
        sp_action_perform(Inkscape::Verb::get(SP_VERB_DIALOG_LIVE_PATH_EFFECT)->get_action(Inkscape::ActionContext(_desktop)), nullptr);
    }
    _open_lpe_dialog_item->set_active(false);
}

void
LPEToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_LPETOOL_CONTEXT(ec)) {
        // Watch selection
        c_selection_modified = desktop->getSelection()->connectModified(sigc::mem_fun(*this, &LPEToolbar::sel_modified));
        c_selection_changed = desktop->getSelection()->connectChanged(sigc::mem_fun(*this, &LPEToolbar::sel_changed));
        sel_changed(desktop->getSelection());
    } else {
        if (c_selection_modified)
            c_selection_modified.disconnect();
        if (c_selection_changed)
            c_selection_changed.disconnect();
    }
}

void
LPEToolbar::sel_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    ToolBase *ec = selection->desktop()->event_context;
    if (SP_IS_LPETOOL_CONTEXT(ec)) {
        lpetool_update_measuring_items(SP_LPETOOL_CONTEXT(ec));
    }
}

void
LPEToolbar::sel_changed(Inkscape::Selection *selection)
{
    using namespace Inkscape::LivePathEffect;
    ToolBase *ec = selection->desktop()->event_context;
    if (!SP_IS_LPETOOL_CONTEXT(ec)) {
        return;
    }
    LpeTool *lc = SP_LPETOOL_CONTEXT(ec);

    lpetool_delete_measuring_items(lc);
    lpetool_create_measuring_items(lc, selection);

    // activate line segment combo box if a single item with LPELineSegment is selected
    SPItem *item = selection->singleItem();
    if (item && SP_IS_LPE_ITEM(item) && lpetool_item_has_construction(lc, item)) {

        SPLPEItem *lpeitem = SP_LPE_ITEM(item);
        Effect* lpe = lpeitem->getCurrentLPE();
        if (lpe && lpe->effectType() == LINE_SEGMENT) {
            LPELineSegment *lpels = static_cast<LPELineSegment*>(lpe);
            _currentlpe = lpe;
            _currentlpeitem = lpeitem;
            _line_segment_combo->set_sensitive(true);
            _line_segment_combo->set_active( lpels->end_type.get_value() );
        } else {
            _currentlpe = nullptr;
            _currentlpeitem = nullptr;
            _line_segment_combo->set_sensitive(false);
        }

    } else {
        _currentlpe = nullptr;
        _currentlpeitem = nullptr;
        _line_segment_combo->set_sensitive(false);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
