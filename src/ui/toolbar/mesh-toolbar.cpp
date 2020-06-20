// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gradient aux toolbar
 *
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *   Tavmjong Bah <tavjong@free.fr>
 *
 * Copyright (C) 2012 Tavmjong Bah
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "mesh-toolbar.h"

#include <glibmm/i18n.h>

#include <gtkmm/comboboxtext.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "gradient-chemistry.h"
#include "gradient-drag.h"
#include "inkscape.h"
#include "verbs.h"

#include "object/sp-defs.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-stop.h"
#include "style.h"

#include "svg/css-ostringstream.h"

#include "ui/icon-names.h"
#include "ui/simple-pref-pusher.h"
#include "ui/tools/gradient-tool.h"
#include "ui/tools/mesh-tool.h"
#include "ui/widget/canvas.h"
#include "ui/widget/color-preview.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/gradient-image.h"
#include "ui/widget/spin-button-tool-item.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::MeshTool;

static bool blocked = false;

// Get a list of selected meshes taking into account fill/stroke toggles
std::vector<SPMeshGradient *>  ms_get_dt_selected_gradients(Inkscape::Selection *selection)
{
    std::vector<SPMeshGradient *> ms_selected;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool edit_fill   = prefs->getBool("/tools/mesh/edit_fill",   true);
    bool edit_stroke = prefs->getBool("/tools/mesh/edit_stroke", true);

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;// get the items gradient, not the getVector() version
        SPStyle *style = item->style;

        if (style) {

            
            if (edit_fill   && style->fill.isPaintserver()) {
                SPPaintServer *server = item->style->getFillPaintServer();
                SPMeshGradient *mesh = dynamic_cast<SPMeshGradient *>(server);
                if (mesh) {
                    ms_selected.push_back(mesh);
                }
            }

            if (edit_stroke && style->stroke.isPaintserver()) {
                SPPaintServer *server = item->style->getStrokePaintServer();
                SPMeshGradient *mesh = dynamic_cast<SPMeshGradient *>(server);
                if (mesh) {
                    ms_selected.push_back(mesh);
                }
            }
        }

    }
    return ms_selected;
}


/*
 * Get the current selection status from the desktop
 */
void ms_read_selection( Inkscape::Selection *selection,
                        SPMeshGradient *&ms_selected,
                        bool &ms_selected_multi,
                        SPMeshType &ms_type,
                        bool &ms_type_multi )
{
    ms_selected = nullptr;
    ms_selected_multi = false;
    ms_type = SP_MESH_TYPE_COONS;
    ms_type_multi = false;

    bool first = true;

    // Read desktop selection, taking into account fill/stroke toggles
    std::vector<SPMeshGradient *> meshes = ms_get_dt_selected_gradients( selection );
    for (auto & meshe : meshes) {
        if (first) {
            ms_selected = meshe;
            ms_type = meshe->type;
            first = false;
        } else {
            if (ms_selected != meshe) {
                ms_selected_multi = true;
            }
            if (ms_type != meshe->type) {
                ms_type_multi = true;
            }
        }
    }
}


/*
 * Callback functions for user actions
 */


/** Temporary hack: Returns the mesh tool in the active desktop.
 * Will go away during tool refactoring. */
static MeshTool *get_mesh_tool()
{
    MeshTool *tool = nullptr;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (SP_IS_MESH_CONTEXT(ec)) {
            tool = static_cast<MeshTool*>(ec);
        }
    }
    return tool;
}


namespace Inkscape {
namespace UI {
namespace Toolbar {
MeshToolbar::MeshToolbar(SPDesktop *desktop)
    : Toolbar(desktop),
    _edit_fill_pusher(nullptr)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    /* New mesh: normal or conical */
    {
        add_label(_("New:"));

        Gtk::RadioToolButton::Group new_type_group;

        auto normal_type_btn = Gtk::manage(new Gtk::RadioToolButton(new_type_group, _("normal")));
        normal_type_btn->set_tooltip_text(_("Create mesh gradient"));
        normal_type_btn->set_icon_name(INKSCAPE_ICON("paint-gradient-mesh"));
        _new_type_buttons.push_back(normal_type_btn);

        auto conical_type_btn = Gtk::manage(new Gtk::RadioToolButton(new_type_group, _("conical")));
        conical_type_btn->set_tooltip_text(_("Create conical gradient"));
        conical_type_btn->set_icon_name(INKSCAPE_ICON("paint-gradient-conical"));
        _new_type_buttons.push_back(conical_type_btn);

        int btn_idx = 0;
        for (auto btn : _new_type_buttons) {
            add(*btn);
            btn->set_sensitive();
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &MeshToolbar::new_geometry_changed), btn_idx++));
        }

        gint mode = prefs->getInt("/tools/mesh/mesh_geometry", SP_MESH_GEOMETRY_NORMAL);
        _new_type_buttons[mode]->set_active();
    }

    /* New gradient on fill or stroke*/
    {
        Gtk::RadioToolButton::Group new_fillstroke_group;

        auto fill_button = Gtk::manage(new Gtk::RadioToolButton(new_fillstroke_group, _("fill")));
        fill_button->set_tooltip_text(_("Create gradient in the fill"));
        fill_button->set_icon_name(INKSCAPE_ICON("object-fill"));
        _new_fillstroke_buttons.push_back(fill_button);

        auto stroke_btn = Gtk::manage(new Gtk::RadioToolButton(new_fillstroke_group, _("stroke")));
        stroke_btn->set_tooltip_text(_("Create gradient in the stroke"));
        stroke_btn->set_icon_name(INKSCAPE_ICON("object-stroke"));
        _new_fillstroke_buttons.push_back(stroke_btn);

        int btn_idx = 0;
        for(auto btn : _new_fillstroke_buttons) {
            add(*btn);
            btn->set_sensitive(true);
            btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &MeshToolbar::new_fillstroke_changed), btn_idx++));
        }

        gint mode = prefs->getInt("/tools/mesh/newfillorstroke");
        _new_fillstroke_buttons[mode]->set_active();
    }

    /* Number of mesh rows */
    {
        std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto rows_val = prefs->getDouble("/tools/mesh/mesh_rows", 1);
        _row_adj = Gtk::Adjustment::create(rows_val, 1, 20, 1, 1);
        auto row_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("mesh-row", _("Rows:"), _row_adj, 1.0, 0));
        row_item->set_tooltip_text(_("Number of rows in new mesh"));
        row_item->set_custom_numeric_menu_data(values);
        row_item->set_focus_widget(desktop->canvas);
        _row_adj->signal_value_changed().connect(sigc::mem_fun(*this, &MeshToolbar::row_changed));
        add(*row_item);
        row_item->set_sensitive(true);
    }

    /* Number of mesh columns */
    {
        std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto col_val = prefs->getDouble("/tools/mesh/mesh_cols", 1);
        _col_adj = Gtk::Adjustment::create(col_val, 1, 20, 1, 1);
        auto col_item = Gtk::manage(new UI::Widget::SpinButtonToolItem("mesh-col", _("Columns:"), _col_adj, 1.0, 0));
        col_item->set_tooltip_text(_("Number of columns in new mesh"));
        col_item->set_custom_numeric_menu_data(values);
        col_item->set_focus_widget(desktop->canvas);
        _col_adj->signal_value_changed().connect(sigc::mem_fun(*this, &MeshToolbar::col_changed));
        add(*col_item);
        col_item->set_sensitive(true);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    // TODO: These were disabled in the UI file.  Either activate or delete
#if 0
    /* Edit fill mesh */
    {
        _edit_fill_item = add_toggle_button(_("Edit Fill"),
                                            _("Edit fill mesh"));
        _edit_fill_item->set_icon_name(INKSCAPE_ICON("object-fill"));
        _edit_fill_pusher.reset(new UI::SimplePrefPusher(_edit_fill_item, "/tools/mesh/edit_fill"));
        _edit_fill_item->signal_toggled().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_fill_stroke));
    }

    /* Edit stroke mesh */
    {
        _edit_stroke_item = add_toggle_button(_("Edit Stroke"),
                                              _("Edit stroke mesh"));
        _edit_stroke_item->set_icon_name(INKSCAPE_ICON("object-stroke"));
        _edit_stroke_pusher.reset(new UI::SimplePrefPusher(_edit_stroke_item, "/tools/mesh/edit_stroke"));
        _edit_stroke_item->signal_toggled().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_fill_stroke));
    }

    /* Show/hide side and tensor handles */
    {
        auto show_handles_item = add_toggle_button(_("Show Handles"),
                                                   _("Show handles"));
        show_handles_item->set_icon_name(INKSCAPE_ICON("show-node-handles"));
        _show_handles_pusher.reset(new UI::SimplePrefPusher(show_handles_item, "/tools/mesh/show_handles"));
        show_handles_item->signal_toggled().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_handles));
    }
#endif

    desktop->connectEventContextChanged(sigc::mem_fun(*this, &MeshToolbar::watch_ec));

    {
        auto btn = Gtk::manage(new Gtk::ToolButton(_("Toggle Sides")));
        btn->set_tooltip_text(_("Toggle selected sides between Beziers and lines."));
        btn->set_icon_name(INKSCAPE_ICON("node-segment-line"));
        btn->signal_clicked().connect(sigc::mem_fun(*this, &MeshToolbar::toggle_sides));
        add(*btn);
    }

    {
        auto btn = Gtk::manage(new Gtk::ToolButton(_("Make elliptical")));
        btn->set_tooltip_text(_("Make selected sides elliptical by changing length of handles. Works best if handles already approximate ellipse."));
        btn->set_icon_name(INKSCAPE_ICON("node-segment-curve"));
        btn->signal_clicked().connect(sigc::mem_fun(*this, &MeshToolbar::make_elliptical));
        add(*btn);
    }

    {
        auto btn = Gtk::manage(new Gtk::ToolButton(_("Pick colors:")));
        btn->set_tooltip_text(_("Pick colors for selected corner nodes from underneath mesh."));
        btn->set_icon_name(INKSCAPE_ICON("color-picker"));
        btn->signal_clicked().connect(sigc::mem_fun(*this, &MeshToolbar::pick_colors));
        add(*btn);
    }


    {
        auto btn = Gtk::manage(new Gtk::ToolButton(_("Scale mesh to bounding box:")));
        btn->set_tooltip_text(_("Scale mesh to fit inside bounding box."));
        btn->set_icon_name(INKSCAPE_ICON("mesh-gradient-fit"));
        btn->signal_clicked().connect(sigc::mem_fun(*this, &MeshToolbar::fit_mesh));
        add(*btn);
    }

    add(* Gtk::manage(new Gtk::SeparatorToolItem()));

    /* Warning */
    {
        auto btn = Gtk::manage(new Gtk::ToolButton(_("WARNING: Mesh SVG Syntax Subject to Change")));
        btn->set_tooltip_text(_("WARNING: Mesh SVG Syntax Subject to Change"));
        btn->set_icon_name(INKSCAPE_ICON("dialog-warning"));
        add(*btn);
        btn->signal_clicked().connect(sigc::mem_fun(*this, &MeshToolbar::warning_popup));
        btn->set_sensitive(true);
    }

    /* Type */
    {
        UI::Widget::ComboToolItemColumns columns;
        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);
        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = C_("Type", "Coons");
        row[columns.col_sensitive] = true;

        row = *(store->append());
        row[columns.col_label    ] = _("Bicubic");
        row[columns.col_sensitive] = true;

        _select_type_item = Gtk::manage(UI::Widget::ComboToolItem::create(_("Smoothing"),
            // TRANSLATORS: Type of Smoothing. See https://en.wikipedia.org/wiki/Coons_patch
            _("Coons: no smoothing. Bicubic: smoothing across patch boundaries."),
            "Not Used", store));
        _select_type_item->use_group_label(true);

        _select_type_item->set_active(0);

        _select_type_item->signal_changed().connect(sigc::mem_fun(*this, &MeshToolbar::type_changed));
        add(*_select_type_item);
    }

    show_all();
}

/**
 * Mesh auxiliary toolbar construction and setup.
 * Don't forget to add to XML in widgets/toolbox.cpp!
 *
 */
GtkWidget *
MeshToolbar::create(SPDesktop * desktop)
{
    auto toolbar = new MeshToolbar(desktop);
    return GTK_WIDGET(toolbar->gobj());
}

void
MeshToolbar::new_geometry_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/mesh/mesh_geometry", mode);
}

void
MeshToolbar::new_fillstroke_changed(int mode)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setInt("/tools/mesh/newfillorstroke", mode);
}

void
MeshToolbar::row_changed()
{
    if (blocked) {
        return;
    }

    blocked = TRUE;

    int rows = _row_adj->get_value();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/tools/mesh/mesh_rows", rows);

    blocked = FALSE;
}

void
MeshToolbar::col_changed()
{
    if (blocked) {
        return;
    }

    blocked = TRUE;

    int cols = _col_adj->get_value();

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    prefs->setInt("/tools/mesh/mesh_cols", cols);

    blocked = FALSE;
}

void
MeshToolbar::toggle_fill_stroke()
{
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("tools/mesh/edit_fill", _edit_fill_item->get_active());
    prefs->setBool("tools/mesh/edit_stroke", _edit_stroke_item->get_active());

    MeshTool *mt = get_mesh_tool();
    if (mt) {
        GrDrag *drag = mt->_grdrag;
        drag->updateDraggers();
        drag->updateLines();
        drag->updateLevels();
        selection_changed(nullptr); // Need to update Type widget
    }
}

void
MeshToolbar::toggle_handles()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        GrDrag *drag = mt->_grdrag;
        drag->refreshDraggers();
    }
}

void
MeshToolbar::watch_ec(SPDesktop* desktop, Inkscape::UI::Tools::ToolBase* ec)
{
    if (SP_IS_MESH_CONTEXT(ec)) {
        // connect to selection modified and changed signals
        Inkscape::Selection *selection = desktop->getSelection();
        SPDocument *document = desktop->getDocument();

        c_selection_changed = selection->connectChanged(sigc::mem_fun(*this, &MeshToolbar::selection_changed));
        c_selection_modified = selection->connectModified(sigc::mem_fun(*this, &MeshToolbar::selection_modified));
        c_subselection_changed = desktop->connectToolSubselectionChanged(sigc::mem_fun(*this, &MeshToolbar::drag_selection_changed));

        c_defs_release = document->getDefs()->connectRelease(sigc::mem_fun(*this, &MeshToolbar::defs_release));
        c_defs_modified = document->getDefs()->connectModified(sigc::mem_fun(*this, &MeshToolbar::defs_modified));
        selection_changed(selection);
    } else {
        if (c_selection_changed)
            c_selection_changed.disconnect();
        if (c_selection_modified)
            c_selection_modified.disconnect();
        if (c_subselection_changed)
            c_subselection_changed.disconnect();
        if (c_defs_release)
            c_defs_release.disconnect();
        if (c_defs_modified)
            c_defs_modified.disconnect();
    }
}

void
MeshToolbar::selection_modified(Inkscape::Selection *selection, guint /*flags*/)
{
    selection_changed(selection);
}

void
MeshToolbar::drag_selection_changed(gpointer /*dragger*/)
{
    selection_changed(nullptr);
}

void
MeshToolbar::defs_release(SPObject * /*defs*/)
{
    selection_changed(nullptr);
}

void
MeshToolbar::defs_modified(SPObject * /*defs*/, guint /*flags*/)
{
    selection_changed(nullptr);
}

/*
 * Core function, setup all the widgets whenever something changes on the desktop
 */
void
MeshToolbar::selection_changed(Inkscape::Selection * /* selection */)
{
    // std::cout << "ms_tb_selection_changed" << std::endl;

    if (blocked)
        return;

    if (!_desktop) {
        return;
    }

    Inkscape::Selection *selection = _desktop->getSelection(); // take from desktop, not from args
    if (selection) {
        // ToolBase *ev = sp_desktop_event_context(desktop);
        // GrDrag *drag = NULL;
        // if (ev) {
        //     drag = ev->get_drag();
        //     // Hide/show handles?
        // }

        SPMeshGradient *ms_selected = nullptr;
        SPMeshType ms_type = SP_MESH_TYPE_COONS;
        bool ms_selected_multi = false;
        bool ms_type_multi = false; 
        ms_read_selection( selection, ms_selected, ms_selected_multi, ms_type, ms_type_multi );
        // std::cout << "   type: " << ms_type << std::endl;
        
        if (_select_type_item) {
            _select_type_item->set_sensitive(!ms_type_multi);
            blocked = TRUE;
            _select_type_item->set_active(ms_type);
            blocked = FALSE;
        }
    }
}

void
MeshToolbar::warning_popup()
{
    char *msg = _("Mesh gradients are part of SVG 2:\n"
                  "* Syntax may change.\n"
                  "* Web browser implementation is not guaranteed.\n"
                  "\n"
                  "For web: convert to bitmap (Edit->Make bitmap copy).\n"
                  "For print: export to PDF.");
    Gtk::MessageDialog dialog(msg, false, Gtk::MESSAGE_WARNING,
                              Gtk::BUTTONS_OK, true);
    dialog.run();
}

/**
 * Sets mesh type: Coons, Bicubic
 */
void
MeshToolbar::type_changed(int mode)
{
    if (blocked) {
        return;
    }

    Inkscape::Selection *selection = _desktop->getSelection();
    std::vector<SPMeshGradient *> meshes = ms_get_dt_selected_gradients(selection);

    SPMeshType type = (SPMeshType) mode;
    for (auto & meshe : meshes) {
        meshe->type = type;
        meshe->type_set = true;
        meshe->updateRepr();
    }
    if (!meshes.empty() ) {
        DocumentUndo::done(_desktop->getDocument(), SP_VERB_CONTEXT_MESH,_("Set mesh type"));
    }
}

void
MeshToolbar::toggle_sides()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_SIDE_TOGGLE );
    }
}

void
MeshToolbar::make_elliptical()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_SIDE_ARC );
    }
}

void
MeshToolbar::pick_colors()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_corner_operation( mt, MG_CORNER_COLOR_PICK );
    }
}

void
MeshToolbar::fit_mesh()
{
    MeshTool *mt = get_mesh_tool();
    if (mt) {
        sp_mesh_context_fit_mesh_in_bbox( mt );
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
