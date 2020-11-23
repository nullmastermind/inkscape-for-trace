// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   buliabyak@gmail.com
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2005 author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "selected-style.h"

#include <vector>

#include <gtkmm/separatormenuitem.h>


#include "desktop-style.h"
#include "document-undo.h"
#include "gradient-chemistry.h"
#include "message-context.h"
#include "selection.h"

#include "include/gtkmm_version.h"

#include "object/sp-hatch.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-namedview.h"
#include "object/sp-pattern.h"
#include "object/sp-radial-gradient.h"
#include "style.h"

#include "svg/css-ostringstream.h"
#include "svg/svg-color.h"

#include "ui/cursor-utils.h"
#include "ui/dialog/dialog-container.h"
#include "ui/dialog/dialog-base.h"
#include "ui/dialog/fill-and-stroke.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/canvas.h"   // Forced redraws.
#include "ui/widget/color-preview.h"
#include "ui/widget/gradient-image.h"

#include "widgets/ege-paint-def.h"
#include "widgets/spw-utilities.h"

using Inkscape::Util::unit_table;

static gdouble const _sw_presets[]     = { 32 ,  16 ,  10 ,  8 ,  6 ,  4 ,  3 ,  2 ,  1.5 ,  1 ,  0.75 ,  0.5 ,  0.25 ,  0.1 };
static gchar const *const _sw_presets_str[] = {"32", "16", "10", "8", "6", "4", "3", "2", "1.5", "1", "0.75", "0.5", "0.25", "0.1"};

static void
ss_selection_changed (Inkscape::Selection *, gpointer data)
{
    Inkscape::UI::Widget::SelectedStyle *ss = (Inkscape::UI::Widget::SelectedStyle *) data;
    ss->update();
}

static void
ss_selection_modified( Inkscape::Selection *selection, guint flags, gpointer data )
{
    // Don't update the style when dragging or doing non-style related changes
    if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG)) {
        ss_selection_changed (selection, data);
    }
}

static void
ss_subselection_changed( gpointer /*dragger*/, gpointer data )
{
    ss_selection_changed (nullptr, data);
}

namespace {

void clearTooltip( Gtk::Widget &widget )
{
    widget.set_tooltip_text("");
    widget.set_has_tooltip(false);
}

} // namespace

namespace Inkscape {
namespace UI {
namespace Widget {


struct DropTracker {
    SelectedStyle* parent;
    int item;
};

/* Drag and Drop */
enum ui_drop_target_info {
    APP_OSWB_COLOR
};

//TODO: warning: deprecated conversion from string constant to ‘gchar*’
//
//Turn out to be warnings that we should probably leave in place. The
// pointers/types used need to be read-only. So until we correct the using
// code, those warnings are actually desired. They say "Hey! Fix this". We
// definitely don't want to hide/ignore them. --JonCruz
static const GtkTargetEntry ui_drop_target_entries [] = {
    {g_strdup("application/x-oswb-color"), 0, APP_OSWB_COLOR}
};

static guint nui_drop_target_entries = G_N_ELEMENTS(ui_drop_target_entries);

/* convenience function */
static Dialog::FillAndStroke *get_fill_and_stroke_panel(SPDesktop *desktop);

SelectedStyle::SelectedStyle(bool /*layout*/)
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , current_stroke_width(0)
    , _sw_unit(nullptr)
    , _desktop(nullptr)
    , _table()
    , _fill_label(_("Fill:"))
    , _stroke_label(_("Stroke:"))
    , _opacity_label(_("O:"))
    , _fill_place(this, SS_FILL)
    , _stroke_place(this, SS_STROKE)
    , _fill_flag_place()
    , _stroke_flag_place()
    , _opacity_place()
    , _opacity_adjustment(Gtk::Adjustment::create(100, 0.0, 100, 1.0, 10.0))
    , _opacity_sb(0.02, 0)
    , _fill(Gtk::ORIENTATION_HORIZONTAL, 1)
    , _stroke(Gtk::ORIENTATION_HORIZONTAL)
    , _stroke_width_place(this)
    , _stroke_width("")
    , _fill_empty_space("")
    , _opacity_blocked(false)
{
    set_name("SelectedStyle");
    _drop[0] = _drop[1] = nullptr;
    _dropEnabled[0] = _dropEnabled[1] = false;

    _fill_label.set_halign(Gtk::ALIGN_END);
    _fill_label.set_valign(Gtk::ALIGN_CENTER);
    _fill_label.set_margin_top(0);
    _fill_label.set_margin_bottom(0);
    _stroke_label.set_halign(Gtk::ALIGN_END);
    _stroke_label.set_valign(Gtk::ALIGN_CENTER);
    _stroke_label.set_margin_top(0);
    _stroke_label.set_margin_bottom(0);
    _opacity_label.set_halign(Gtk::ALIGN_START);
    _opacity_label.set_valign(Gtk::ALIGN_CENTER);
    _opacity_label.set_margin_top(0);
    _opacity_label.set_margin_bottom(0);
    _stroke_width.set_name("monoStrokeWidth");
    _fill_empty_space.set_name("fillEmptySpace");

    _fill_label.set_margin_start(0);
    _fill_label.set_margin_end(0);
    _stroke_label.set_margin_start(0);
    _stroke_label.set_margin_end(0);
    _opacity_label.set_margin_start(0);
    _opacity_label.set_margin_end(0);

    _table.set_column_spacing(2);
    _table.set_row_spacing(0);

    for (int i = SS_FILL; i <= SS_STROKE; i++) {

        _na[i].set_markup (_("N/A"));
        _na[i].show_all();
        __na[i] = (_("Nothing selected"));

        if (i == SS_FILL) {
            _none[i].set_markup (C_("Fill", "<i>None</i>"));
        } else {
            _none[i].set_markup (C_("Stroke", "<i>None</i>"));
        }
        _none[i].show_all();
        __none[i] = (i == SS_FILL)? (C_("Fill and stroke", "No fill, middle-click for black fill")) : (C_("Fill and stroke", "No stroke, middle-click for black stroke"));

        _pattern[i].set_markup (_("Pattern"));
        _pattern[i].show_all();
        __pattern[i] = (i == SS_FILL)? (_("Pattern (fill)")) : (_("Pattern (stroke)"));

        _hatch[i].set_markup(_("Hatch"));
        _hatch[i].show_all();
        __hatch[i] = (i == SS_FILL) ? (_("Hatch (fill)")) : (_("Hatch (stroke)"));

        _lgradient[i].set_markup (_("<b>L</b>"));
        _lgradient[i].show_all();
        __lgradient[i] = (i == SS_FILL)? (_("Linear gradient (fill)")) : (_("Linear gradient (stroke)"));

        _gradient_preview_l[i] = Gtk::manage(new GradientImage(nullptr));
        _gradient_box_l[i].set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        _gradient_box_l[i].pack_start(_lgradient[i]);
        _gradient_box_l[i].pack_start(*_gradient_preview_l[i]);
        _gradient_box_l[i].show_all();

        _rgradient[i].set_markup (_("<b>R</b>"));
        _rgradient[i].show_all();
        __rgradient[i] = (i == SS_FILL)? (_("Radial gradient (fill)")) : (_("Radial gradient (stroke)"));

        _gradient_preview_r[i] = Gtk::manage(new GradientImage(nullptr));
        _gradient_box_r[i].set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        _gradient_box_r[i].pack_start(_rgradient[i]);
        _gradient_box_r[i].pack_start(*_gradient_preview_r[i]);
        _gradient_box_r[i].show_all();

#ifdef WITH_MESH
        _mgradient[i].set_markup (_("<b>M</b>"));
        _mgradient[i].show_all();
        __mgradient[i] = (i == SS_FILL)? (_("Mesh gradient (fill)")) : (_("Mesh gradient (stroke)"));

        _gradient_preview_m[i] = Gtk::manage(new GradientImage(nullptr));
        _gradient_box_m[i].set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        _gradient_box_m[i].pack_start(_mgradient[i]);
        _gradient_box_m[i].pack_start(*_gradient_preview_m[i]);
        _gradient_box_m[i].show_all();
#endif

        _many[i].set_markup (_("≠"));
        _many[i].show_all();
        __many[i] = (i == SS_FILL)? (_("Different fills")) : (_("Different strokes"));

        _unset[i].set_markup (_("<b>Unset</b>"));
        _unset[i].show_all();
        __unset[i] = (i == SS_FILL)? (_("Unset fill")) : (_("Unset stroke"));

        _color_preview[i] = new Inkscape::UI::Widget::ColorPreview (0);
        __color[i] = (i == SS_FILL)? (_("Flat color (fill)")) : (_("Flat color (stroke)"));

        // TRANSLATORS: A means "Averaged"
        _averaged[i].set_markup (_("<b>a</b>"));
        _averaged[i].show_all();
        __averaged[i] = (i == SS_FILL)? (_("Fill is averaged over selected objects")) : (_("Stroke is averaged over selected objects"));

        // TRANSLATORS: M means "Multiple"
        _multiple[i].set_markup (_("<b>m</b>"));
        _multiple[i].show_all();
        __multiple[i] = (i == SS_FILL)? (_("Multiple selected objects have the same fill")) : (_("Multiple selected objects have the same stroke"));

        _popup_edit[i].add(*(new Gtk::Label((i == SS_FILL)? _("Edit fill...") : _("Edit stroke..."), Gtk::ALIGN_START)));
        _popup_edit[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_edit : &SelectedStyle::on_stroke_edit ));

        _popup_lastused[i].add(*(new Gtk::Label(_("Last set color"), Gtk::ALIGN_START)));
        _popup_lastused[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_lastused : &SelectedStyle::on_stroke_lastused ));

        _popup_lastselected[i].add(*(new Gtk::Label(_("Last selected color"), Gtk::ALIGN_START)));
        _popup_lastselected[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_lastselected : &SelectedStyle::on_stroke_lastselected ));

        _popup_invert[i].add(*(new Gtk::Label(_("Invert"), Gtk::ALIGN_START)));
        _popup_invert[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_invert : &SelectedStyle::on_stroke_invert ));

        _popup_white[i].add(*(new Gtk::Label(_("White"), Gtk::ALIGN_START)));
        _popup_white[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_white : &SelectedStyle::on_stroke_white ));

        _popup_black[i].add(*(new Gtk::Label(_("Black"), Gtk::ALIGN_START)));
        _popup_black[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_black : &SelectedStyle::on_stroke_black ));

        _popup_copy[i].add(*(new Gtk::Label(_("Copy color"), Gtk::ALIGN_START)));
        _popup_copy[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_copy : &SelectedStyle::on_stroke_copy ));

        _popup_paste[i].add(*(new Gtk::Label(_("Paste color"), Gtk::ALIGN_START)));
        _popup_paste[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_paste : &SelectedStyle::on_stroke_paste ));

        _popup_swap[i].add(*(new Gtk::Label(_("Swap fill and stroke"), Gtk::ALIGN_START)));
        _popup_swap[i].signal_activate().connect(sigc::mem_fun(*this,
                               &SelectedStyle::on_fillstroke_swap));

        _popup_opaque[i].add(*(new Gtk::Label((i == SS_FILL)? _("Make fill opaque") : _("Make stroke opaque"), Gtk::ALIGN_START)));
        _popup_opaque[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_opaque : &SelectedStyle::on_stroke_opaque ));

        //TRANSLATORS COMMENT: unset is a verb here
        _popup_unset[i].add(*(new Gtk::Label((i == SS_FILL)? _("Unset fill") : _("Unset stroke"), Gtk::ALIGN_START)));
        _popup_unset[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_unset : &SelectedStyle::on_stroke_unset ));

        _popup_remove[i].add(*(new Gtk::Label((i == SS_FILL)? _("Remove fill") : _("Remove stroke"), Gtk::ALIGN_START)));
        _popup_remove[i].signal_activate().connect(sigc::mem_fun(*this,
                               (i == SS_FILL)? &SelectedStyle::on_fill_remove : &SelectedStyle::on_stroke_remove ));

        _popup[i].attach(_popup_edit[i], 0,1, 0,1);
          _popup[i].attach(*(new Gtk::SeparatorMenuItem()), 0,1, 1,2);
        _popup[i].attach(_popup_lastused[i], 0,1, 2,3);
        _popup[i].attach(_popup_lastselected[i], 0,1, 3,4);
          _popup[i].attach(*(new Gtk::SeparatorMenuItem()), 0,1, 4,5);
        _popup[i].attach(_popup_invert[i], 0,1, 5,6);
          _popup[i].attach(*(new Gtk::SeparatorMenuItem()), 0,1, 6,7);
        _popup[i].attach(_popup_white[i], 0,1, 7,8);
        _popup[i].attach(_popup_black[i], 0,1, 8,9);
          _popup[i].attach(*(new Gtk::SeparatorMenuItem()), 0,1, 9,10);
        _popup[i].attach(_popup_copy[i], 0,1, 10,11);
        _popup_copy[i].set_sensitive(false);
        _popup[i].attach(_popup_paste[i], 0,1, 11,12);
        _popup[i].attach(_popup_swap[i], 0,1, 12,13);
          _popup[i].attach(*(new Gtk::SeparatorMenuItem()), 0,1, 13,14);
        _popup[i].attach(_popup_opaque[i], 0,1, 14,15);
        _popup[i].attach(_popup_unset[i], 0,1, 15,16);
        _popup[i].attach(_popup_remove[i], 0,1, 16,17);
        _popup[i].show_all();

        _mode[i] = SS_NA;
    }

    {
        int row = 0;

        Inkscape::Util::UnitTable::UnitMap m = unit_table.units(Inkscape::Util::UNIT_TYPE_LINEAR);
        Inkscape::Util::UnitTable::UnitMap::iterator iter = m.begin();
        while(iter != m.end()) {
            Gtk::RadioMenuItem *mi = Gtk::manage(new Gtk::RadioMenuItem(_sw_group));
            mi->add(*(new Gtk::Label(iter->first, Gtk::ALIGN_START)));
            _unit_mis.push_back(mi);
            Inkscape::Util::Unit const *u = unit_table.getUnit(iter->first);
            mi->signal_activate().connect(sigc::bind<Inkscape::Util::Unit const *>(sigc::mem_fun(*this, &SelectedStyle::on_popup_units), u));
            _popup_sw.attach(*mi, 0,1, row, row+1);
            row++;
            ++iter;
        }

        _popup_sw.attach(*(new Gtk::SeparatorMenuItem()), 0,1, row, row+1);
        row++;

        for (guint i = 0; i < G_N_ELEMENTS(_sw_presets_str); ++i) {
            Gtk::MenuItem *mi = Gtk::manage(new Gtk::MenuItem());
            mi->add(*(new Gtk::Label(_sw_presets_str[i], Gtk::ALIGN_START)));
            mi->signal_activate().connect(sigc::bind<int>(sigc::mem_fun(*this, &SelectedStyle::on_popup_preset), i));
            _popup_sw.attach(*mi, 0,1, row, row+1);
            row++;
        }

        _popup_sw.attach(*(new Gtk::SeparatorMenuItem()), 0,1, row, row+1);
        row++;

        _popup_sw_remove.add(*(new Gtk::Label(_("Remove"), Gtk::ALIGN_START)));
        _popup_sw_remove.signal_activate().connect(sigc::mem_fun(*this, &SelectedStyle::on_stroke_remove));
        _popup_sw.attach(_popup_sw_remove, 0,1, row, row+1);
        row++;

        _popup_sw.show_all();
    }
    // fill row
    _fill_flag_place.set_size_request(SELECTED_STYLE_FLAG_WIDTH , -1);

    _fill_place.add(_na[SS_FILL]);
    _fill_place.set_tooltip_text(__na[SS_FILL]);
    _fill.set_size_request(SELECTED_STYLE_PLACE_WIDTH, -1);
    _fill.pack_start(_fill_place, Gtk::PACK_EXPAND_WIDGET);

    _fill_empty_space.set_size_request(SELECTED_STYLE_STROKE_WIDTH);

    // stroke row
    _stroke_flag_place.set_size_request(SELECTED_STYLE_FLAG_WIDTH, -1);

    _stroke_place.add(_na[SS_STROKE]);
    _stroke_place.set_tooltip_text(__na[SS_STROKE]);
    _stroke.set_size_request(SELECTED_STYLE_PLACE_WIDTH, -1);
    _stroke.pack_start(_stroke_place, Gtk::PACK_EXPAND_WIDGET);

    _stroke_width_place.add(_stroke_width);
    _stroke_width_place.set_size_request(SELECTED_STYLE_STROKE_WIDTH);

    // opacity selector
    _opacity_place.add(_opacity_label);

    _opacity_sb.set_adjustment(_opacity_adjustment);
    _opacity_sb.set_size_request (SELECTED_STYLE_SB_WIDTH, -1);
    _opacity_sb.set_sensitive (false);

    // arrange in table
    _table.attach(_fill_label, 0, 0, 1, 1);
    _table.attach(_stroke_label, 0, 1, 1, 1);

    _table.attach(_fill_flag_place, 1, 0, 1, 1);
    _table.attach(_stroke_flag_place, 1, 1, 1, 1);

    _table.attach(_fill, 2, 0, 1, 1);
    _table.attach(_stroke, 2, 1, 1, 1);

    _table.attach(_fill_empty_space, 3, 0, 1, 1);
    _table.attach(_stroke_width_place, 3, 1, 1, 1);

    _table.attach(_opacity_place, 4, 0, 1, 2);
    _table.attach(_opacity_sb, 5, 0, 1, 2);

    pack_start(_table, true, true, 2);

    set_size_request (SELECTED_STYLE_WIDTH, -1);

    _drop[SS_FILL] = new DropTracker();
    ((DropTracker*)_drop[SS_FILL])->parent = this;
    ((DropTracker*)_drop[SS_FILL])->item = SS_FILL;

    _drop[SS_STROKE] = new DropTracker();
    ((DropTracker*)_drop[SS_STROKE])->parent = this;
    ((DropTracker*)_drop[SS_STROKE])->item = SS_STROKE;

    g_signal_connect(_stroke_place.gobj(),
                     "drag_data_received",
                     G_CALLBACK(dragDataReceived),
                     _drop[SS_STROKE]);

    g_signal_connect(_fill_place.gobj(),
                     "drag_data_received",
                     G_CALLBACK(dragDataReceived),
                     _drop[SS_FILL]);

    _fill_place.signal_button_release_event().connect(sigc::mem_fun(*this, &SelectedStyle::on_fill_click));
    _stroke_place.signal_button_release_event().connect(sigc::mem_fun(*this, &SelectedStyle::on_stroke_click));
    _opacity_place.signal_button_press_event().connect(sigc::mem_fun(*this, &SelectedStyle::on_opacity_click));
    _stroke_width_place.signal_button_press_event().connect(sigc::mem_fun(*this, &SelectedStyle::on_sw_click));
    _stroke_width_place.signal_button_release_event().connect(sigc::mem_fun(*this, &SelectedStyle::on_sw_click));
    _opacity_sb.signal_populate_popup().connect(sigc::mem_fun(*this, &SelectedStyle::on_opacity_menu));
    _opacity_sb.signal_value_changed().connect(sigc::mem_fun(*this, &SelectedStyle::on_opacity_changed));
}

SelectedStyle::~SelectedStyle()
{
    selection_changed_connection->disconnect();
    delete selection_changed_connection;
    selection_modified_connection->disconnect();
    delete selection_modified_connection;
    subselection_changed_connection->disconnect();
    delete subselection_changed_connection;
    _unit_mis.clear();

    _fill_place.remove();
    _stroke_place.remove();

    for (int i = SS_FILL; i <= SS_STROKE; i++) {
        delete _color_preview[i];
    }

    delete (DropTracker*)_drop[SS_FILL];
    delete (DropTracker*)_drop[SS_STROKE];
}

void
SelectedStyle::setDesktop(SPDesktop *desktop)
{
    _desktop = desktop;

    Inkscape::Selection *selection = desktop->getSelection();

    selection_changed_connection = new sigc::connection (selection->connectChanged(
        sigc::bind (
            sigc::ptr_fun(&ss_selection_changed),
            this )
    ));
    selection_modified_connection = new sigc::connection (selection->connectModified(
        sigc::bind (
            sigc::ptr_fun(&ss_selection_modified),
            this )
    ));
    subselection_changed_connection = new sigc::connection (desktop->connectToolSubselectionChanged(
        sigc::bind (
            sigc::ptr_fun(&ss_subselection_changed),
            this )
    ));

    _sw_unit = desktop->getNamedView()->display_units;

    // Set the doc default unit active in the units list
    for ( auto mi:_unit_mis ) {
        if (mi && mi->get_label() == _sw_unit->abbr) {
            mi->set_active();
            break;
        }
    }
}

void SelectedStyle::dragDataReceived( GtkWidget */*widget*/,
                                      GdkDragContext */*drag_context*/,
                                      gint /*x*/, gint /*y*/,
                                      GtkSelectionData *data,
                                      guint /*info*/,
                                      guint /*event_time*/,
                                      gpointer user_data )
{
    DropTracker* tracker = (DropTracker*)user_data;

    // copied from drag-and-drop.cpp, case APP_OSWB_COLOR
    bool worked = false;
    Glib::ustring colorspec;
    if (gtk_selection_data_get_format(data) == 8) {
        ege::PaintDef color;
        worked = color.fromMIMEData("application/x-oswb-color",
                                    reinterpret_cast<char const *>(gtk_selection_data_get_data(data)),
                                    gtk_selection_data_get_length(data),
                                    gtk_selection_data_get_format(data));
        if (worked) {
            if (color.getType() == ege::PaintDef::CLEAR) {
                colorspec = ""; // TODO check if this is sufficient
            } else if (color.getType() == ege::PaintDef::NONE) {
                colorspec = "none";
            } else {
                unsigned int r = color.getR();
                unsigned int g = color.getG();
                unsigned int b = color.getB();

                gchar* tmp = g_strdup_printf("#%02x%02x%02x", r, g, b);
                colorspec = tmp;
                g_free(tmp);
            }
        }
    }
    if (worked) {
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, (tracker->item == SS_FILL) ? "fill":"stroke", colorspec.c_str());

        sp_desktop_set_style(tracker->parent->_desktop, css);
        sp_repr_css_attr_unref(css);
        DocumentUndo::done(tracker->parent->_desktop->getDocument(), SP_VERB_NONE, _("Drop color"));
    }
}

void SelectedStyle::on_fill_remove() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    sp_repr_css_set_property (css, "fill", "none");
    sp_desktop_set_style (_desktop, css, true, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                        _("Remove fill"));
}

void SelectedStyle::on_stroke_remove() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    sp_repr_css_set_property (css, "stroke", "none");
    sp_desktop_set_style (_desktop, css, true, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Remove stroke"));
}

void SelectedStyle::on_fill_unset() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    sp_repr_css_unset_property (css, "fill");
    sp_desktop_set_style (_desktop, css, true, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Unset fill"));
}

void SelectedStyle::on_stroke_unset() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    sp_repr_css_unset_property (css, "stroke");
    sp_repr_css_unset_property (css, "stroke-opacity");
    sp_repr_css_unset_property (css, "stroke-width");
    sp_repr_css_unset_property (css, "stroke-miterlimit");
    sp_repr_css_unset_property (css, "stroke-linejoin");
    sp_repr_css_unset_property (css, "stroke-linecap");
    sp_repr_css_unset_property (css, "stroke-dashoffset");
    sp_repr_css_unset_property (css, "stroke-dasharray");
    sp_desktop_set_style (_desktop, css, true, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Unset stroke"));
}

void SelectedStyle::on_fill_opaque() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    sp_repr_css_set_property (css, "fill-opacity", "1");
    sp_desktop_set_style (_desktop, css, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Make fill opaque"));
}

void SelectedStyle::on_stroke_opaque() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    sp_repr_css_set_property (css, "stroke-opacity", "1");
    sp_desktop_set_style (_desktop, css, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Make fill opaque"));
}

void SelectedStyle::on_fill_lastused() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    guint32 color = sp_desktop_get_color(_desktop, true);
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), color);
    sp_repr_css_set_property (css, "fill", c);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Apply last set color to fill"));
}

void SelectedStyle::on_stroke_lastused() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    guint32 color = sp_desktop_get_color(_desktop, false);
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), color);
    sp_repr_css_set_property (css, "stroke", c);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Apply last set color to stroke"));
}

void SelectedStyle::on_fill_lastselected() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), _lastselected[SS_FILL]);
    sp_repr_css_set_property (css, "fill", c);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Apply last selected color to fill"));
}

void SelectedStyle::on_stroke_lastselected() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), _lastselected[SS_STROKE]);
    sp_repr_css_set_property (css, "stroke", c);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Apply last selected color to stroke"));
}

void SelectedStyle::on_fill_invert() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    guint32 color = _thisselected[SS_FILL];
    gchar c[64];
    if (_mode[SS_FILL] == SS_LGRADIENT || _mode[SS_FILL] == SS_RGRADIENT) {
        sp_gradient_invert_selected_gradients(_desktop, Inkscape::FOR_FILL);
        return;

    }

    if (_mode[SS_FILL] != SS_COLOR) return;
    sp_svg_write_color (c, sizeof(c),
        SP_RGBA32_U_COMPOSE(
                (255 - SP_RGBA32_R_U(color)),
                (255 - SP_RGBA32_G_U(color)),
                (255 - SP_RGBA32_B_U(color)),
                SP_RGBA32_A_U(color)
        )
    );
    sp_repr_css_set_property (css, "fill", c);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Invert fill"));
}

void SelectedStyle::on_stroke_invert() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    guint32 color = _thisselected[SS_STROKE];
    gchar c[64];
    if (_mode[SS_STROKE] == SS_LGRADIENT || _mode[SS_STROKE] == SS_RGRADIENT) {
        sp_gradient_invert_selected_gradients(_desktop, Inkscape::FOR_STROKE);
        return;
    }
    if (_mode[SS_STROKE] != SS_COLOR) return;
    sp_svg_write_color (c, sizeof(c),
        SP_RGBA32_U_COMPOSE(
                (255 - SP_RGBA32_R_U(color)),
                (255 - SP_RGBA32_G_U(color)),
                (255 - SP_RGBA32_B_U(color)),
                SP_RGBA32_A_U(color)
        )
    );
    sp_repr_css_set_property (css, "stroke", c);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Invert stroke"));
}

void SelectedStyle::on_fill_white() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), 0xffffffff);
    sp_repr_css_set_property (css, "fill", c);
    sp_repr_css_set_property (css, "fill-opacity", "1");
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("White fill"));
}

void SelectedStyle::on_stroke_white() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), 0xffffffff);
    sp_repr_css_set_property (css, "stroke", c);
    sp_repr_css_set_property (css, "stroke-opacity", "1");
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("White stroke"));
}

void SelectedStyle::on_fill_black() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), 0x000000ff);
    sp_repr_css_set_property (css, "fill", c);
    sp_repr_css_set_property (css, "fill-opacity", "1.0");
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Black fill"));
}

void SelectedStyle::on_stroke_black() {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gchar c[64];
    sp_svg_write_color (c, sizeof(c), 0x000000ff);
    sp_repr_css_set_property (css, "stroke", c);
    sp_repr_css_set_property (css, "stroke-opacity", "1.0");
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                       _("Black stroke"));
}

void SelectedStyle::on_fill_copy() {
    if (_mode[SS_FILL] == SS_COLOR) {
        gchar c[64];
        sp_svg_write_color (c, sizeof(c), _thisselected[SS_FILL]);
        Glib::ustring text;
        text += c;
        if (!text.empty()) {
            Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();
            refClipboard->set_text(text);
        }
    }
}

void SelectedStyle::on_stroke_copy() {
    if (_mode[SS_STROKE] == SS_COLOR) {
        gchar c[64];
        sp_svg_write_color (c, sizeof(c), _thisselected[SS_STROKE]);
        Glib::ustring text;
        text += c;
        if (!text.empty()) {
            Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();
            refClipboard->set_text(text);
        }
    }
}

void SelectedStyle::on_fill_paste() {
    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();
    Glib::ustring const text = refClipboard->wait_for_text();

    if (!text.empty()) {
        guint32 color = sp_svg_read_color(text.c_str(), 0x000000ff); // impossible value, as SVG color cannot have opacity
        if (color == 0x000000ff) // failed to parse color string
            return;

        SPCSSAttr *css = sp_repr_css_attr_new ();
        sp_repr_css_set_property (css, "fill", text.c_str());
        sp_desktop_set_style (_desktop, css);
        sp_repr_css_attr_unref (css);
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                           _("Paste fill"));
    }
}

void SelectedStyle::on_stroke_paste() {
    Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get();
    Glib::ustring const text = refClipboard->wait_for_text();

    if (!text.empty()) {
        guint32 color = sp_svg_read_color(text.c_str(), 0x000000ff); // impossible value, as SVG color cannot have opacity
        if (color == 0x000000ff) // failed to parse color string
            return;

        SPCSSAttr *css = sp_repr_css_attr_new ();
        sp_repr_css_set_property (css, "stroke", text.c_str());
        sp_desktop_set_style (_desktop, css);
        sp_repr_css_attr_unref (css);
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                           _("Paste stroke"));
    }
}

void SelectedStyle::on_fillstroke_swap() {
    _desktop->getSelection()->swapFillStroke();
}

void SelectedStyle::on_fill_edit() {
    if (Dialog::FillAndStroke *fs = get_fill_and_stroke_panel(_desktop))
        fs->showPageFill();
}

void SelectedStyle::on_stroke_edit() {
    if (Dialog::FillAndStroke *fs = get_fill_and_stroke_panel(_desktop))
        fs->showPageStrokePaint();
}

bool
SelectedStyle::on_fill_click(GdkEventButton *event)
{
    if (event->button == 1) { // click, open fill&stroke

        if (Dialog::FillAndStroke *fs = get_fill_and_stroke_panel(_desktop))
            fs->showPageFill();

    } else if (event->button == 3) { // right-click, popup menu
        _popup[SS_FILL].popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    } else if (event->button == 2) { // middle click, toggle none/lastcolor
        if (_mode[SS_FILL] == SS_NONE) {
            on_fill_lastused();
        } else {
            on_fill_remove();
        }
    }
    return true;
}

bool
SelectedStyle::on_stroke_click(GdkEventButton *event)
{
    if (event->button == 1) { // click, open fill&stroke
        if (Dialog::FillAndStroke *fs = get_fill_and_stroke_panel(_desktop))
            fs->showPageStrokePaint();
    } else if (event->button == 3) { // right-click, popup menu
        _popup[SS_STROKE].popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    } else if (event->button == 2) { // middle click, toggle none/lastcolor
        if (_mode[SS_STROKE] == SS_NONE) {
            on_stroke_lastused();
        } else {
            on_stroke_remove();
        }
    }
    return true;
}

bool
SelectedStyle::on_sw_click(GdkEventButton *event)
{
    if (event->button == 1) { // click, open fill&stroke
        if (Dialog::FillAndStroke *fs = get_fill_and_stroke_panel(_desktop))
            fs->showPageStrokeStyle();
    } else if (event->button == 3) { // right-click, popup menu
        _popup_sw.popup_at_pointer(reinterpret_cast<GdkEvent *>(event));
    } else if (event->button == 2) { // middle click, toggle none/lastwidth?
        //
    }
    return true;
}

bool
SelectedStyle::on_opacity_click(GdkEventButton *event)
{
    if (event->button == 2) { // middle click
        const char* opacity = _opacity_sb.get_value() < 50? "0.5" : (_opacity_sb.get_value() == 100? "0" : "1");
        SPCSSAttr *css = sp_repr_css_attr_new ();
        sp_repr_css_set_property (css, "opacity", opacity);
        sp_desktop_set_style (_desktop, css);
        sp_repr_css_attr_unref (css);
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_FILL_STROKE,
                           _("Change opacity"));
        return true;
    }

    return false;
}

void SelectedStyle::on_popup_units(Inkscape::Util::Unit const *unit) {
    _sw_unit = unit;
    update();
}

void SelectedStyle::on_popup_preset(int i) {
    SPCSSAttr *css = sp_repr_css_attr_new ();
    gdouble w;
    if (_sw_unit) {
        w = Inkscape::Util::Quantity::convert(_sw_presets[i], _sw_unit, "px");
    } else {
        w = _sw_presets[i];
    }
    Inkscape::CSSOStringStream os;
    os << w;
    sp_repr_css_set_property (css, "stroke-width", os.str().c_str());
    // FIXME: update dash patterns!
    sp_desktop_set_style (_desktop, css, true);
    sp_repr_css_attr_unref (css);
    DocumentUndo::done(_desktop->getDocument(), SP_VERB_DIALOG_SWATCHES,
                       _("Change stroke width"));
}

void
SelectedStyle::update()
{
    if (_desktop == nullptr)
        return;

    // create temporary style
    SPStyle query(_desktop->getDocument());

    for (int i = SS_FILL; i <= SS_STROKE; i++) {
        Gtk::EventBox *place = (i == SS_FILL)? &_fill_place : &_stroke_place;
        Gtk::EventBox *flag_place = (i == SS_FILL)? &_fill_flag_place : &_stroke_flag_place;

        place->remove();
        flag_place->remove();

        clearTooltip(*place);
        clearTooltip(*flag_place);

        _mode[i] = SS_NA;
        _paintserver_id[i].clear();

        _popup_copy[i].set_sensitive(false);

        // query style from desktop. This returns a result flag and fills query with the style of subselection, if any, or selection
        int result = sp_desktop_query_style (_desktop, &query,
                                             (i == SS_FILL)? QUERY_STYLE_PROPERTY_FILL : QUERY_STYLE_PROPERTY_STROKE);
        switch (result) {
        case QUERY_STYLE_NOTHING:
            place->add(_na[i]);
            place->set_tooltip_text(__na[i]);
            _mode[i] = SS_NA;
            if ( _dropEnabled[i] ) {
                gtk_drag_dest_unset( GTK_WIDGET((i==SS_FILL) ? _fill_place.gobj():_stroke_place.gobj()) );
                _dropEnabled[i] = false;
            }
            break;
        case QUERY_STYLE_SINGLE:
        case QUERY_STYLE_MULTIPLE_AVERAGED:
        case QUERY_STYLE_MULTIPLE_SAME:
            if ( !_dropEnabled[i] ) {
                gtk_drag_dest_set( GTK_WIDGET( (i==SS_FILL) ? _fill_place.gobj():_stroke_place.gobj()),
                                   GTK_DEST_DEFAULT_ALL,
                                   ui_drop_target_entries,
                                   nui_drop_target_entries,
                                   GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE) );
                _dropEnabled[i] = true;
            }
            SPIPaint *paint;
            if (i == SS_FILL) {
                paint = &(query.fill);
            } else {
                paint = &(query.stroke);
            }
            if (paint->set && paint->isPaintserver()) {
                SPPaintServer *server = (i == SS_FILL)? SP_STYLE_FILL_SERVER (&query) : SP_STYLE_STROKE_SERVER (&query);
                if ( server ) {
                    Inkscape::XML::Node *srepr = server->getRepr();
                    _paintserver_id[i] += "url(#";
                    _paintserver_id[i] += srepr->attribute("id");
                    _paintserver_id[i] += ")";

                    if (SP_IS_LINEARGRADIENT(server)) {
                        SPGradient *vector = SP_GRADIENT(server)->getVector();
                        _gradient_preview_l[i]->set_gradient(vector);
                        place->add(_gradient_box_l[i]);
                        place->set_tooltip_text(__lgradient[i]);
                        _mode[i] = SS_LGRADIENT;
                    } else if (SP_IS_RADIALGRADIENT(server)) {
                        SPGradient *vector = SP_GRADIENT(server)->getVector();
                        _gradient_preview_r[i]->set_gradient(vector);
                        place->add(_gradient_box_r[i]);
                        place->set_tooltip_text(__rgradient[i]);
                        _mode[i] = SS_RGRADIENT;
#ifdef WITH_MESH
                    } else if (SP_IS_MESHGRADIENT(server)) {
                        SPGradient *array = SP_GRADIENT(server)->getArray();
                        _gradient_preview_m[i]->set_gradient(array);
                        place->add(_gradient_box_m[i]);
                        place->set_tooltip_text(__mgradient[i]);
                        _mode[i] = SS_MGRADIENT;
#endif
                    } else if (SP_IS_PATTERN(server)) {
                        place->add(_pattern[i]);
                        place->set_tooltip_text(__pattern[i]);
                        _mode[i] = SS_PATTERN;
                    } else if (SP_IS_HATCH(server)) {
                        place->add(_hatch[i]);
                        place->set_tooltip_text(__hatch[i]);
                        _mode[i] = SS_HATCH;
                    }
                } else {
                    g_warning ("file %s: line %d: Unknown paint server", __FILE__, __LINE__);
                }
            } else if (paint->set && paint->isColor()) {
                guint32 color = paint->value.color.toRGBA32(
                                     SP_SCALE24_TO_FLOAT ((i == SS_FILL)? query.fill_opacity.value : query.stroke_opacity.value));
                _lastselected[i] = _thisselected[i];
                _thisselected[i] = color; // include opacity
                ((Inkscape::UI::Widget::ColorPreview*)_color_preview[i])->setRgba32 (color);
                _color_preview[i]->show_all();
                place->add(*_color_preview[i]);
                gchar c_string[64];
                g_snprintf (c_string, 64, "%06x/%.3g", color >> 8, SP_RGBA32_A_F(color));
                place->set_tooltip_text(__color[i] + ": " + c_string + _(", drag to adjust, middle-click to remove"));
                _mode[i] = SS_COLOR;
                _popup_copy[i].set_sensitive(true);

            } else if (paint->set && paint->isNone()) {
                place->add(_none[i]);
                place->set_tooltip_text(__none[i]);
                _mode[i] = SS_NONE;
            } else if (!paint->set) {
                place->add(_unset[i]);
                place->set_tooltip_text(__unset[i]);
                _mode[i] = SS_UNSET;
            }
            if (result == QUERY_STYLE_MULTIPLE_AVERAGED) {
                flag_place->add(_averaged[i]);
                flag_place->set_tooltip_text(__averaged[i]);
            } else if (result == QUERY_STYLE_MULTIPLE_SAME) {
                flag_place->add(_multiple[i]);
                flag_place->set_tooltip_text(__multiple[i]);
            }
            break;
        case QUERY_STYLE_MULTIPLE_DIFFERENT:
            place->add(_many[i]);
            place->set_tooltip_text(__many[i]);
            _mode[i] = SS_MANY;
            break;
        default:
            break;
        }
    }

// Now query opacity
    clearTooltip(_opacity_place);
    clearTooltip(_opacity_sb);

    int result = sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_MASTEROPACITY);

    switch (result) {
    case QUERY_STYLE_NOTHING:
        _opacity_place.set_tooltip_text(_("Nothing selected"));
        _opacity_sb.set_tooltip_text(_("Nothing selected"));
        _opacity_sb.set_sensitive(false);
        break;
    case QUERY_STYLE_SINGLE:
    case QUERY_STYLE_MULTIPLE_AVERAGED:
    case QUERY_STYLE_MULTIPLE_SAME:
        _opacity_place.set_tooltip_text(_("Opacity (%)"));
        _opacity_sb.set_tooltip_text(_("Opacity (%)"));
        if (_opacity_blocked) break;
        _opacity_blocked = true;
        _opacity_sb.set_sensitive(true);
        _opacity_adjustment->set_value(SP_SCALE24_TO_FLOAT(query.opacity.value) * 100);
        _opacity_blocked = false;
        break;
    }

// Now query stroke_width
    int result_sw = sp_desktop_query_style (_desktop, &query, QUERY_STYLE_PROPERTY_STROKEWIDTH);
    switch (result_sw) {
    case QUERY_STYLE_NOTHING:
        _stroke_width.set_markup("");
        current_stroke_width = 0;
        break;
    case QUERY_STYLE_SINGLE:
    case QUERY_STYLE_MULTIPLE_AVERAGED:
    case QUERY_STYLE_MULTIPLE_SAME:
    {
        if (query.stroke_extensions.hairline) {
            _stroke_width.set_markup(_("Hairline"));
            auto str = Glib::ustring::compose(_("Stroke width: %1"), _("Hairline"));
            _stroke_width_place.set_tooltip_text(str);
        } else {
            double w;
            if (_sw_unit) {
                w = Inkscape::Util::Quantity::convert(query.stroke_width.computed, "px", _sw_unit);
            } else {
                w = query.stroke_width.computed;
            }
            current_stroke_width = w;

            {
                gchar *str = g_strdup_printf(" %#.3g", w);
                if (str[strlen(str) - 1] == ',' || str[strlen(str) - 1] == '.') {
                    str[strlen(str)-1] = '\0';
                }
                _stroke_width.set_markup(str);
                g_free (str);
            }
            {
                gchar *str = g_strdup_printf(_("Stroke width: %.5g%s%s"),
                                             w,
                                             _sw_unit? _sw_unit->abbr.c_str() : "px",
                                             (result_sw == QUERY_STYLE_MULTIPLE_AVERAGED)?
                                             _(" (averaged)") : "");
                _stroke_width_place.set_tooltip_text(str);
                g_free (str);
            }
        }
        break;
    }
    default:
        break;
    }
}

void SelectedStyle::opacity_0() {_opacity_sb.set_value(0);}
void SelectedStyle::opacity_025() {_opacity_sb.set_value(25);}
void SelectedStyle::opacity_05() {_opacity_sb.set_value(50);}
void SelectedStyle::opacity_075() {_opacity_sb.set_value(75);}
void SelectedStyle::opacity_1() {_opacity_sb.set_value(100);}

void SelectedStyle::on_opacity_menu (Gtk::Menu *menu) {

    std::vector<Gtk::Widget *> children = menu->get_children();
    for (auto iter : children) {
        menu->remove(*iter);
    }

    {
        Gtk::MenuItem *item = new Gtk::MenuItem;
        item->add(*(new Gtk::Label(_("0 (transparent)"), Gtk::ALIGN_START, Gtk::ALIGN_START)));
        item->signal_activate().connect(sigc::mem_fun(*this, &SelectedStyle::opacity_0 ));
        menu->add(*item);
    }
    {
        Gtk::MenuItem *item = new Gtk::MenuItem;
        item->add(*(new Gtk::Label("25%", Gtk::ALIGN_START, Gtk::ALIGN_START)));
        item->signal_activate().connect(sigc::mem_fun(*this, &SelectedStyle::opacity_025 ));
        menu->add(*item);
    }
    {
        Gtk::MenuItem *item = new Gtk::MenuItem;
        item->add(*(new Gtk::Label("50%", Gtk::ALIGN_START, Gtk::ALIGN_START)));
        item->signal_activate().connect(sigc::mem_fun(*this, &SelectedStyle::opacity_05 ));
        menu->add(*item);
    }
    {
        Gtk::MenuItem *item = new Gtk::MenuItem;
        item->add(*(new Gtk::Label("75%", Gtk::ALIGN_START, Gtk::ALIGN_START)));
        item->signal_activate().connect(sigc::mem_fun(*this, &SelectedStyle::opacity_075 ));
        menu->add(*item);
    }
    {
        Gtk::MenuItem *item = new Gtk::MenuItem;
        item->add(*(new Gtk::Label(_("100% (opaque)"), Gtk::ALIGN_START, Gtk::ALIGN_START)));
        item->signal_activate().connect(sigc::mem_fun(*this, &SelectedStyle::opacity_1 ));
        menu->add(*item);
    }

    menu->show_all();
}

void SelectedStyle::on_opacity_changed ()
{
    g_return_if_fail(_desktop); // TODO this shouldn't happen!
    if (_opacity_blocked)
        return;
    _opacity_blocked = true;
    SPCSSAttr *css = sp_repr_css_attr_new ();
    Inkscape::CSSOStringStream os;
    os << CLAMP ((_opacity_adjustment->get_value() / 100), 0.0, 1.0);
    sp_repr_css_set_property (css, "opacity", os.str().c_str());

    // FIXME: workaround for GTK breakage: display interruptibility sometimes results in GTK
    // sending multiple value-changed events. As if when Inkscape interrupts redraw for main loop
    // iterations, GTK discovers that this callback hasn't finished yet, and for some weird reason
    // decides to add yet another value-changed event to the queue. Totally braindead if you ask
    // me. As a result, scrolling the spinbutton once results in runaway change until it hits 1.0
    // or 0.0. (And no, this is not a race with ::update, I checked that.)
    // Sigh. So we disable interruptibility while we're setting the new value.
    _desktop->getCanvas()->forced_redraws_start(0);
    sp_desktop_set_style (_desktop, css);
    sp_repr_css_attr_unref (css);
    DocumentUndo::maybeDone(_desktop->getDocument(), "fillstroke:opacity", SP_VERB_DIALOG_FILL_STROKE,
                            _("Change opacity"));
    // resume interruptibility
    _desktop->getCanvas()->forced_redraws_stop();
    _opacity_blocked = false;
}

/* =============================================  RotateableSwatch  */

RotateableSwatch::RotateableSwatch(SelectedStyle *parent, guint mode)
    : fillstroke(mode)
    , parent(parent)
{
}

RotateableSwatch::~RotateableSwatch() = default;

double
RotateableSwatch::color_adjust(float *hsla, double by, guint32 cc, guint modifier)
{
    SPColor::rgb_to_hsl_floatv (hsla, SP_RGBA32_R_F(cc), SP_RGBA32_G_F(cc), SP_RGBA32_B_F(cc));
    hsla[3] = SP_RGBA32_A_F(cc);
    double diff = 0;
    if (modifier == 2) { // saturation
        double old = hsla[1];
        if (by > 0) {
            hsla[1] += by * (1 - hsla[1]);
        } else {
            hsla[1] += by * (hsla[1]);
        }
        diff = hsla[1] - old;
    } else if (modifier == 1) { // lightness
        double old = hsla[2];
        if (by > 0) {
            hsla[2] += by * (1 - hsla[2]);
        } else {
            hsla[2] += by * (hsla[2]);
        }
        diff = hsla[2] - old;
    } else if (modifier == 3) { // alpha
        double old = hsla[3];
        hsla[3] += by/2;
        if (hsla[3] < 0) {
            hsla[3] = 0;
        } else if (hsla[3] > 1) {
            hsla[3] = 1;
        }
        diff = hsla[3] - old;
    } else { // hue
        double old = hsla[0];
        hsla[0] += by/2;
        while (hsla[0] < 0)
            hsla[0] += 1;
        while (hsla[0] > 1)
            hsla[0] -= 1;
        diff = hsla[0] - old;
    }

    float rgb[3];
    SPColor::hsl_to_rgb_floatv (rgb, hsla[0], hsla[1], hsla[2]);

    gchar c[64];
    sp_svg_write_color (c, sizeof(c),
        SP_RGBA32_U_COMPOSE(
                (SP_COLOR_F_TO_U(rgb[0])),
                (SP_COLOR_F_TO_U(rgb[1])),
                (SP_COLOR_F_TO_U(rgb[2])),
                0xff
        )
    );

    SPCSSAttr *css = sp_repr_css_attr_new ();

    if (modifier == 3) { // alpha
        Inkscape::CSSOStringStream osalpha;
        osalpha << hsla[3];
        sp_repr_css_set_property(css, (fillstroke == SS_FILL) ? "fill-opacity" : "stroke-opacity", osalpha.str().c_str());
    } else {
        sp_repr_css_set_property (css, (fillstroke == SS_FILL) ? "fill" : "stroke", c);
    }
    sp_desktop_set_style (parent->getDesktop(), css);
    sp_repr_css_attr_unref (css);
    return diff;
}

void
RotateableSwatch::do_motion(double by, guint modifier) {
    if (parent->_mode[fillstroke] != SS_COLOR)
        return;

    if (!scrolling && !cr_set) {

        std::string cursor_filename = "adjust_hue.svg";
        if (modifier == 2) {
            cursor_filename = "adjust_saturation.svg";
        } else if (modifier == 1) {
            cursor_filename = "adjust_lightness.svg";
        } else if (modifier == 3) {
            cursor_filename = "adjust_alpha.svg";
        }

        load_svg_cursor(get_display(), get_window(), cursor_filename);
    }

    guint32 cc;
    if (!startcolor_set) {
        cc = startcolor = parent->_thisselected[fillstroke];
        startcolor_set = true;
    } else {
        cc = startcolor;
    }

    float hsla[4];
    double diff = 0;

    diff = color_adjust(hsla, by, cc, modifier);

    if (modifier == 3) { // alpha
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, (_("Adjust alpha")));
        double ch = hsla[3];
        parent->getDesktop()->event_context->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, _("Adjusting <b>alpha</b>: was %.3g, now <b>%.3g</b> (diff %.3g); with <b>Ctrl</b> to adjust lightness, with <b>Shift</b> to adjust saturation, without modifiers to adjust hue"), ch - diff, ch, diff);

    } else if (modifier == 2) { // saturation
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, (_("Adjust saturation")));
        double ch = hsla[1];
        parent->getDesktop()->event_context->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, _("Adjusting <b>saturation</b>: was %.3g, now <b>%.3g</b> (diff %.3g); with <b>Ctrl</b> to adjust lightness, with <b>Alt</b> to adjust alpha, without modifiers to adjust hue"), ch - diff, ch, diff);

    } else if (modifier == 1) { // lightness
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, (_("Adjust lightness")));
        double ch = hsla[2];
        parent->getDesktop()->event_context->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, _("Adjusting <b>lightness</b>: was %.3g, now <b>%.3g</b> (diff %.3g); with <b>Shift</b> to adjust saturation, with <b>Alt</b> to adjust alpha, without modifiers to adjust hue"), ch - diff, ch, diff);

    } else { // hue
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, (_("Adjust hue")));
        double ch = hsla[0];
        parent->getDesktop()->event_context->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, _("Adjusting <b>hue</b>: was %.3g, now <b>%.3g</b> (diff %.3g); with <b>Shift</b> to adjust saturation, with <b>Alt</b> to adjust alpha, with <b>Ctrl</b> to adjust lightness"), ch - diff, ch, diff);
    }
}


void
RotateableSwatch::do_scroll(double by, guint modifier) {
    do_motion(by/30.0, modifier);
    do_release(by/30.0, modifier);
}

void
RotateableSwatch::do_release(double by, guint modifier) {
    if (parent->_mode[fillstroke] != SS_COLOR)
        return;

    float hsla[4];
    color_adjust(hsla, by, startcolor, modifier);

    if (cr_set) {
        get_window()->set_cursor(); // Use parent window cursor.
        cr_set = false;
    }

    if (modifier == 3) { // alpha
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, ("Adjust alpha"));
    } else if (modifier == 2) { // saturation
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, ("Adjust saturation"));

    } else if (modifier == 1) { // lightness
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, ("Adjust lightness"));

    } else { // hue
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, ("Adjust hue"));
    }

    if (!strcmp(undokey, "ssrot1")) {
        undokey = "ssrot2";
    } else {
        undokey = "ssrot1";
    }

    parent->getDesktop()->event_context->message_context->clear();
    startcolor_set = false;
}

/* =============================================  RotateableStrokeWidth  */

RotateableStrokeWidth::RotateableStrokeWidth(SelectedStyle *parent) :
    parent(parent),
    startvalue(0),
    startvalue_set(false),
    undokey("swrot1")
{
}

RotateableStrokeWidth::~RotateableStrokeWidth() = default;

double
RotateableStrokeWidth::value_adjust(double current, double by, guint /*modifier*/, bool final)
{
    double newval;
    // by is -1..1
    double max_f = 50;  // maximum width is (current * max_f), minimum - zero
    newval = current * (std::exp(std::log(max_f-1) * (by+1)) - 1) / (max_f-2);

    SPCSSAttr *css = sp_repr_css_attr_new ();
    if (final && newval < 1e-6) {
        // if dragged into zero and this is the final adjust on mouse release, delete stroke;
        // if it's not final, leave it a chance to increase again (which is not possible with "none")
        sp_repr_css_set_property (css, "stroke", "none");
    } else {
        newval = Inkscape::Util::Quantity::convert(newval, parent->_sw_unit, "px");
        Inkscape::CSSOStringStream os;
        os << newval;
        sp_repr_css_set_property (css, "stroke-width", os.str().c_str());
    }

    sp_desktop_set_style (parent->getDesktop(), css);
    sp_repr_css_attr_unref (css);
    return newval - current;
}

void
RotateableStrokeWidth::do_motion(double by, guint modifier) {

    // if this is the first motion after a mouse grab, remember the current width
    if (!startvalue_set) {
        startvalue = parent->current_stroke_width;
        // if it's 0, adjusting (which uses multiplication) will not be able to change it, so we
        // cheat and provide a non-zero value
        if (startvalue == 0)
            startvalue = 1;
        startvalue_set = true;
    }

    if (modifier == 3) { // Alt, do nothing
    } else {
        double diff = value_adjust(startvalue, by, modifier, false);
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, (_("Adjust stroke width")));
        parent->getDesktop()->event_context->message_context->setF(Inkscape::IMMEDIATE_MESSAGE, _("Adjusting <b>stroke width</b>: was %.3g, now <b>%.3g</b> (diff %.3g)"), startvalue, startvalue + diff, diff);
    }
}

void
RotateableStrokeWidth::do_release(double by, guint modifier) {

    if (modifier == 3) { // do nothing

    } else {
        value_adjust(startvalue, by, modifier, true);
        startvalue_set = false;
        DocumentUndo::maybeDone(parent->getDesktop()->getDocument(), undokey,
                                SP_VERB_DIALOG_FILL_STROKE, (_("Adjust stroke width")));
    }

    if (!strcmp(undokey, "swrot1")) {
        undokey = "swrot2";
    } else {
        undokey = "swrot1";
    }
    parent->getDesktop()->event_context->message_context->clear();
}

void
RotateableStrokeWidth::do_scroll(double by, guint modifier) {
    do_motion(by/10.0, modifier);
    startvalue_set = false;
}

Dialog::FillAndStroke *get_fill_and_stroke_panel(SPDesktop *desktop)
{
    Dialog::DialogBase *dialog = desktop->getContainer()->get_dialog(SP_VERB_DIALOG_FILL_STROKE);
    if (!dialog) {
        desktop->getContainer()->new_dialog(SP_VERB_DIALOG_FILL_STROKE);
        return dynamic_cast<Dialog::FillAndStroke *>(desktop->getContainer()->get_dialog(SP_VERB_DIALOG_FILL_STROKE));
    }
    return dynamic_cast<Dialog::FillAndStroke *>(dialog);
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
