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

#include <gtkmm/comboboxtext.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop.h"
#include "document-undo.h"

#include "ui/icon-names.h"
#include "ui/tools/flood-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/combo-tool-item.h"
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
        add_label(_("Fill by:"));

        _channels_cbt = Gtk::manage(new Gtk::ComboBoxText());
        
        for (auto item: Inkscape::UI::Tools::FloodTool::channel_list) {
            _channels_cbt->append(item);
        }

        int channels = prefs->getInt("/tools/paintbucket/channels", 0);
        _channels_cbt->set_active( channels );
        _channels_cbt->signal_changed().connect(sigc::mem_fun(*this, &PaintbucketToolbar::channels_changed));

        auto channels_item = Gtk::manage(new Gtk::ToolItem());
        channels_item->add(*_channels_cbt);
        add(*channels_item);
    }

    // Spacing spinbox
    {
        auto threshold_val = prefs->getDouble("/tools/paintbucket/threshold", 5);
        _threshold_adj = Gtk::Adjustment::create(threshold_val, 0, 100.0, 1.0, 10.0);
        auto threshold_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("inkscape:paintbucket-threshold", _("Threshold:"), _threshold_adj, 1, 0));
        threshold_item->set_tooltip_text(_("The maximum allowed difference between the clicked pixel and the neighboring pixels to be counted in the fill"));
        threshold_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
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
        offset_item->set_focus_widget(Glib::wrap(GTK_WIDGET(desktop->canvas)));
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
        add_label(_("Close gaps:"));

        _autogap_cbt = Gtk::manage(new Gtk::ComboBoxText());

        for (auto item: Inkscape::UI::Tools::FloodTool::gap_list) {
            _autogap_cbt->append(item);
        }

        int autogap = prefs->getInt("/tools/paintbucket/autogap");
        _autogap_cbt->set_active( autogap );
        auto autogap_item = Gtk::manage(new Gtk::ToolItem());
        autogap_item->add(*_autogap_cbt);
        add(*autogap_item);
        _autogap_cbt->signal_changed().connect(sigc::mem_fun(*this, &PaintbucketToolbar::autogap_changed));
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
PaintbucketToolbar::channels_changed()
{
    auto channels = _channels_cbt->get_active_row_number();
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
PaintbucketToolbar::autogap_changed()
{
    int autogap = _autogap_cbt->get_active_row_number();
    auto prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/paintbucket/autogap", autogap);
}

void
PaintbucketToolbar::defaults()
{
    // FIXME: make defaults settable via Inkscape Options
    _threshold_adj->set_value(15);
    _offset_adj->set_value(0.0);

    _channels_cbt->set_active( Inkscape::UI::Tools::FLOOD_CHANNELS_RGB );
    _autogap_cbt->set_active( 0 );
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
