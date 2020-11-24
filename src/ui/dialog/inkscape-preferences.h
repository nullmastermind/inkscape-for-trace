// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Inkscape Preferences dialog
 */
/* Authors:
 *   Carl Hetherington
 *   Marco Scholten
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *
 * Copyright (C) 2004-2013 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_INKSCAPE_PREFERENCES_H
#define INKSCAPE_UI_DIALOG_INKSCAPE_PREFERENCES_H

#include <gtkmm/treerowreference.h>
#include <iostream>
#include <iterator>
#include <vector>
#include "ui/widget/preferences-widget.h"
#include <cstddef>
#include <gtkmm/colorbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treemodelsort.h>
#include <gtkmm/frame.h>
#include <gtkmm/notebook.h>
#include <gtkmm/textview.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelfilter.h>
#include <glibmm/regex.h>

#include "ui/dialog/dialog-base.h"

// UPDATE THIS IF YOU'RE ADDING PREFS PAGES.
// Otherwise the commands that open the dialog with the new page will fail.

enum {
    PREFS_PAGE_TOOLS,
    PREFS_PAGE_TOOLS_SELECTOR,
    PREFS_PAGE_TOOLS_NODE,
    PREFS_PAGE_TOOLS_TWEAK,
    PREFS_PAGE_TOOLS_ZOOM,
    PREFS_PAGE_TOOLS_MEASURE,
    PREFS_PAGE_TOOLS_SHAPES,
    PREFS_PAGE_TOOLS_SHAPES_RECT,
    PREFS_PAGE_TOOLS_SHAPES_3DBOX,
    PREFS_PAGE_TOOLS_SHAPES_ELLIPSE,
    PREFS_PAGE_TOOLS_SHAPES_STAR,
    PREFS_PAGE_TOOLS_SHAPES_SPIRAL,
    PREFS_PAGE_TOOLS_PENCIL,
    PREFS_PAGE_TOOLS_PEN,
    PREFS_PAGE_TOOLS_CALLIGRAPHY,
    PREFS_PAGE_TOOLS_TEXT,
    PREFS_PAGE_TOOLS_SPRAY,
    PREFS_PAGE_TOOLS_ERASER,
    PREFS_PAGE_TOOLS_PAINTBUCKET,
    PREFS_PAGE_TOOLS_GRADIENT,
    PREFS_PAGE_TOOLS_DROPPER,
    PREFS_PAGE_TOOLS_CONNECTOR,
    PREFS_PAGE_TOOLS_LPETOOL,
    PREFS_PAGE_UI,
    PREFS_PAGE_UI_THEME,
    PREFS_PAGE_UI_WINDOWS,
    PREFS_PAGE_UI_GRIDS,
    PREFS_PAGE_UI_KEYBOARD_SHORTCUTS,
    PREFS_PAGE_BEHAVIOR,
    PREFS_PAGE_BEHAVIOR_SELECTING,
    PREFS_PAGE_BEHAVIOR_TRANSFORMS,
    PREFS_PAGE_BEHAVIOR_DASHES,
    PREFS_PAGE_BEHAVIOR_SCROLLING,
    PREFS_PAGE_BEHAVIOR_SNAPPING,
    PREFS_PAGE_BEHAVIOR_STEPS,
    PREFS_PAGE_BEHAVIOR_CLONES,
    PREFS_PAGE_BEHAVIOR_MASKS,
    PREFS_PAGE_BEHAVIOR_MARKERS,
    PREFS_PAGE_BEHAVIOR_CLEANUP,
    PREFS_PAGE_IO,
    PREFS_PAGE_IO_MOUSE,
    PREFS_PAGE_IO_SVGOUTPUT,
    PREFS_PAGE_IO_SVGEXPORT,
    PREFS_PAGE_IO_CMS,
    PREFS_PAGE_IO_AUTOSAVE,
    PREFS_PAGE_IO_OPENCLIPART,
    PREFS_PAGE_SYSTEM,
    PREFS_PAGE_BITMAPS,
    PREFS_PAGE_RENDERING,
    PREFS_PAGE_SPELLCHECK,
    PREFS_PAGE_NOTFOUND
};

namespace Gtk {
class Scale;
}

namespace Inkscape {
namespace UI {
namespace Dialog {

class InkscapePreferences : public DialogBase
{
public:
    ~InkscapePreferences() override;

    static InkscapePreferences &getInstance() { return *new InkscapePreferences(); }

protected:
    Gtk::Frame _page_frame;
    Gtk::Label _page_title;
    Gtk::TreeView _page_list;
    Gtk::SearchEntry _search;
    Glib::RefPtr<Gtk::TreeStore> _page_list_model;
    Gtk::Widget *_highlighted_widget = nullptr;
    Glib::RefPtr<Gtk::TreeModelFilter> _page_list_model_filter;
    Glib::RefPtr<Gtk::TreeModelSort> _page_list_model_sort;
    std::vector<Gtk::Widget *> _search_results;
    Glib::RefPtr<Glib::Regex> _rx;
    int _num_results = 0;

    //Pagelist model columns:
    class PageListModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        PageListModelColumns()
        { Gtk::TreeModelColumnRecord::add(_col_name); Gtk::TreeModelColumnRecord::add(_col_page); Gtk::TreeModelColumnRecord::add(_col_id); }
        Gtk::TreeModelColumn<Glib::ustring> _col_name;
        Gtk::TreeModelColumn<int> _col_id;
        Gtk::TreeModelColumn<UI::Widget::DialogPage*> _col_page;
    };
    PageListModelColumns _page_list_columns;

    UI::Widget::DialogPage _page_tools;
    UI::Widget::DialogPage _page_selector;
    UI::Widget::DialogPage _page_node;
    UI::Widget::DialogPage _page_tweak;
    UI::Widget::DialogPage _page_spray;
    UI::Widget::DialogPage _page_zoom;
    UI::Widget::DialogPage _page_measure;
    UI::Widget::DialogPage _page_shapes;
    UI::Widget::DialogPage _page_pencil;
    UI::Widget::DialogPage _page_pen;
    UI::Widget::DialogPage _page_calligraphy;
    UI::Widget::DialogPage _page_text;
    UI::Widget::DialogPage _page_gradient;
    UI::Widget::DialogPage _page_connector;
    UI::Widget::DialogPage _page_dropper;
    UI::Widget::DialogPage _page_lpetool;

    UI::Widget::DialogPage _page_rectangle;
    UI::Widget::DialogPage _page_3dbox;
    UI::Widget::DialogPage _page_ellipse;
    UI::Widget::DialogPage _page_star;
    UI::Widget::DialogPage _page_spiral;
    UI::Widget::DialogPage _page_paintbucket;
    UI::Widget::DialogPage _page_eraser;

    UI::Widget::DialogPage _page_ui;
    UI::Widget::DialogPage _page_notfound;
    UI::Widget::DialogPage _page_theme;
    UI::Widget::DialogPage _page_windows;
    UI::Widget::DialogPage _page_grids;

    UI::Widget::DialogPage _page_behavior;
    UI::Widget::DialogPage _page_select;
    UI::Widget::DialogPage _page_transforms;
    UI::Widget::DialogPage _page_dashes;
    UI::Widget::DialogPage _page_scrolling;
    UI::Widget::DialogPage _page_snapping;
    UI::Widget::DialogPage _page_steps;
    UI::Widget::DialogPage _page_clones;
    UI::Widget::DialogPage _page_mask;
    UI::Widget::DialogPage _page_markers;
    UI::Widget::DialogPage _page_cleanup;

    UI::Widget::DialogPage _page_io;
    UI::Widget::DialogPage _page_mouse;
    UI::Widget::DialogPage _page_svgoutput;
    UI::Widget::DialogPage _page_svgexport;
    UI::Widget::DialogPage _page_cms;
    UI::Widget::DialogPage _page_autosave;

    UI::Widget::DialogPage _page_rendering;
    UI::Widget::DialogPage _page_system;
    UI::Widget::DialogPage _page_bitmaps;
    UI::Widget::DialogPage _page_spellcheck;

    UI::Widget::DialogPage _page_keyshortcuts;

    UI::Widget::PrefSpinButton _mouse_sens;
    UI::Widget::PrefSpinButton _mouse_thres;
    UI::Widget::PrefSlider      _mouse_grabsize;
    UI::Widget::PrefCheckButton _mouse_use_ext_input;
    UI::Widget::PrefCheckButton _mouse_switch_on_ext_input;

    UI::Widget::PrefSpinButton _scroll_wheel;
    UI::Widget::PrefSpinButton _scroll_arrow_px;
    UI::Widget::PrefSpinButton _scroll_arrow_acc;
    UI::Widget::PrefSpinButton _scroll_auto_speed;
    UI::Widget::PrefSpinButton _scroll_auto_thres;
    UI::Widget::PrefCheckButton _scroll_space;

    Gtk::Scale      *_slider_snapping_delay;

    UI::Widget::PrefCheckButton _snap_default;
    UI::Widget::PrefCheckButton _snap_indicator;
    UI::Widget::PrefCheckButton _snap_closest_only;
    UI::Widget::PrefCheckButton _snap_mouse_pointer;

    UI::Widget::PrefCombo       _steps_rot_snap;
    UI::Widget::PrefCheckButton _steps_rot_relative;
    UI::Widget::PrefCheckButton _steps_compass;
    UI::Widget::PrefSpinUnit    _steps_arrow;
    UI::Widget::PrefSpinUnit    _steps_scale;
    UI::Widget::PrefSpinUnit    _steps_inset;
    UI::Widget::PrefSpinButton  _steps_zoom;
    UI::Widget::PrefCheckButton _middle_mouse_zoom;
    UI::Widget::PrefSpinButton  _steps_rotate;

    UI::Widget::PrefRadioButton _t_sel_trans_obj;
    UI::Widget::PrefRadioButton _t_sel_trans_outl;
    UI::Widget::PrefRadioButton _t_sel_cue_none;
    UI::Widget::PrefRadioButton _t_sel_cue_mark;
    UI::Widget::PrefRadioButton _t_sel_cue_box;
    UI::Widget::PrefRadioButton _t_bbox_visual;
    UI::Widget::PrefRadioButton _t_bbox_geometric;

    UI::Widget::PrefCheckButton _t_cvg_keep_objects;
    UI::Widget::PrefCheckButton _t_cvg_convert_whole_groups;
    UI::Widget::PrefCheckButton _t_node_show_outline;
    UI::Widget::PrefCheckButton _t_node_live_outline;
    UI::Widget::PrefCheckButton _t_node_live_objects;
    UI::Widget::PrefCheckButton _t_node_pathflash_enabled;
    UI::Widget::PrefCheckButton _t_node_pathflash_selected;
    UI::Widget::PrefSpinButton  _t_node_pathflash_timeout;
    UI::Widget::PrefCheckButton _t_node_show_path_direction;
    UI::Widget::PrefCheckButton _t_node_single_node_transform_handles;
    UI::Widget::PrefCheckButton _t_node_delete_preserves_shape;
    UI::Widget::PrefColorPicker _t_node_pathoutline_color;

    UI::Widget::PrefCombo _gtk_theme;
    UI::Widget::PrefOpenFolder _sys_user_themes_dir_copy;
    UI::Widget::PrefOpenFolder _sys_user_icons_dir_copy;
    UI::Widget::PrefCombo _icon_theme;
    UI::Widget::PrefCheckButton _dark_theme;
    UI::Widget::PrefSlider _contrast_theme;
    UI::Widget::PrefCheckButton _symbolic_icons;
    UI::Widget::PrefCheckButton _symbolic_base_colors;
    UI::Widget::PrefColorPicker _symbolic_base_color;
    UI::Widget::PrefColorPicker _symbolic_warning_color;
    UI::Widget::PrefColorPicker _symbolic_error_color;
    UI::Widget::PrefColorPicker _symbolic_success_color;
    /*     Gtk::Image *_complementary_colors; */
    UI::Widget::PrefCombo _misc_small_toolbar;
    UI::Widget::PrefCombo _misc_small_secondary;
    UI::Widget::PrefCombo _misc_small_tools;
    UI::Widget::PrefCombo _menu_icons;

    UI::Widget::PrefRadioButton _win_dockable;
    UI::Widget::PrefRadioButton _win_floating;
    UI::Widget::PrefRadioButton _win_native;
    UI::Widget::PrefRadioButton _win_gtk;
    UI::Widget::PrefRadioButton _win_save_dialog_pos_on;
    UI::Widget::PrefRadioButton _win_save_dialog_pos_off;
    UI::Widget::PrefCombo       _win_default_size;
    UI::Widget::PrefRadioButton _win_ontop_none;
    UI::Widget::PrefRadioButton _win_ontop_normal;
    UI::Widget::PrefRadioButton _win_ontop_agressive;
    UI::Widget::PrefRadioButton _win_dialogs_labels_auto;
    UI::Widget::PrefRadioButton _win_dialogs_labels_off;
    UI::Widget::PrefRadioButton _win_save_geom_off;
    UI::Widget::PrefRadioButton _win_save_geom;
    UI::Widget::PrefRadioButton _win_save_geom_prefs;
    UI::Widget::PrefCheckButton _win_hide_task;
    UI::Widget::PrefCheckButton _win_save_viewport;
    UI::Widget::PrefCheckButton _win_zoom_resize;

    UI::Widget::PrefCheckButton _pencil_average_all_sketches;

    UI::Widget::PrefCheckButton _calligrapy_keep_selected;

    UI::Widget::PrefCheckButton _connector_ignore_text;

    UI::Widget::PrefRadioButton _clone_option_parallel;
    UI::Widget::PrefRadioButton _clone_option_stay;
    UI::Widget::PrefRadioButton _clone_option_transform;
    UI::Widget::PrefRadioButton _clone_option_unlink;
    UI::Widget::PrefRadioButton _clone_option_delete;
    UI::Widget::PrefCheckButton _clone_relink_on_duplicate;
    UI::Widget::PrefCheckButton _clone_to_curves;

    UI::Widget::PrefCheckButton _mask_mask_on_top;
    UI::Widget::PrefCheckButton _mask_mask_remove;
    UI::Widget::PrefRadioButton _mask_grouping_none;
    UI::Widget::PrefRadioButton _mask_grouping_separate;
    UI::Widget::PrefRadioButton _mask_grouping_all;
    UI::Widget::PrefCheckButton _mask_ungrouping;

    UI::Widget::PrefRadioButton _blur_quality_best;
    UI::Widget::PrefRadioButton _blur_quality_better;
    UI::Widget::PrefRadioButton _blur_quality_normal;
    UI::Widget::PrefRadioButton _blur_quality_worse;
    UI::Widget::PrefRadioButton _blur_quality_worst;
    UI::Widget::PrefRadioButton _filter_quality_best;
    UI::Widget::PrefRadioButton _filter_quality_better;
    UI::Widget::PrefRadioButton _filter_quality_normal;
    UI::Widget::PrefRadioButton _filter_quality_worse;
    UI::Widget::PrefRadioButton _filter_quality_worst;
    UI::Widget::PrefCheckButton _show_filters_info_box;
    UI::Widget::PrefCombo       _dockbar_style;
    UI::Widget::PrefCombo       _switcher_style;
    UI::Widget::PrefCheckButton _rendering_image_outline;
    UI::Widget::PrefSpinButton  _rendering_cache_size;
    UI::Widget::PrefSpinButton  _rendering_tile_multiplier;
    UI::Widget::PrefSpinButton  _rendering_xray_radius;
    UI::Widget::PrefSpinButton  _rendering_outline_overlay_opacity;
    UI::Widget::PrefCombo       _rendering_redraw_priority;
    UI::Widget::PrefSpinButton  _filter_multi_threaded;

    UI::Widget::PrefCheckButton _trans_scale_stroke;
    UI::Widget::PrefCheckButton _trans_scale_corner;
    UI::Widget::PrefCheckButton _trans_gradient;
    UI::Widget::PrefCheckButton _trans_pattern;
    UI::Widget::PrefRadioButton _trans_optimized;
    UI::Widget::PrefRadioButton _trans_preserved;

    UI::Widget::PrefCheckButton _dash_scale;

    UI::Widget::PrefRadioButton _sel_all;
    UI::Widget::PrefRadioButton _sel_current;
    UI::Widget::PrefRadioButton _sel_recursive;
    UI::Widget::PrefCheckButton _sel_hidden;
    UI::Widget::PrefCheckButton _sel_locked;
    UI::Widget::PrefCheckButton _sel_layer_deselects;
    UI::Widget::PrefCheckButton _sel_cycle;

    UI::Widget::PrefCheckButton _markers_color_stock;
    UI::Widget::PrefCheckButton _markers_color_custom;
    UI::Widget::PrefCheckButton _markers_color_update;

    UI::Widget::PrefCheckButton _cleanup_swatches;

    UI::Widget::PrefSpinButton  _importexport_export_res;
    UI::Widget::PrefSpinButton  _importexport_import_res;
    UI::Widget::PrefCheckButton _importexport_import_res_override;
    UI::Widget::PrefSlider      _snap_delay;
    UI::Widget::PrefSlider      _snap_weight;
    UI::Widget::PrefSlider      _snap_persistence;
    UI::Widget::PrefEntry       _font_sample;
    UI::Widget::PrefCheckButton _font_dialog;
    UI::Widget::PrefCombo       _font_unit_type;
    UI::Widget::PrefCheckButton _font_output_px;
    UI::Widget::PrefCheckButton _font_fontsdir_system;
    UI::Widget::PrefCheckButton _font_fontsdir_user;
    UI::Widget::PrefMultiEntry  _font_fontdirs_custom;

    UI::Widget::PrefCheckButton _misc_comment;
    UI::Widget::PrefCheckButton _misc_default_metadata;
    UI::Widget::PrefCheckButton _misc_forkvectors;
    UI::Widget::PrefSpinButton  _misc_gradientangle;
    UI::Widget::PrefCheckButton _misc_scripts;
    UI::Widget::PrefCheckButton _misc_namedicon_delay;

    // System page
    UI::Widget::PrefSpinButton  _misc_latency_skew;
    UI::Widget::PrefSpinButton  _misc_simpl;
    Gtk::Entry                  _sys_user_prefs;
    Gtk::Entry                  _sys_tmp_files;
    Gtk::Entry                  _sys_extension_dir;
    UI::Widget::PrefOpenFolder _sys_user_config;
    UI::Widget::PrefOpenFolder _sys_user_extension_dir;
    UI::Widget::PrefOpenFolder _sys_user_themes_dir;
    UI::Widget::PrefOpenFolder _sys_user_ui_dir;
    UI::Widget::PrefOpenFolder _sys_user_fonts_dir;
    UI::Widget::PrefOpenFolder _sys_user_icons_dir;
    UI::Widget::PrefOpenFolder _sys_user_keys_dir;
    UI::Widget::PrefOpenFolder _sys_user_palettes_dir;
    UI::Widget::PrefOpenFolder _sys_user_templates_dir;
    UI::Widget::PrefOpenFolder _sys_user_symbols_dir;
    UI::Widget::PrefOpenFolder _sys_user_paint_servers_dir;
    UI::Widget::PrefMultiEntry _sys_fontdirs_custom;
    Gtk::Entry                  _sys_user_cache;
    Gtk::Entry                  _sys_data;
    Gtk::TextView               _sys_icon;
    Gtk::ScrolledWindow         _sys_icon_scroll;
    Gtk::TextView               _sys_systemdata;
    Gtk::ScrolledWindow         _sys_systemdata_scroll;

    // UI page
    UI::Widget::PrefCombo       _ui_languages;
    UI::Widget::PrefCheckButton _ui_colorsliders_top;
    UI::Widget::PrefSpinButton  _misc_recent;
    UI::Widget::PrefCheckButton _ui_realworldzoom;
    UI::Widget::PrefCheckButton _ui_partialdynamic;
    UI::Widget::ZoomCorrRulerSlider _ui_zoom_correction;
    UI::Widget::PrefCheckButton _ui_yaxisdown;
    UI::Widget::PrefCheckButton _ui_rotationlock;
    UI::Widget::PrefCheckButton _ui_cursorscaling;

    //Spellcheck
    UI::Widget::PrefCombo       _spell_language;
    UI::Widget::PrefCombo       _spell_language2;
    UI::Widget::PrefCombo       _spell_language3;
    UI::Widget::PrefCheckButton _spell_ignorenumbers;
    UI::Widget::PrefCheckButton _spell_ignoreallcaps;

    // Bitmaps
    UI::Widget::PrefCombo       _misc_overs_bitmap;
    UI::Widget::PrefEntryFileButtonHBox       _misc_bitmap_editor;
    UI::Widget::PrefEntryFileButtonHBox       _misc_svg_editor;
    UI::Widget::PrefCheckButton _misc_bitmap_autoreload;
    UI::Widget::PrefSpinButton  _bitmap_copy_res;
    UI::Widget::PrefCheckButton _bitmap_ask;
    UI::Widget::PrefCheckButton _svg_ask;
    UI::Widget::PrefCombo       _bitmap_link;
    UI::Widget::PrefCombo       _svg_link;
    UI::Widget::PrefCombo       _bitmap_scale;
    UI::Widget::PrefSpinButton  _bitmap_import_quality;

    UI::Widget::PrefEntry       _kb_search;
    UI::Widget::PrefCombo       _kb_filelist;

    UI::Widget::PrefCheckButton _save_use_current_dir;
    UI::Widget::PrefCheckButton _save_autosave_enable;
    UI::Widget::PrefSpinButton  _save_autosave_interval;
    UI::Widget::PrefEntry       _save_autosave_path;
    UI::Widget::PrefSpinButton  _save_autosave_max;

    Gtk::ComboBoxText   _cms_display_profile;
    UI::Widget::PrefCheckButton     _cms_from_display;
    UI::Widget::PrefCombo           _cms_intent;

    UI::Widget::PrefCheckButton     _cms_softproof;
    UI::Widget::PrefCheckButton     _cms_gamutwarn;
    Gtk::ColorButton    _cms_gamutcolor;
    Gtk::ComboBoxText   _cms_proof_profile;
    UI::Widget::PrefCombo           _cms_proof_intent;
    UI::Widget::PrefCheckButton     _cms_proof_blackpoint;
    UI::Widget::PrefCheckButton     _cms_proof_preserveblack;

    Gtk::Notebook       _grids_notebook;
    UI::Widget::PrefRadioButton     _grids_no_emphasize_on_zoom;
    UI::Widget::PrefRadioButton     _grids_emphasize_on_zoom;
    UI::Widget::DialogPage          _grids_xy;
    UI::Widget::DialogPage          _grids_axonom;
    // CanvasXYGrid properties:
        UI::Widget::PrefUnit            _grids_xy_units;
        UI::Widget::PrefSpinButton      _grids_xy_origin_x;
        UI::Widget::PrefSpinButton      _grids_xy_origin_y;
        UI::Widget::PrefSpinButton      _grids_xy_spacing_x;
        UI::Widget::PrefSpinButton      _grids_xy_spacing_y;
        UI::Widget::PrefColorPicker     _grids_xy_color;
        UI::Widget::PrefColorPicker     _grids_xy_empcolor;
        UI::Widget::PrefSpinButton      _grids_xy_empspacing;
        UI::Widget::PrefCheckButton     _grids_xy_dotted;
    // CanvasAxonomGrid properties:
        UI::Widget::PrefUnit            _grids_axonom_units;
        UI::Widget::PrefSpinButton      _grids_axonom_origin_x;
        UI::Widget::PrefSpinButton      _grids_axonom_origin_y;
        UI::Widget::PrefSpinButton      _grids_axonom_spacing_y;
        UI::Widget::PrefSpinButton      _grids_axonom_angle_x;
        UI::Widget::PrefSpinButton      _grids_axonom_angle_z;
        UI::Widget::PrefColorPicker     _grids_axonom_color;
        UI::Widget::PrefColorPicker     _grids_axonom_empcolor;
        UI::Widget::PrefSpinButton      _grids_axonom_empspacing;

    // SVG Output page:
    UI::Widget::PrefCheckButton   _svgoutput_usenamedcolors;
    UI::Widget::PrefCheckButton   _svgoutput_usesodipodiabsref;
    UI::Widget::PrefSpinButton    _svgoutput_numericprecision;
    UI::Widget::PrefSpinButton    _svgoutput_minimumexponent;
    UI::Widget::PrefCheckButton   _svgoutput_inlineattrs;
    UI::Widget::PrefSpinButton    _svgoutput_indent;
    UI::Widget::PrefCombo         _svgoutput_pathformat;
    UI::Widget::PrefCheckButton   _svgoutput_forcerepeatcommands;

    // Attribute Checking controls for SVG Output page:
    UI::Widget::PrefCheckButton   _svgoutput_attrwarn;
    UI::Widget::PrefCheckButton   _svgoutput_attrremove;
    UI::Widget::PrefCheckButton   _svgoutput_stylepropwarn;
    UI::Widget::PrefCheckButton   _svgoutput_stylepropremove;
    UI::Widget::PrefCheckButton   _svgoutput_styledefaultswarn;
    UI::Widget::PrefCheckButton   _svgoutput_styledefaultsremove;
    UI::Widget::PrefCheckButton   _svgoutput_check_reading;
    UI::Widget::PrefCheckButton   _svgoutput_check_editing;
    UI::Widget::PrefCheckButton   _svgoutput_check_writing;

    // SVG Output export:
    UI::Widget::PrefCheckButton   _svgexport_insert_text_fallback;
    UI::Widget::PrefCheckButton   _svgexport_insert_mesh_polyfill;
    UI::Widget::PrefCheckButton   _svgexport_insert_hatch_polyfill;
    UI::Widget::PrefCheckButton   _svgexport_remove_marker_auto_start_reverse;
    UI::Widget::PrefCheckButton   _svgexport_remove_marker_context_paint;


    Gtk::Notebook _kb_notebook;
    UI::Widget::DialogPage _kb_page_shortcuts;
    UI::Widget::DialogPage _kb_page_modifiers;
    gboolean _kb_shortcuts_loaded;
    /*
     * Keyboard shortcut members
     */
    class ModelColumns: public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
            add(name);
            add(id);
            add(shortcut);
            add(description);
            add(shortcutkey);
            add(user_set);
        }
        ~ModelColumns() override = default;

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> id;
        Gtk::TreeModelColumn<Glib::ustring> shortcut;
        Gtk::TreeModelColumn<Glib::ustring> description;
        Gtk::TreeModelColumn<Gtk::AccelKey> shortcutkey;
        Gtk::TreeModelColumn<unsigned int> user_set;
    };
    ModelColumns _kb_columns;
    static ModelColumns &onKBGetCols();
    Glib::RefPtr<Gtk::TreeStore> _kb_store;
    Gtk::TreeView _kb_tree;
    Gtk::CellRendererAccel _kb_shortcut_renderer;
    Glib::RefPtr<Gtk::TreeModelFilter> _kb_filter;

    /*
     * Keyboard modifiers interface
     */
    class ModifierColumns: public Gtk::TreeModel::ColumnRecord {
    public:
        ModifierColumns() {
            add(name);
            add(id);
            add(description);
            add(and_modifiers);
            add(user_set);
        }
        ~ModifierColumns() override = default;

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> id;
        Gtk::TreeModelColumn<Glib::ustring> description;
        Gtk::TreeModelColumn<Glib::ustring> and_modifiers;
        Gtk::TreeModelColumn<unsigned int> user_set;
        Gtk::TreeModelColumn<unsigned int> is_enabled;
    };
    ModifierColumns _mod_columns;
    Glib::RefPtr<Gtk::TreeStore> _mod_store;
    Gtk::TreeView _mod_tree;
    Gtk::ToggleButton _kb_mod_ctrl;
    Gtk::ToggleButton _kb_mod_shift;
    Gtk::ToggleButton _kb_mod_alt;
    Gtk::ToggleButton _kb_mod_meta;
    Gtk::CheckButton _kb_mod_enabled;
    bool _kb_is_updated;

    int _minimum_width;
    int _minimum_height;
    int _natural_width;
    int _natural_height;
    bool GetSizeRequest(const Gtk::TreeModel::iterator& iter);
    void get_preferred_width_vfunc (int& minimum_width, int& natural_width) const override {
        minimum_width = _minimum_width;
        natural_width = _natural_width;
    }
    void get_preferred_width_for_height_vfunc (int height, int& minimum_width, int& natural_width) const override {
        minimum_width = _minimum_width;
        natural_width = _natural_width;
    }
    void get_preferred_height_vfunc (int& minimum_height, int& natural_height) const override {
        minimum_height = _minimum_height;
        natural_height = _natural_height;
    }
    void get_preferred_height_for_width_vfunc (int width, int& minimum_height, int& natural_height) const override {
        minimum_height = _minimum_height;
        natural_height = _natural_height;
    }
    int _sb_width;
    UI::Widget::DialogPage* _current_page;

    Gtk::TreeModel::iterator AddPage(UI::Widget::DialogPage& p, Glib::ustring title, int id);
    Gtk::TreeModel::iterator AddPage(UI::Widget::DialogPage& p, Glib::ustring title, Gtk::TreeModel::iterator parent, int id);
    Gtk::TreePath get_next_result(Gtk::TreeIter& iter, bool check_children = true);
    Gtk::TreePath get_prev_result(Gtk::TreeIter& iter, bool iterate = true);
    bool PresentPage(const Gtk::TreeModel::iterator& iter);

    static void AddSelcueCheckbox(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, bool def_value);
    static void AddGradientCheckbox(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, bool def_value);
    static void AddConvertGuidesCheckbox(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, bool def_value);
    static void AddFirstAndLastCheckbox(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, bool def_value);
    static void AddDotSizeSpinbutton(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, double def_value);
    static void AddBaseSimplifySpinbutton(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, double def_value);
    static void AddNewObjectsStyle(UI::Widget::DialogPage& p, Glib::ustring const &prefs_path, const gchar* banner = nullptr);

    void on_pagelist_selection_changed();
    void show_not_found();
    void show_nothing_on_page();
    void show_try_search();
    void on_reset_open_recent_clicked();
    void on_reset_prefs_clicked();
    void on_search_changed();
    void highlight_results(Glib::ustring const &key, Gtk::TreeModel::iterator &iter);
    void goto_first_result();

    void get_widgets_in_grid(Glib::ustring const &key, Gtk::Widget *widget);
    int num_widgets_in_grid(Glib::ustring const &key, Gtk::Widget *widget);
    void remove_highlight(Gtk::Label *label);
    void add_highlight(Gtk::Label *label, Glib::ustring const &key);

    bool recursive_filter(Glib::ustring &key, Gtk::TreeModel::const_iterator const &row);
    bool on_navigate_key_press(GdkEventKey *evt);

    void initPageTools();
    void initPageUI();
    void initPageBehavior();
    void initPageIO();

    void initPageRendering();
    void initPageSpellcheck();
    void initPageBitmaps();
    void initPageSystem();
    void initPageI18n(); // Do we still need it?
    void initKeyboardShortcuts(Gtk::TreeModel::iterator iter_ui);

    void _presentPages();

    /*
     * Functions for the Keyboard shortcut editor panel
     */
    void onKBReset();
    void onKBImport();
    void onKBExport();
    void onKBList();
    void onKBRealize();
    void onKBListKeyboardShortcuts();
    void onKBTreeEdited (const Glib::ustring& path, guint accel_key, Gdk::ModifierType accel_mods, guint hardware_keycode);
    void onKBTreeCleared(const Glib::ustring& path_string);
    bool onKBSearchKeyEvent(GdkEventKey *event);
    bool onKBSearchFilter(const Gtk::TreeModel::const_iterator& iter);
    static void onKBShortcutRenderer(Gtk::CellRenderer *rndr, Gtk::TreeIter const &iter);
    void on_modifier_selection_changed();
    void on_modifier_enabled();
    void on_modifier_edited();

private:
  Gtk::TreeModel::iterator searchRows(char const* srch, Gtk::TreeModel::iterator& iter, Gtk::TreeModel::Children list_model_childern);
  void themeChange();
  void preferDarkThemeChange();
  bool contrastChange(GdkEventButton* button_event);
  void symbolicThemeCheck();
  void toggleSymbolic();
  void changeIconsColors();
  void resetIconsColors(bool themechange = false);
  void resetIconsColorsWrapper();
  void changeIconsColor(guint32 /*color*/);
  void get_highlight_colors(guint32 &colorsetbase, guint32 &colorsetsuccess, guint32 &colorsetwarning,
                            guint32 &colorseterror);

  std::map<Glib::ustring, bool> dark_themes;
  InkscapePreferences();
  InkscapePreferences(InkscapePreferences const &d);
  InkscapePreferences operator=(InkscapePreferences const &d);
  bool _init;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif //INKSCAPE_UI_DIALOG_INKSCAPE_PREFERENCES_H

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
