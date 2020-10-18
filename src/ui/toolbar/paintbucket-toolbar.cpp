// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Paint bucket aux toolbar
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

#include "paintbucket-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"

#include "ui/icon-names.h"
#include "ui/tools/flood-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/canvas.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/spin-button-tool-item.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::DocumentUndo;
using Inkscape::Util::unit_table;

namespace Inkscape {
namespace UI {
namespace Toolbar {
PaintbucketToolbar::PaintbucketToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
      _tracker(new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR))
{
    auto prefs = Inkscape::Preferences::get();

    // Channel
    {
        UI::Widget::ComboToolItemColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        for (auto item: Inkscape::UI::Tools::FloodTool::channel_list) {
            Gtk::TreeModel::Row row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_sensitive] = true;
        }

        _channels_item = Gtk::manage(UI::Widget::ComboToolItem::create(_("Fill by"), Glib::ustring(), "Not Used", store));
        _channels_item->use_group_label(true);

        int channels = prefs->getInt("/tools/paintbucket/channels", 0);
        _channels_item->set_active(channels);

        _channels_item->signal_changed().connect(sigc::mem_fun(*this, &PaintbucketToolbar::channels_changed));
        add(*_channels_item);
    }

    // Spacing spinbox
    {
        auto threshold_val = prefs->getDouble("/tools/paintbucket/threshold", 5);
        _threshold_adj = Gtk::Adjustment::create(threshold_val, 0, 100.0, 1.0, 10.0);
        auto threshold_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("inkscape:paintbucket-threshold", _("Threshold:"), _threshold_adj, 1, 0));
        threshold_item->set_tooltip_text(_("The maximum allowed difference between the clicked pixel and the neighboring pixels to be counted in the fill"));
        threshold_item->set_focus_widget(desktop->canvas);
        _threshold_adj->signal_value_changed().connect(sigc::mem_fun(*this, &PaintbucketToolbar::threshold_changed));
        // ege_adjustment_action_set_appearance( eact, TOOLBAR_SLIDER_HINT );
        add(*threshold_item);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    // Create the units menu.
    Glib::ustring stored_unit = prefs->getString("/tools/paintbucket/offsetunits");
    if (!stored_unit.empty()) {
        Unit const *u = unit_table.getUnit(stored_unit);
        _tracker->setActiveUnit(u);
    }

    // Offset spinbox
    {
        auto offset_val = prefs->getDouble("/tools/paintbucket/offset", 0);
        _offset_adj = Gtk::Adjustment::create(offset_val, -1e4, 1e4, 0.1, 0.5);
        auto offset_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("inkscape:paintbucket-offset", _("Grow/shrink by:"), _offset_adj, 1, 2));
        offset_item->set_tooltip_text(_("The amount to grow (positive) or shrink (negative) the created fill path"));
        _tracker->addAdjustment(_offset_adj->gobj());
        offset_item->get_spin_button()->addUnitTracker(_tracker);
        offset_item->set_focus_widget(desktop->canvas);
        _offset_adj->signal_value_changed().connect(sigc::mem_fun(*this, &PaintbucketToolbar::offset_changed));
        add(*offset_item);
    }

    {
        auto unit_menu = _tracker->create_tool_item(_("Units"), (""));
        add(*unit_menu);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Auto Gap */
    {
        UI::Widget::ComboToolItemColumns columns;

        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        for (auto item: Inkscape::UI::Tools::FloodTool::gap_list) {
            Gtk::TreeModel::Row row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_sensitive] = true;
        }

        _autogap_item = Gtk::manage(UI::Widget::ComboToolItem::create(_("Close gaps"), Glib::ustring(), "Not Used", store));
        _autogap_item->use_group_label(true);

        int autogap = prefs->getInt("/tools/paintbucket/autogap", 0);
        _autogap_item->set_active(autogap);

        _autogap_item->signal_changed().connect(sigc::mem_fun(*this, &PaintbucketToolbar::autogap_changed));
        add(*_autogap_item);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Reset */
    {
        auto reset_button = Gtk::manage(new Gtk::ToolButton(_("Defaults")));
        reset_button->set_tooltip_text(_("Reset paint bucket parameters to defaults (use Inkscape Preferences > Tools to change defaults)"));
        reset_button->set_icon_name(INKSCAPE_ICON("edit-clear"));
        reset_button->signal_clicked().connect(sigc::mem_fun(*this, &PaintbucketToolbar::defaults));
        add(*reset_button);
        reset_button->set_sensitive(true);
    }

    show_all();
}

GtkWidget *
PaintbucketToolbar::create(SPDesktop *desktop)
{
    auto toolbar = new PaintbucketToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
PaintbucketToolbar::channels_changed(int channels)
{
    Inkscape::UI::Tools::FloodTool::set_channels(channels);
}

void
PaintbucketToolbar::threshold_changed()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/threshold", (gint)_threshold_adj->get_value());
}

void
PaintbucketToolbar::offset_changed()
{
    Unit const *unit = _tracker->getActiveUnit();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Don't adjust the offset value because we're saving the
    // unit and it'll be correctly handled on load.
    prefs->setDouble("/tools/paintbucket/offset", (gdouble)_offset_adj->get_value());

    g_return_if_fail(unit != nullptr);
    prefs->setString("/tools/paintbucket/offsetunits", unit->abbr);
}

void
PaintbucketToolbar::autogap_changed(int autogap)
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/autogap", autogap);
}

void
PaintbucketToolbar::defaults()
{
    // FIXME: make defaults settable via Inkscape Options
    _threshold_adj->set_value(15);
    _offset_adj->set_value(0.0);

    _channels_item->set_active(Inkscape::UI::Tools::FLOOD_CHANNELS_RGB);
    _autogap_item->set_active(0);
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
