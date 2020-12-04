// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkscape Preferences dialog - implementation.
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

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include "inkscape-preferences.h"

#include <fstream>
#include <strings.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "auto-save.h"
#include "cms-system.h"
#include "document.h"
#include "enums.h"
#include "inkscape-window.h"
#include "inkscape.h"
#include "message-stack.h"
#include "path-prefix.h"
#include "preferences.h"
#include "selcue.h"
#include "selection-chemistry.h"
#include "selection.h"
#include "style.h"
#include "verbs.h"

#include "display/control/canvas-grid.h"
#include "display/nr-filter-gaussian.h"

#include "extension/internal/gdkpixbuf-input.h"

#include "io/resource.h"

#include "object/color-profile.h"

#include "svg/svg-color.h"

#include "ui/interface.h"
#include "ui/shortcuts.h"
#include "ui/modifiers.h"
#include "ui/widget/style-swatch.h"

#include "widgets/desktop-widget.h"

#include <gtkmm/accelgroup.h>

#if WITH_GSPELL
# include "ui/dialog/spellcheck.h" // for get_available_langs
# ifdef _WIN32
#  include <windows.h>
# endif
#endif

namespace Inkscape {
namespace UI {
namespace Dialog {

using Inkscape::UI::Widget::DialogPage;
using Inkscape::UI::Widget::PrefCheckButton;
using Inkscape::UI::Widget::PrefRadioButton;
using Inkscape::UI::Widget::PrefSpinButton;
using Inkscape::UI::Widget::StyleSwatch;
using Inkscape::CMSSystem;

#define REMOVE_SPACES(x)                                                                                               \
    x.erase(0, x.find_first_not_of(' '));                                                                              \
    x.erase(x.find_last_not_of(' ') + 1);

/**
 * Case-insensitive and unicode normalized search of `pattern` in `string`.
 *
 * @param pattern Text to find
 * @param string Text to search in
 * @param[out] score Match score between 0.0 (not found) and 1.0 (pattern == string)
 * @return True if `pattern` is a substring of `string`.
 */
static bool fuzzy_search(Glib::ustring const &pattern, Glib::ustring const &string, float &score)
{
    Glib::ustring norm_patt = pattern.lowercase().normalize();
    Glib::ustring norm_str = string.lowercase().normalize();
    bool found = (norm_str.find(norm_patt) != Glib::ustring::npos);
    score = found ? (float)pattern.size() / (float)string.size() : 0;
    return score > 0.0 ? true : false;
}

/**
 * Case-insensitive and unicode normalized search of `pattern` in `string`.
 *
 * @param pattern Text to find
 * @param string Text to search in
 * @return True if `pattern` is a substring of `string`.
 */
static bool fuzzy_search(Glib::ustring const &pattern, Glib::ustring const &string)
{
    float score;
    return fuzzy_search(pattern, string, score);
}

/**
 * Get number of child Labels that match a key in a widget
 *
 * @param key Text to find
 * @param widget Gtk::Widget to search in
 * @return Number of matches found
 */
static int get_num_matches(Glib::ustring const &key, Gtk::Widget *widget)
{
    int matches = 0;
    if (auto label = dynamic_cast<Gtk::Label *>(widget)) {
        if (fuzzy_search(key, label->get_text().lowercase())) {
            // set score
            ++matches;
        }
    }
    std::vector<Gtk::Widget *> children;
    if (auto container = dynamic_cast<Gtk::Container *>(widget)) {
        children = container->get_children();
    } else {
        children = widget->list_mnemonic_labels();
    }
    for (auto *child : children) {
        if (int child_matches = get_num_matches(key, child)) {
            matches += child_matches;
        }
    }
    return matches;
}

/**
 * Add CSS-based highlight-class and pango highlight to a Gtk::Label
 *
 * @param label Label to add highlight to
 * @param key Text to add pango highlight
 */
void InkscapePreferences::add_highlight(Gtk::Label *label, Glib::ustring const &key)
{
    Glib::ustring text = label->get_text();
    Glib::ustring const n_text = text.lowercase().normalize();
    Glib::ustring const n_key = key.lowercase().normalize();
    label->get_style_context()->add_class("highlight");
    auto const pos = n_text.find(n_key);
    auto const len = n_key.size();
    text = Glib::Markup::escape_text(text.substr(0, pos)) + "<span weight=\"bold\" underline=\"single\">" +
           Glib::Markup::escape_text(text.substr(pos, len)) + "</span>" +
           Glib::Markup::escape_text(text.substr(pos + len));
    label->set_markup(text);
}

/**
 * Remove CSS-based highlight-class and pango highlight from a Gtk::Label
 *
 * @param label Label to remove highlight from
 * @param key Text to remove pango highlight from
 */
void InkscapePreferences::remove_highlight(Gtk::Label *label)
{
    if (label->get_use_markup()) {
        Glib::ustring text = label->get_text();
        label->set_text(text);
        label->get_style_context()->remove_class("highlight");
    }
}

InkscapePreferences::InkscapePreferences()
    : DialogBase("/dialogs/preferences", SP_VERB_DIALOG_PREFERENCES),
      _minimum_width(0),
      _minimum_height(0),
      _natural_width(900),
      _natural_height(700),
      _current_page(nullptr),
      _init(true)
{
    //get the width of a spinbutton
    Inkscape::UI::Widget::SpinButton* sb = new Inkscape::UI::Widget::SpinButton;
    sb->set_width_chars(6);
    add(*sb);
    show_all_children();
    Gtk::Requisition sreq;
    Gtk::Requisition sreq_natural;
    sb->get_preferred_size(sreq_natural, sreq);
    _sb_width = sreq.width;
    remove(*sb);
    delete sb;

    //Main HBox
    auto hbox_list_page = Gtk::manage(new Gtk::Box());
    hbox_list_page->set_border_width(12);
    hbox_list_page->set_spacing(12);
    add(*hbox_list_page);

    //Pagelist
    auto list_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 3));
    Gtk::ScrolledWindow* scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    _search.set_valign(Gtk::Align::ALIGN_START);
    list_box->pack_start(_search, false, true, 0);
    list_box->pack_start(*scrolled_window, false, true, 0);
    hbox_list_page->pack_start(*list_box, false, true, 0);
    _page_list.set_headers_visible(false);
    scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scrolled_window->set_valign(Gtk::Align::ALIGN_FILL);
    scrolled_window->set_propagate_natural_width();
    scrolled_window->set_propagate_natural_height();
    scrolled_window->add(_page_list);
    scrolled_window->set_vexpand_set(true);
    scrolled_window->set_vexpand(true);
    _page_list_model = Gtk::TreeStore::create(_page_list_columns);
    _page_list_model_filter = Gtk::TreeModelFilter::create(_page_list_model);
    _page_list_model_sort = Gtk::TreeModelSort::create(_page_list_model_filter);
    _page_list_model_sort->set_sort_column(_page_list_columns._col_name, Gtk::SortType::SORT_ASCENDING);
    _page_list.set_enable_search(false);
    _page_list.set_model(_page_list_model_sort);
    _page_list.append_column("name",_page_list_columns._col_name);
    Glib::RefPtr<Gtk::TreeSelection> page_list_selection = _page_list.get_selection();
    page_list_selection->signal_changed().connect(sigc::mem_fun(*this, &InkscapePreferences::on_pagelist_selection_changed));
    page_list_selection->set_mode(Gtk::SELECTION_BROWSE);

    // Search
    _page_list.set_search_column(-1); // this disables pop-up search!
    _search.signal_search_changed().connect(sigc::mem_fun(*this, &InkscapePreferences::on_search_changed));
    _search.set_tooltip_text("Search");
    _page_list_model_sort->set_sort_func(
        _page_list_columns._col_name, [=](Gtk::TreeModel::iterator const &a, Gtk::TreeModel::iterator const &b) -> int {
            float score_a, score_b;
            Glib::ustring key = _search.get_text().lowercase();
            if (key == "") {
                return -1;
            }
            Glib::ustring label_a = a->get_value(_page_list_columns._col_name).lowercase();
            Glib::ustring label_b = b->get_value(_page_list_columns._col_name).lowercase();
            auto *grid_a = a->get_value(_page_list_columns._col_page);
            auto *grid_b = b->get_value(_page_list_columns._col_page);
            int num_res_a = num_widgets_in_grid(key, grid_a);
            int num_res_b = num_widgets_in_grid(key, grid_b);
            fuzzy_search(key, label_a, score_a);
            fuzzy_search(key, label_b, score_b);
            if (score_a > score_b) {
                return -1;
            } else if (score_a < score_b) {
                return 1;
            } else if (num_res_a >= num_res_b) {
                return -1;
            } else if (num_res_a < num_res_b) {
                return 1;
            } else {
                return a->get_value(_page_list_columns._col_id) > b->get_value(_page_list_columns._col_id) ? -1 : 1;
            }
        });

    _search.signal_next_match().connect([=]() {
        if (_search_results.size() > 0) {
            Gtk::TreeIter curr = _page_list.get_selection()->get_selected();
            auto _page_list_selection = _page_list.get_selection();
            auto next = get_next_result(curr);
            if (next) {
                _page_list.get_model()->get_iter(next);
                _page_list.scroll_to_cell(next, *_page_list.get_column(0));
                _page_list.set_cursor(next);
            }
        }
    });

    _search.signal_previous_match().connect([=]() {
        if (_search_results.size() > 0) {
            Gtk::TreeIter curr = _page_list.get_selection()->get_selected();
            auto _page_list_selection = _page_list.get_selection();
            auto prev = get_prev_result(curr);
            if (prev) {
                _page_list.get_model()->get_iter(prev);
                _page_list.scroll_to_cell(prev, *_page_list.get_column(0));
                _page_list.set_cursor(prev);
            }
        }
    });

    _search.signal_key_press_event().connect(sigc::mem_fun(*this, &InkscapePreferences::on_navigate_key_press));

    _page_list_model_filter->set_visible_func([=](Gtk::TreeModel::const_iterator const &row) -> bool {
        auto key_lower = _search.get_text().lowercase();
        return recursive_filter(key_lower, row);
    });

    //Pages
    auto vbox_page = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    Gtk::Frame* title_frame = Gtk::manage(new Gtk::Frame());

    Gtk::ScrolledWindow* pageScroller = Gtk::manage(new Gtk::ScrolledWindow());
    pageScroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    pageScroller->set_propagate_natural_width();
    pageScroller->set_propagate_natural_height();
    pageScroller->add(*vbox_page);
    hbox_list_page->pack_start(*pageScroller, true, true, 0);

    title_frame->add(_page_title);
    vbox_page->pack_start(*title_frame, false, false, 0);
    vbox_page->pack_start(_page_frame, true, true, 0);
    _page_frame.set_shadow_type(Gtk::SHADOW_IN);
    title_frame->set_shadow_type(Gtk::SHADOW_IN);

    initPageTools();
    initPageUI();
    initPageBehavior();
    initPageIO();

    initPageSystem();
    initPageBitmaps();
    initPageRendering();
    initPageSpellcheck();

    signal_map().connect(sigc::mem_fun(*this, &InkscapePreferences::_presentPages));
    
    //calculate the size request for this dialog
    _page_list.expand_all();
    _page_list_model->foreach_iter(sigc::mem_fun(*this, &InkscapePreferences::GetSizeRequest));
    _page_list.collapse_all();
}

InkscapePreferences::~InkscapePreferences()
= default;

/**
 * Get child Labels that match a key in a widget grid
 * and add the results to global _search_results vector
 *
 * @param key Text to find
 * @param widget Gtk::Widget to search in
 */
void InkscapePreferences::get_widgets_in_grid(Glib::ustring const &key, Gtk::Widget *widget)
{
    if (auto label = dynamic_cast<Gtk::Label *>(widget)) {
        if (fuzzy_search(key, label->get_text())) {
            _search_results.push_back(widget);
        }
    }
    std::vector<Gtk::Widget *> children;
    if (auto container = dynamic_cast<Gtk::Container *>(widget)) {
        children = container->get_children();
    } else {
        children = widget->list_mnemonic_labels();
    }
    for (auto *child : children) {
        get_widgets_in_grid(key, child);
    }
}

/**
 * Get number of child Labels that match a key in a widget grid
 * and add the results to global _search_results vector
 *
 * @param key Text to find
 * @param widget Gtk::Widget to search in
 * @return Number of results found
 */
int InkscapePreferences::num_widgets_in_grid(Glib::ustring const &key, Gtk::Widget *widget)
{
    int results = 0;
    if (auto label = dynamic_cast<Gtk::Label *>(widget)) {
        float score;
        if (fuzzy_search(key, label->get_text(), score)) {
            ++results;
        }
    }
    std::vector<Gtk::Widget *> children;
    if (auto container = dynamic_cast<Gtk::Container *>(widget)) {
        children = container->get_children();
    } else {
        children = widget->list_mnemonic_labels();
    }
    for (auto *child : children) {
        int num_child = num_widgets_in_grid(key, child);
        results += num_child;
    }
    return results;
}

/**
 * Implementation of the search functionality executes each time
 * search entry is changed
 */
void InkscapePreferences::on_search_changed()
{
    _num_results = 0;
    if (_search_results.size() > 0) {
        for (auto *result : _search_results) {
            remove_highlight(static_cast<Gtk::Label *>(result));
        }
        _search_results.clear();
    }
    auto key = _search.get_text();
    _page_list_model_filter->refilter();
    // get first iter
    Gtk::TreeModel::Children children = _page_list.get_model()->children();
    Gtk::TreeModel::iterator iter = children.begin();

    highlight_results(key, iter);
    goto_first_result();
    if (key == "") {
        Gtk::TreeModel::Children children = _page_list.get_model()->children();
        Gtk::TreeModel::iterator iter = children.begin();
        _page_list.scroll_to_cell(Gtk::TreePath(iter), *_page_list.get_column(0));
        _page_list.set_cursor(Gtk::TreePath(iter));
    } else if (_num_results == 0 && key != "") {
        _page_list.set_has_tooltip(false);
        // TODO:Show all contents
        show_not_found();
    } else {
        _page_list.expand_all();
    }
}

/**
 * Select the first row in the tree that has a result
 */
void InkscapePreferences::goto_first_result()
{
    auto key = _search.get_text();
    if (_num_results > 0) {
        Gtk::TreeIter curr = _page_list.get_model()->children().begin();
        if (fuzzy_search(key, curr->get_value(_page_list_columns._col_name)) ||
            get_num_matches(key, curr->get_value(_page_list_columns._col_page)) > 0) {
            _page_list.scroll_to_cell(Gtk::TreePath(curr), *_page_list.get_column(0));
            _page_list.set_cursor(Gtk::TreePath(curr));
        } else {
            auto next = get_next_result(curr);
            if (next) {
                _page_list.scroll_to_cell(next, *_page_list.get_column(0));
                _page_list.set_cursor(next);
            }
        }
    }
}

/**
 * Look for the immediate next row in the tree that contains a search result
 *
 * @param iter Current iter the that is selected in the tree
 * @param check_children Bool whether to check if the children of the iter
 * contain search result
 * @return Immediate next row than contains a search result
 */
Gtk::TreePath InkscapePreferences::get_next_result(Gtk::TreeIter &iter, bool check_children)
{
    auto key = _search.get_text();
    Gtk::TreePath path = Gtk::TreePath(iter);
    if (iter->children().begin() && check_children) { // check for search results in children
        auto child = iter->children().begin();
        _page_list.expand_row(path, false);
        if (fuzzy_search(key, child->get_value(_page_list_columns._col_name)) ||
            get_num_matches(key, child->get_value(_page_list_columns._col_page)) > 0) {
            return Gtk::TreePath(child);
        } else {
            return get_next_result(child);
        }
    } else {
        ++iter;     // go to next row
        if (iter) { // if row exists
            if (fuzzy_search(key, iter->get_value(_page_list_columns._col_name)) ||
                get_num_matches(key, iter->get_value(_page_list_columns._col_page))) {
                path.next();
                return path;
            } else {
                return get_next_result(iter);
            }
        } else if (path.up() && path) {
            path.next();
            iter = _page_list.get_model()->get_iter(path);
            if (iter) {
                if (fuzzy_search(key, iter->get_value(_page_list_columns._col_name)) ||
                    get_num_matches(key, iter->get_value(_page_list_columns._col_page))) {
                    return Gtk::TreePath(iter);
                } else {
                    return get_next_result(iter);
                }
            } else {
                path.up();
                if (path) {
                    iter = _page_list.get_model()->get_iter(path);
                    return get_next_result(iter, false); // dont check for children
                } else {
                    return Gtk::TreePath(_page_list.get_model()->children().begin());
                }
            }
        }
    }
    assert(!iter);
    return Gtk::TreePath();
}

/**
 * Look for the immediate previous row in the tree that contains a search result
 *
 * @param iter Current iter that is selected in the tree
 * @param check_children Bool whether to check if the children of the iter
 * contain search result
 * @return Immediate previous row than contains a search result
 */
Gtk::TreePath InkscapePreferences::get_prev_result(Gtk::TreeIter &iter, bool iterate)
{
    auto key = _search.get_text();
    Gtk::TreePath path = Gtk::TreePath(iter);
    if (iterate) {
        --iter;
    }
    if (iter) {
        if (iter->children().begin()) {
            auto child = iter->children().end();
            --child;
            Gtk::TreePath path = Gtk::TreePath(iter);
            _page_list.expand_row(path, false);
            return get_prev_result(child, false);
        } else if (fuzzy_search(key, iter->get_value(_page_list_columns._col_name)) ||
                   get_num_matches(key, iter->get_value(_page_list_columns._col_page))) {
            return (Gtk::TreePath(iter));
        } else {
            return get_prev_result(iter);
        }
    } else if (path.up()) {
        if (path) {
            iter = _page_list.get_model()->get_iter(path);
            if (fuzzy_search(key, iter->get_value(_page_list_columns._col_name)) ||
                get_num_matches(key, iter->get_value(_page_list_columns._col_page))) {
                return path;
            } else {
                return get_prev_result(iter);
            }
        } else {
            auto lastIter = _page_list.get_model()->children().end();
            --lastIter;
            return get_prev_result(lastIter, false);
        }
    } else {
        return Gtk::TreePath(iter);
    }
}

/**
 * Handle key F3 and Shift+F3 key press events to navigate to the next search
 * result
 *
 * @param evt event object
 * @return Always returns True to label the key press event as handled
 */
bool InkscapePreferences::on_navigate_key_press(GdkEventKey *evt)
{
    if (evt->keyval == GDK_KEY_F3) {
        if (_search_results.size() > 0) {
            GdkModifierType modmask = gtk_accelerator_get_default_mod_mask();
            if ((evt->state & modmask) == Gdk::SHIFT_MASK) {
                Gtk::TreeIter curr = _page_list.get_selection()->get_selected();
                auto _page_list_selection = _page_list.get_selection();
                auto prev = get_prev_result(curr);
                if (prev) {
                    _page_list.scroll_to_cell(prev, *_page_list.get_column(0));
                    _page_list.set_cursor(prev);
                }
            } else {
                Gtk::TreeIter curr = _page_list.get_selection()->get_selected();
                auto _page_list_selection = _page_list.get_selection();
                auto next = get_next_result(curr);
                if (next) {
                    _page_list.scroll_to_cell(next, *_page_list.get_column(0));
                    _page_list.set_cursor(next);
                }
            }
        }
    }
    return true;
}

/**
 * Add highlight to all the search results
 *
 * @param key Text to search
 * @param iter pointing to the first row of the tree
 */
void InkscapePreferences::highlight_results(Glib::ustring const &key, Gtk::TreeModel::iterator &iter)
{
    Gtk::TreeModel::Path path;
    Glib::ustring Txt;
    while (iter) {
        Gtk::TreeModel::Row row = *iter;
        auto *grid = row->get_value(_page_list_columns._col_page);
        get_widgets_in_grid(key, grid);
        if (_search_results.size() > 0) {
            for (auto *result : _search_results) {
                // underline and highlight
                add_highlight(static_cast<Gtk::Label *>(result), key);
            }
        }
        if (iter->children()) {
            auto children = iter->children();
            auto child_iter = children.begin();
            highlight_results(key, child_iter);
        }
        iter++;
    }
}

/**
 * Filter function for the search functionality to show only matching
 * rows or rows with matching search resuls in the tree view
 *
 * @param key Text to search for
 * @param iter iter pointing to the first row of the tree view
 * @return True if the row is to be shown else return False
 */
bool InkscapePreferences::recursive_filter(Glib::ustring &key, Gtk::TreeModel::const_iterator const &iter)
{
    auto row_label = iter->get_value(_page_list_columns._col_name).lowercase();
    if (key == "") {
        return true;
    }
    if (fuzzy_search(key, row_label)) {
        ++_num_results;
        return true;
    }
    auto *grid = iter->get_value(_page_list_columns._col_page);
    int matches = get_num_matches(key, grid);
    _num_results += matches;
    if (matches) {
        return true;
    }
    auto child = iter->children().begin();
    if (child) {
        for (auto inner = child; inner; ++inner) {
            if (recursive_filter(key, inner)) {
                return true;
            }
        }
    }
    return false;
}

Gtk::TreeModel::iterator InkscapePreferences::AddPage(DialogPage& p, Glib::ustring title, int id)
{
    return AddPage(p, title, Gtk::TreeModel::iterator() , id);
}

Gtk::TreeModel::iterator InkscapePreferences::AddPage(DialogPage& p, Glib::ustring title, Gtk::TreeModel::iterator parent, int id)
{
    Gtk::TreeModel::iterator iter;
    if (parent)
       iter = _page_list_model->append((*parent).children());
    else
       iter = _page_list_model->append();
    Gtk::TreeModel::Row row = *iter;
    row[_page_list_columns._col_name] = title;
    row[_page_list_columns._col_id] = id;
    row[_page_list_columns._col_page] = &p;
    return iter;
}

void InkscapePreferences::AddSelcueCheckbox(DialogPage &p, Glib::ustring const &prefs_path, bool def_value)
{
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Show selection cue"), prefs_path + "/selcue", def_value);
    p.add_line( false, "", *cb, "", _("Whether selected objects display a selection cue (the same as in selector)"));
}

void InkscapePreferences::AddGradientCheckbox(DialogPage &p, Glib::ustring const &prefs_path, bool def_value)
{
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Enable gradient editing"), prefs_path + "/gradientdrag", def_value);
    p.add_line( false, "", *cb, "", _("Whether selected objects display gradient editing controls"));
}

void InkscapePreferences::AddConvertGuidesCheckbox(DialogPage &p, Glib::ustring const &prefs_path, bool def_value) {
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Conversion to guides uses edges instead of bounding box"), prefs_path + "/convertguides", def_value);
    p.add_line( false, "", *cb, "", _("Converting an object to guides places these along the object's true edges (imitating the object's shape), not along the bounding box"));
}

void InkscapePreferences::AddDotSizeSpinbutton(DialogPage &p, Glib::ustring const &prefs_path, double def_value)
{
    PrefSpinButton* sb = Gtk::manage( new PrefSpinButton);
    sb->init ( prefs_path + "/dot-size", 0.0, 1000.0, 0.1, 10.0, def_value, false, false);
    p.add_line( false, _("Ctrl+click _dot size:"), *sb, _("times current stroke width"),
                       _("Size of dots created with Ctrl+click (relative to current stroke width)"),
                       false );
}

void InkscapePreferences::AddBaseSimplifySpinbutton(DialogPage &p, Glib::ustring const &prefs_path, double def_value)
{
    PrefSpinButton* sb = Gtk::manage( new PrefSpinButton);
    sb->init ( prefs_path + "/base-simplify", 0.0, 100.0, 1.0, 10.0, def_value, false, false);
    p.add_line( false, _("Base simplify:"), *sb, _("on dynamic LPE simplify"),
                       _("Base simplify of dynamic LPE based simplify"),
                       false );
}


static void StyleFromSelectionToTool(Glib::ustring const &prefs_path, StyleSwatch *swatch)
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop == nullptr)
        return;

    Inkscape::Selection *selection = desktop->getSelection();

    if (selection->isEmpty()) {
        desktop->getMessageStack()->flash(Inkscape::ERROR_MESSAGE,
                                       _("<b>No objects selected</b> to take the style from."));
        return;
    }
    SPItem *item = selection->singleItem();
    if (!item) {
        /* TODO: If each item in the selection has the same style then don't consider it an error.
         * Maybe we should try to handle multiple selections anyway, e.g. the intersection of the
         * style attributes for the selected items. */
        desktop->getMessageStack()->flash(Inkscape::ERROR_MESSAGE,
                                       _("<b>More than one object selected.</b>  Cannot take style from multiple objects."));
        return;
    }

    SPCSSAttr *css = take_style_from_item (item);

    if (!css) return;

    // remove black-listed properties
    css = sp_css_attr_unset_blacklist (css);

    // only store text style for the text tool
    if (prefs_path != "/tools/text") {
        css = sp_css_attr_unset_text (css);
    }

    // we cannot store properties with uris - they will be invalid in other documents
    css = sp_css_attr_unset_uris (css);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setStyle(prefs_path + "/style", css);
    sp_repr_css_attr_unref (css);

    // update the swatch
    if (swatch) {
        SPCSSAttr *css = prefs->getInheritedStyle(prefs_path + "/style");
        swatch->setStyle (css);
        sp_repr_css_attr_unref(css);
    }
}

void InkscapePreferences::AddNewObjectsStyle(DialogPage &p, Glib::ustring const &prefs_path, const gchar *banner)
{
    if (banner)
        p.add_group_header(banner);
    else
        p.add_group_header( _("Style of new objects"));
    PrefRadioButton* current = Gtk::manage( new PrefRadioButton);
    current->init ( _("Last used style"), prefs_path + "/usecurrent", 1, true, nullptr);
    p.add_line( true, "", *current, "",
                _("Apply the style you last set on an object"));

    PrefRadioButton* own = Gtk::manage( new PrefRadioButton);
    auto hb = Gtk::manage( new Gtk::Box);
    own->init ( _("This tool's own style:"), prefs_path + "/usecurrent", 0, false, current);
    own->set_halign(Gtk::ALIGN_START);
    own->set_valign(Gtk::ALIGN_START);
    hb->add(*own);
    p.set_tip( *own, _("Each tool may store its own style to apply to the newly created objects. Use the button below to set it."));
    p.add_line( true, "", *hb, "", "");

    // style swatch
    Gtk::Button* button = Gtk::manage( new Gtk::Button(_("Take from selection"), true));
    StyleSwatch *swatch = nullptr;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getInt(prefs_path + "/usecurrent")) {
        button->set_sensitive(false);
    }

    SPCSSAttr *css = prefs->getStyle(prefs_path + "/style");
    swatch = new StyleSwatch(css, _("This tool's style of new objects"));
    hb->add(*swatch);
    sp_repr_css_attr_unref(css);

    button->signal_clicked().connect( sigc::bind( sigc::ptr_fun(StyleFromSelectionToTool), prefs_path, swatch)  );
    own->changed_signal.connect( sigc::mem_fun(*button, &Gtk::Button::set_sensitive) );
    p.add_line( true, "", *button, "",
                _("Remember the style of the (first) selected object as this tool's style"));
}

void InkscapePreferences::initPageTools()
{
    Gtk::TreeModel::iterator iter_tools = this->AddPage(_page_tools, _("Tools"), PREFS_PAGE_TOOLS);
    this->AddPage(_page_selector, _("Selector"), iter_tools, PREFS_PAGE_TOOLS_SELECTOR);
    this->AddPage(_page_node, _("Node"), iter_tools, PREFS_PAGE_TOOLS_NODE);

    // shapes
    Gtk::TreeModel::iterator iter_shapes = this->AddPage(_page_shapes, _("Shapes"), iter_tools, PREFS_PAGE_TOOLS_SHAPES);
    this->AddPage(_page_rectangle, _("Rectangle"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_RECT);
    this->AddPage(_page_ellipse, _("Ellipse"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_ELLIPSE);
    this->AddPage(_page_star, _("Star"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_STAR);
    this->AddPage(_page_3dbox, _("3D Box"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_3DBOX);
    this->AddPage(_page_spiral, _("Spiral"), iter_shapes, PREFS_PAGE_TOOLS_SHAPES_SPIRAL);

    this->AddPage(_page_pen, _("Pen"), iter_tools, PREFS_PAGE_TOOLS_PEN);
    this->AddPage(_page_pencil, _("Pencil"), iter_tools, PREFS_PAGE_TOOLS_PENCIL);
    this->AddPage(_page_calligraphy, _("Calligraphy"), iter_tools, PREFS_PAGE_TOOLS_CALLIGRAPHY);
    this->AddPage(_page_text, C_("ContextVerb", "Text"), iter_tools, PREFS_PAGE_TOOLS_TEXT);

    this->AddPage(_page_gradient, _("Gradient"), iter_tools, PREFS_PAGE_TOOLS_GRADIENT);
    this->AddPage(_page_dropper, _("Dropper"), iter_tools, PREFS_PAGE_TOOLS_DROPPER);
    this->AddPage(_page_paintbucket, _("Paint Bucket"), iter_tools, PREFS_PAGE_TOOLS_PAINTBUCKET);

    this->AddPage(_page_tweak, _("Tweak"), iter_tools, PREFS_PAGE_TOOLS_TWEAK);
    this->AddPage(_page_spray, _("Spray"), iter_tools, PREFS_PAGE_TOOLS_SPRAY);
    this->AddPage(_page_eraser, _("Eraser"), iter_tools, PREFS_PAGE_TOOLS_ERASER);
    this->AddPage(_page_connector, _("Connector"), iter_tools, PREFS_PAGE_TOOLS_CONNECTOR);
#ifdef WITH_LPETOOL
    this->AddPage(_page_lpetool, _("LPE Tool"), iter_tools, PREFS_PAGE_TOOLS_LPETOOL);
#endif // WITH_LPETOOL
    this->AddPage(_page_zoom, _("Zoom"), iter_tools, PREFS_PAGE_TOOLS_ZOOM);
    this->AddPage(_page_measure, C_("ContextVerb", "Measure"), iter_tools, PREFS_PAGE_TOOLS_MEASURE);

    _page_tools.add_group_header( _("Bounding box to use"));
    _t_bbox_visual.init ( _("Visual bounding box"), "/tools/bounding_box", 0, false, nullptr); // 0 means visual
    _page_tools.add_line( true, "", _t_bbox_visual, "",
                            _("This bounding box includes stroke width, markers, filter margins, etc."));
    _t_bbox_geometric.init ( _("Geometric bounding box"), "/tools/bounding_box", 1, true, &_t_bbox_visual); // 1 means geometric
    _page_tools.add_line( true, "", _t_bbox_geometric, "",
                            _("This bounding box includes only the bare path"));

    _page_tools.add_group_header( _("Conversion to guides"));
    _t_cvg_keep_objects.init ( _("Keep objects after conversion to guides"), "/tools/cvg_keep_objects", false);
    _page_tools.add_line( true, "", _t_cvg_keep_objects, "",
                            _("When converting an object to guides, don't delete the object after the conversion"));
    _t_cvg_convert_whole_groups.init ( _("Treat groups as a single object"), "/tools/cvg_convert_whole_groups", false);
    _page_tools.add_line( true, "", _t_cvg_convert_whole_groups, "",
                            _("Treat groups as a single object during conversion to guides rather than converting each child separately"));

    _pencil_average_all_sketches.init ( _("Average all sketches"), "/tools/freehand/pencil/average_all_sketches", false);
    _calligrapy_keep_selected.init ( _("Select new path"), "/tools/calligraphic/keep_selected", true);
    _connector_ignore_text.init( _("Don't attach connectors to text objects"), "/tools/connector/ignoretext", true);

    //Selector


    AddSelcueCheckbox(_page_selector, "/tools/select", false);
    AddGradientCheckbox(_page_selector, "/tools/select", false);

    _page_selector.add_group_header( _("When transforming, show"));
    _t_sel_trans_obj.init ( _("Objects"), "/tools/select/show", "content", true, nullptr);
    _page_selector.add_line( true, "", _t_sel_trans_obj, "",
                            _("Show the actual objects when moving or transforming"));
    _t_sel_trans_outl.init ( _("Box outline"), "/tools/select/show", "outline", false, &_t_sel_trans_obj);
    _page_selector.add_line( true, "", _t_sel_trans_outl, "",
                            _("Show only a box outline of the objects when moving or transforming"));
    _page_selector.add_group_header( _("Per-object selection cue"));
    _t_sel_cue_none.init ( C_("Selection cue", "None"), "/options/selcue/value", Inkscape::SelCue::NONE, false, nullptr);
    _page_selector.add_line( true, "", _t_sel_cue_none, "",
                            _("No per-object selection indication"));
    _t_sel_cue_mark.init ( _("Mark"), "/options/selcue/value", Inkscape::SelCue::MARK, true, &_t_sel_cue_none);
    _page_selector.add_line( true, "", _t_sel_cue_mark, "",
                            _("Each selected object has a diamond mark in the top left corner"));
    _t_sel_cue_box.init ( _("Box"), "/options/selcue/value", Inkscape::SelCue::BBOX, false, &_t_sel_cue_none);
    _page_selector.add_line( true, "", _t_sel_cue_box, "",
                            _("Each selected object displays its bounding box"));

    //Node
    AddSelcueCheckbox(_page_node, "/tools/nodes", true);
    AddGradientCheckbox(_page_node, "/tools/nodes", true);
    _page_node.add_group_header( _("Path outline"));
    _t_node_pathoutline_color.init(_("Path outline color"), "/tools/nodes/highlight_color", 0xff0000ff);
    _page_node.add_line( false, "", _t_node_pathoutline_color, "", _("Selects the color used for showing the path outline"), false);
    _t_node_show_outline.init(_("Always show outline"), "/tools/nodes/show_outline", false);
    _page_node.add_line( true, "", _t_node_show_outline, "", _("Show outlines for all paths, not only invisible paths"));
    _t_node_live_outline.init(_("Update outline when dragging nodes"), "/tools/nodes/live_outline", false);
    _page_node.add_line( true, "", _t_node_live_outline, "", _("Update the outline when dragging or transforming nodes; if this is off, the outline will only update when completing a drag"));
    _t_node_live_objects.init(_("Update paths when dragging nodes"), "/tools/nodes/live_objects", false);
    _page_node.add_line( true, "", _t_node_live_objects, "", _("Update paths when dragging or transforming nodes; if this is off, paths will only be updated when completing a drag"));
    _t_node_show_path_direction.init(_("Show path direction on outlines"), "/tools/nodes/show_path_direction", false);
    _page_node.add_line( true, "", _t_node_show_path_direction, "", _("Visualize the direction of selected paths by drawing small arrows in the middle of each outline segment"));
    _t_node_pathflash_enabled.init ( _("Show temporary path outline"), "/tools/nodes/pathflash_enabled", false);
    _page_node.add_line( true, "", _t_node_pathflash_enabled, "", _("When hovering over a path, briefly flash its outline"));
    _t_node_pathflash_selected.init ( _("Show temporary outline for selected paths"), "/tools/nodes/pathflash_selected", false);
    _page_node.add_line( true, "", _t_node_pathflash_selected, "", _("Show temporary outline even when a path is selected for editing"));
    _t_node_pathflash_timeout.init("/tools/nodes/pathflash_timeout", 0, 10000.0, 100.0, 100.0, 1000.0, true, false);
    _page_node.add_line( false, _("_Flash time:"), _t_node_pathflash_timeout, "ms", _("Specifies how long the path outline will be visible after a mouse-over (in milliseconds); specify 0 to have the outline shown until mouse leaves the path"), false);
    _page_node.add_group_header(_("Editing preferences"));
    _t_node_single_node_transform_handles.init(_("Show transform handles for single nodes"), "/tools/nodes/single_node_transform_handles", false);
    _page_node.add_line( true, "", _t_node_single_node_transform_handles, "", _("Show transform handles even when only a single node is selected"));
    _t_node_delete_preserves_shape.init(_("Deleting nodes preserves shape"), "/tools/nodes/delete_preserves_shape", true);
    _page_node.add_line( true, "", _t_node_delete_preserves_shape, "", _("Move handles next to deleted nodes to resemble original shape; hold Ctrl to get the other behavior"));

    //Tweak
    this->AddNewObjectsStyle(_page_tweak, "/tools/tweak", _("Object paint style"));
    AddSelcueCheckbox(_page_tweak, "/tools/tweak", true);
    AddGradientCheckbox(_page_tweak, "/tools/tweak", false);

    //Zoom
    AddSelcueCheckbox(_page_zoom, "/tools/zoom", true);
    AddGradientCheckbox(_page_zoom, "/tools/zoom", false);

    //Measure
    PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
    cb->init ( _("Ignore first and last points"), "/tools/measure/ignore_1st_and_last", true);
    _page_measure.add_line( false, "", *cb, "", _("The start and end of the measurement tool's control line will not be considered for calculating lengths. Only lengths between actual curve intersections will be displayed."));

    //Shapes
    this->AddSelcueCheckbox(_page_shapes, "/tools/shapes", true);
    this->AddGradientCheckbox(_page_shapes, "/tools/shapes", true);

    //Rectangle
    this->AddNewObjectsStyle(_page_rectangle, "/tools/shapes/rect");
    this->AddConvertGuidesCheckbox(_page_rectangle, "/tools/shapes/rect", true);

    //3D box
    this->AddNewObjectsStyle(_page_3dbox, "/tools/shapes/3dbox");
    this->AddConvertGuidesCheckbox(_page_3dbox, "/tools/shapes/3dbox", true);

    //Ellipse
    this->AddNewObjectsStyle(_page_ellipse, "/tools/shapes/arc");

    //Star
    this->AddNewObjectsStyle(_page_star, "/tools/shapes/star");

    //Spiral
    this->AddNewObjectsStyle(_page_spiral, "/tools/shapes/spiral");

    //Pencil
    this->AddSelcueCheckbox(_page_pencil, "/tools/freehand/pencil", true);
    this->AddNewObjectsStyle(_page_pencil, "/tools/freehand/pencil");
    this->AddDotSizeSpinbutton(_page_pencil, "/tools/freehand/pencil", 3.0);
    this->AddBaseSimplifySpinbutton(_page_pencil, "/tools/freehand/pencil", 25.0);
    _page_pencil.add_group_header( _("Sketch mode"));
    _page_pencil.add_line( true, "", _pencil_average_all_sketches, "",
                            _("If on, the sketch result will be the normal average of all sketches made, instead of averaging the old result with the new sketch"));

    //Pen
    this->AddSelcueCheckbox(_page_pen, "/tools/freehand/pen", true);
    this->AddNewObjectsStyle(_page_pen, "/tools/freehand/pen");
    this->AddDotSizeSpinbutton(_page_pen, "/tools/freehand/pen", 3.0);

    //Calligraphy
    this->AddSelcueCheckbox(_page_calligraphy, "/tools/calligraphic", false);
    this->AddNewObjectsStyle(_page_calligraphy, "/tools/calligraphic");
    _page_calligraphy.add_line( false, "", _calligrapy_keep_selected, "",
                            _("If on, each newly created object will be selected (deselecting previous selection)"));

    //Text
    this->AddSelcueCheckbox(_page_text, "/tools/text", true);
    this->AddGradientCheckbox(_page_text, "/tools/text", true);
    {
        PrefCheckButton* cb = Gtk::manage( new PrefCheckButton);
        cb->init ( _("Show font samples in the drop-down list"), "/tools/text/show_sample_in_list", true);
        _page_text.add_line( false, "", *cb, "", _("Show font samples alongside font names in the drop-down list in Text bar"));

        _font_dialog.init(_("Show font substitution warning dialog"), "/options/font/substitutedlg", false);
        _page_text.add_line( false, "", _font_dialog, "", _("Show font substitution warning dialog when requested fonts are not available on the system"));
        _font_sample.init("/tools/text/font_sample", true);
        _page_text.add_line( false, _("Font sample"), _font_sample, "", _("Edit font sample for the family in text tool controls"), true);
        
        cb = Gtk::manage(new PrefCheckButton);
        cb->init ( _("Use SVG2 auto-flowed text"),  "/tools/text/use_svg2", true);
        _page_text.add_line( false, "", *cb, "", _("Use SVG2 auto-flowed text instead of SVG1.2 auto-flowed text. (Recommended)"));
    }

    //_page_text.add_group_header( _("Text units"));
    //_font_output_px.init ( _("Always output text size in pixels (px)"), "/options/font/textOutputPx", true);
    //_page_text.add_line( true, "", _font_output_px, "", _("Always convert the text size units above into pixels (px) before saving to file"));

    _page_text.add_group_header( _("Font directories"));
    _font_fontsdir_system.init( _("Use Inkscape's fonts directory"), "/options/font/use_fontsdir_system", true);
    _page_text.add_line( true, "", _font_fontsdir_system, "", _("Load additional fonts from \"fonts\" directory located in Inkscape's global \"share\" directory"));
    _font_fontsdir_user.init( _("Use user's fonts directory"), "/options/font/use_fontsdir_user", true);
    _page_text.add_line( true, "", _font_fontsdir_user, "", _("Load additional fonts from \"fonts\" directory located in Inkscape's user configuration directory"));
    _font_fontdirs_custom.init("/options/font/custom_fontdirs", 50);
    _page_text.add_line(true, _("Additional font directories"), _font_fontdirs_custom, "", _("Load additional fonts from custom locations (one path per line)"), true);


    this->AddNewObjectsStyle(_page_text, "/tools/text");

    //Spray
    AddSelcueCheckbox(_page_spray, "/tools/spray", true);
    AddGradientCheckbox(_page_spray, "/tools/spray", false);

    //Eraser
    this->AddNewObjectsStyle(_page_eraser, "/tools/eraser");

    //Paint Bucket
    this->AddSelcueCheckbox(_page_paintbucket, "/tools/paintbucket", false);
    this->AddNewObjectsStyle(_page_paintbucket, "/tools/paintbucket");

    //Gradient
    this->AddSelcueCheckbox(_page_gradient, "/tools/gradient", true);
    _misc_forkvectors.init( _("Prevent sharing of gradient definitions"), "/options/forkgradientvectors/value", true);
    _page_gradient.add_line( false, "", _misc_forkvectors, "",
                           _("When on, shared gradient definitions are automatically forked on change; uncheck to allow sharing of gradient definitions so that editing one object may affect other objects using the same gradient"), true);

    _misc_gradientangle.init("/dialogs/gradienteditor/angle", -359, 359, 1, 90, 0, false, false);
    _page_gradient.add_line( false, _("Linear gradient _angle:"), _misc_gradientangle, "",
                           _("Default angle of new linear gradients in degrees (clockwise from horizontal)"), false);


    //Dropper
    this->AddSelcueCheckbox(_page_dropper, "/tools/dropper", true);
    this->AddGradientCheckbox(_page_dropper, "/tools/dropper", true);

    //Connector
    this->AddSelcueCheckbox(_page_connector, "/tools/connector", true);
    _page_connector.add_line(false, "", _connector_ignore_text, "",
            _("If on, connector attachment points will not be shown for text objects"));

#ifdef WITH_LPETOOL
    //LPETool
    //disabled, because the LPETool is not finished yet.
    this->AddNewObjectsStyle(_page_lpetool, "/tools/lpetool");
#endif // WITH_LPETOOL
}

static void _inkscape_fill_gtk(const gchar *path, std::map<Glib::ustring, bool> &dark_themes)
{
    const gchar *dir_entry;
    GDir *dir = g_dir_open(path, 0, NULL);
    if (!dir)
        return;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring themename = prefs->getString("/theme/gtkTheme");
    while ((dir_entry = g_dir_read_name(dir))) {
        gchar *filename = g_build_filename(path, dir_entry, "gtk-3.0", "gtk.css", NULL);
        bool has_prefer_dark = false;

        Glib::ustring theme = dir_entry;
        gchar *filenamedark = g_build_filename(path, dir_entry, "gtk-3.0", "gtk-dark.css", NULL);
        if (g_file_test(filenamedark, G_FILE_TEST_IS_REGULAR))
            has_prefer_dark = true;
        if (dark_themes.find(theme) != dark_themes.end() && !has_prefer_dark) {
            continue;
        }
        if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
            dark_themes[theme] = has_prefer_dark;
        }
        g_free(filename);
        g_free(filenamedark);
    }

    g_dir_close(dir);
}

void InkscapePreferences::get_highlight_colors(guint32 &colorsetbase, guint32 &colorsetsuccess,
                                               guint32 &colorsetwarning, guint32 &colorseterror)
{
    using namespace Inkscape::IO::Resource;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
    if (themeiconname == prefs->getString("/theme/defaultIconTheme")) {
        themeiconname = "hicolor";
    }
    Glib::ustring prefix = "";
    if (prefs->getBool("/theme/darkTheme", false)) {
        prefix = ".dark ";
    }
    auto higlight = get_filename_string(ICONS, (themeiconname + "/highlights.css").c_str(), false, true);
    if (!higlight.empty()) {
        std::ifstream ifs(higlight);
        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        Glib::ustring result;
        size_t startpos = content.find(prefix + ".base");
        size_t endpos = content.find("}");
        if (startpos != std::string::npos) {
            result = content.substr(startpos, endpos - startpos);
            size_t startposin = result.find("fill:");
            size_t endposin = result.find(";");
            result = result.substr(startposin + 5, endposin - (startposin + 5));
            REMOVE_SPACES(result);
            Gdk::RGBA base_color = Gdk::RGBA(result);
            SPColor base_color_sp(base_color.get_red(), base_color.get_green(), base_color.get_blue());
            colorsetbase = base_color_sp.toRGBA32(base_color.get_alpha());
        }
        content.erase(0, endpos + 1);
        startpos = content.find(prefix + ".success");
        endpos = content.find("}");
        if (startpos != std::string::npos) {
            result = content.substr(startpos, endpos - startpos);
            size_t startposin = result.find("fill:");
            size_t endposin = result.find(";");
            result = result.substr(startposin + 5, endposin - (startposin + 5));
            REMOVE_SPACES(result);
            Gdk::RGBA success_color = Gdk::RGBA(result);
            SPColor success_color_sp(success_color.get_red(), success_color.get_green(), success_color.get_blue());
            colorsetsuccess = success_color_sp.toRGBA32(success_color.get_alpha());
        }
        content.erase(0, endpos + 1);
        startpos = content.find(prefix + ".warning");
        endpos = content.find("}");
        if (startpos != std::string::npos) {
            result = content.substr(startpos, endpos - startpos);
            size_t startposin = result.find("fill:");
            size_t endposin = result.find(";");
            result = result.substr(startposin + 5, endposin - (startposin + 5));
            REMOVE_SPACES(result);
            Gdk::RGBA warning_color = Gdk::RGBA(result);
            SPColor warning_color_sp(warning_color.get_red(), warning_color.get_green(), warning_color.get_blue());
            colorsetwarning = warning_color_sp.toRGBA32(warning_color.get_alpha());
        }
        content.erase(0, endpos + 1);
        startpos = content.find(prefix + ".error");
        endpos = content.find("}");
        if (startpos != std::string::npos) {
            result = content.substr(startpos, endpos - startpos);
            size_t startposin = result.find("fill:");
            size_t endposin = result.find(";");
            result = result.substr(startposin + 5, endposin - (startposin + 5));
            REMOVE_SPACES(result);
            Gdk::RGBA error_color = Gdk::RGBA(result);
            SPColor error_color_sp(error_color.get_red(), error_color.get_green(), error_color.get_blue());
            colorseterror = error_color_sp.toRGBA32(error_color.get_alpha());
        }
    }
}

void InkscapePreferences::resetIconsColors(bool themechange)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
    if (!prefs->getBool("/theme/symbolicIcons", false)) {
        _symbolic_base_colors.set_sensitive(false);
        _symbolic_base_color.setSensitive(false);
        _symbolic_success_color.setSensitive(false);
        _symbolic_warning_color.setSensitive(false);
        _symbolic_error_color.setSensitive(false);
        return;
    }
    if (prefs->getBool("/theme/symbolicDefaultColors", true) ||
        !prefs->getEntry("/theme/" + themeiconname + "/symbolicBaseColor").isValid()) {
        auto const screen = Gdk::Screen::get_default();
        if (INKSCAPE.colorizeprovider) {
            Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.colorizeprovider);
        }
        // This colors are setted on style.css of inkscape
        Gdk::RGBA base_color = _symbolic_base_color.get_style_context()->get_color();
        // This is a hack to fix a proble style is not updated enoght fast on
        // chage from dark to bright themes
        if (themechange) {
            base_color = _symbolic_base_color.get_style_context()->get_background_color();
        }
        Gdk::RGBA success_color = _symbolic_success_color.get_style_context()->get_color();
        Gdk::RGBA warning_color = _symbolic_warning_color.get_style_context()->get_color();
        Gdk::RGBA error_color = _symbolic_error_color.get_style_context()->get_color();
        SPColor base_color_sp(base_color.get_red(), base_color.get_green(), base_color.get_blue());
        SPColor success_color_sp(success_color.get_red(), success_color.get_green(), success_color.get_blue());
        SPColor warning_color_sp(warning_color.get_red(), warning_color.get_green(), warning_color.get_blue());
        SPColor error_color_sp(error_color.get_red(), error_color.get_green(), error_color.get_blue());
        guint32 colorsetbase = base_color_sp.toRGBA32(base_color.get_alpha());
        guint32 colorsetsuccess = success_color_sp.toRGBA32(success_color.get_alpha());
        guint32 colorsetwarning = warning_color_sp.toRGBA32(warning_color.get_alpha());
        guint32 colorseterror = error_color_sp.toRGBA32(error_color.get_alpha());
        get_highlight_colors(colorsetbase, colorsetsuccess, colorsetwarning, colorseterror);
        _symbolic_base_color.setRgba32(colorsetbase);
        _symbolic_success_color.setRgba32(colorsetsuccess);
        _symbolic_warning_color.setRgba32(colorsetwarning);
        _symbolic_error_color.setRgba32(colorseterror);
        prefs->setUInt("/theme/" + themeiconname + "/symbolicBaseColor", colorsetbase);
        prefs->setUInt("/theme/" + themeiconname + "/symbolicSuccessColor", colorsetsuccess);
        prefs->setUInt("/theme/" + themeiconname + "/symbolicWarningColor", colorsetwarning);
        prefs->setUInt("/theme/" + themeiconname + "/symbolicErrorColor", colorseterror);
        if (prefs->getBool("/theme/symbolicDefaultColors", true)) {
            _symbolic_base_color.setSensitive(false);
            _symbolic_success_color.setSensitive(false);
            _symbolic_warning_color.setSensitive(false);
            _symbolic_error_color.setSensitive(false);
            /* _complementary_colors->get_style_context()->add_class("disabled"); */
        }
        changeIconsColors();
    } else {
        _symbolic_base_color.setSensitive(true);
        _symbolic_success_color.setSensitive(true);
        _symbolic_warning_color.setSensitive(true);
        _symbolic_error_color.setSensitive(true);
        /* _complementary_colors->get_style_context()->remove_class("disabled"); */
    }
}

void InkscapePreferences::resetIconsColorsWrapper() { resetIconsColors(false); }

void InkscapePreferences::changeIconsColors()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
    guint32 colorsetbase = prefs->getUInt("/theme/" + themeiconname + "/symbolicBaseColor", 0x2E3436ff);
    guint32 colorsetsuccess = prefs->getUInt("/theme/" + themeiconname + "/symbolicSuccessColor", 0x4AD589ff);
    guint32 colorsetwarning = prefs->getUInt("/theme/" + themeiconname + "/symbolicWarningColor", 0xF57900ff);
    guint32 colorseterror = prefs->getUInt("/theme/" + themeiconname + "/symbolicErrorColor", 0xCC0000ff);
    _symbolic_base_color.setRgba32(colorsetbase);
    _symbolic_success_color.setRgba32(colorsetsuccess);
    _symbolic_warning_color.setRgba32(colorsetwarning);
    _symbolic_error_color.setRgba32(colorseterror);
    auto const screen = Gdk::Screen::get_default();
    if (INKSCAPE.colorizeprovider) {
        Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.colorizeprovider);
    }
    Gtk::CssProvider::create();
    Glib::ustring css_str = "";
    if (prefs->getBool("/theme/symbolicIcons", false)) {
        css_str = INKSCAPE.get_symbolic_colors();
    }
    try {
        INKSCAPE.colorizeprovider->load_from_data(css_str);
    } catch (const Gtk::CssProviderError &ex) {
        g_critical("CSSProviderError::load_from_data(): failed to load '%s'\n(%s)", css_str.c_str(), ex.what().c_str());
    }
    Gtk::StyleContext::add_provider_for_screen(screen, INKSCAPE.colorizeprovider,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void InkscapePreferences::toggleSymbolic()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Gtk::Window *window = SP_ACTIVE_DESKTOP->getToplevel();
    if (prefs->getBool("/theme/symbolicIcons", false)) {
        if (window ) {
            window->get_style_context()->add_class("symbolic");
            window->get_style_context()->remove_class("regular");
        }
        _symbolic_base_colors.set_sensitive(true);
        Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
        if (prefs->getBool("/theme/symbolicDefaultColors", true) ||
            !prefs->getEntry("/theme/" + themeiconname + "/symbolicBaseColor").isValid()) {
            resetIconsColors();
        } else {
            changeIconsColors();
        }
    } else {
        if (window) {
            window->get_style_context()->add_class("regular");
            window->get_style_context()->remove_class("symbolic");
        }
        auto const screen = Gdk::Screen::get_default();
        if (INKSCAPE.colorizeprovider) {
            Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.colorizeprovider);
        }
        _symbolic_base_colors.set_sensitive(false);
    }
    INKSCAPE.signal_change_theme.emit();
}

bool InkscapePreferences::contrastChange(GdkEventButton *button_event)
{
    themeChange();
    return true;
}

void InkscapePreferences::themeChange()
{
    Gtk::Window *window = SP_ACTIVE_DESKTOP->getToplevel();
    if (window) {

        auto const screen = Gdk::Screen::get_default();
        if (INKSCAPE.contrastthemeprovider) {
            Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.contrastthemeprovider);
        }
        if (INKSCAPE.themeprovider) {
            Gtk::StyleContext::remove_provider_for_screen(screen, INKSCAPE.themeprovider);
        }
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        Glib::ustring current_theme = prefs->getString("/theme/gtkTheme");
        auto settings = Gtk::Settings::get_default();
        _dark_theme.get_parent()->set_no_show_all(false);
        if (dark_themes[current_theme]) {
            _dark_theme.get_parent()->show_all();
        } else {
            _dark_theme.get_parent()->hide();
        }

        settings->property_gtk_theme_name() = current_theme;
        bool dark = current_theme.find(":dark") != std::string::npos;
        if (!dark) {
            Glib::RefPtr<Gtk::StyleContext> stylecontext = window->get_style_context();
            Gdk::RGBA rgba;
            bool background_set = stylecontext->lookup_color("theme_bg_color", rgba);
            if (background_set && (0.299 * rgba.get_red() + 0.587 * rgba.get_green() + 0.114 * rgba.get_blue()) < 0.5) {
                dark = true;
            }
        }
        bool toggled = prefs->getBool("/theme/darkTheme", false) != dark;
        if (dark) {
            prefs->setBool("/theme/darkTheme", true);
            window->get_style_context()->add_class("dark");
            window->get_style_context()->remove_class("bright");
        } else {
            prefs->setBool("/theme/darkTheme", false);
            window->get_style_context()->add_class("bright");
            window->get_style_context()->remove_class("dark");
        }
        INKSCAPE.signal_change_theme.emit();
        INKSCAPE.add_gtk_css(true);
        resetIconsColors(toggled);
    }
}

void InkscapePreferences::preferDarkThemeChange()
{
    Gtk::Window *window = SP_ACTIVE_DESKTOP->getToplevel();
    if (window) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        bool dark_theme = prefs->getBool("/theme/preferDarkTheme", false);
        Glib::ustring current_theme = prefs->getString("/theme/gtkTheme");
        auto settings = Gtk::Settings::get_default();
        settings->property_gtk_application_prefer_dark_theme() = dark_theme;
        bool dark = current_theme.find(":dark") != std::string::npos;
        if (!dark) {
            Glib::RefPtr<Gtk::StyleContext> stylecontext = window->get_style_context();
            Gdk::RGBA rgba;
            bool background_set = stylecontext->lookup_color("theme_bg_color", rgba);
            if (background_set && (0.299 * rgba.get_red() + 0.587 * rgba.get_green() + 0.114 * rgba.get_blue()) < 0.5) {
                dark = true;
            }
        }
        bool toggled = prefs->getBool("/theme/darkTheme", false) != dark;
        if (dark) {
            prefs->setBool("/theme/darkTheme", true);
            window->get_style_context()->add_class("dark");
            window->get_style_context()->remove_class("bright");
        } else {
            prefs->setBool("/theme/darkTheme", false);
            window->get_style_context()->add_class("bright");
            window->get_style_context()->remove_class("dark");
        }
        INKSCAPE.signal_change_theme.emit();
        INKSCAPE.add_gtk_css(true);
        resetIconsColors(toggled);
    }
}

void InkscapePreferences::symbolicThemeCheck()
{
    using namespace Inkscape::IO::Resource;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
    bool symbolic = false;
    auto settings = Gtk::Settings::get_default();
    if (settings) {
        if (themeiconname != "") {
            settings->property_gtk_icon_theme_name() = themeiconname;
        }
    }
    if (prefs->getString("/theme/defaultIconTheme") != prefs->getString("/theme/iconTheme")) {
        auto folders = get_foldernames(ICONS, { "application" });
        for (auto &folder : folders) {
            auto path = folder;
            const size_t last_slash_idx = folder.find_last_of("\\/");
            if (std::string::npos != last_slash_idx) {
                folder.erase(0, last_slash_idx + 1);
            }
            if (folder == prefs->getString("/theme/iconTheme")) {
#ifdef _WIN32
                path += g_win32_locale_filename_from_utf8("/symbolic/actions");
#else
                path += "/symbolic/actions";
#endif
                std::vector<Glib::ustring> symbolic_icons = get_filenames(path, { ".svg" }, {});
                if (symbolic_icons.size() > 0) {
                    symbolic = true;
                    symbolic_icons.clear();
                }
            }
        }
    } else {
        symbolic = true;
    }
    if (_symbolic_icons.get_parent()) {
        if (!symbolic) {
            _symbolic_icons.set_active(false);
            _symbolic_icons.get_parent()->hide();
            _symbolic_base_colors.get_parent()->hide();
            _symbolic_base_color.get_parent()->get_parent()->hide();
            _symbolic_success_color.get_parent()->get_parent()->hide();
        } else {
            _symbolic_icons.get_parent()->show();
            _symbolic_base_colors.get_parent()->show();
            _symbolic_base_color.get_parent()->get_parent()->show();
            _symbolic_success_color.get_parent()->get_parent()->show();
        }
    }
    if (symbolic) {
        if (prefs->getBool("/theme/symbolicDefaultColors", true) ||
            !prefs->getEntry("/theme/" + themeiconname + "/symbolicBaseColor").isValid()) {
            resetIconsColors();
        } else {
            changeIconsColors();
        }
        guint32 colorsetbase = prefs->getUInt("/theme/" + themeiconname + "/symbolicBaseColor", 0x2E3436ff);
        guint32 colorsetsuccess = prefs->getUInt("/theme/" + themeiconname + "/symbolicSuccessColor", 0x4AD589ff);
        guint32 colorsetwarning = prefs->getUInt("/theme/" + themeiconname + "/symbolicWarningColor", 0xF57900ff);
        guint32 colorseterror = prefs->getUInt("/theme/" + themeiconname + "/symbolicErrorColor", 0xCC0000ff);
        _symbolic_base_color.init(_("Color for symbolic icons:"), "/theme/" + themeiconname + "/symbolicBaseColor",
                                  colorsetbase);
        _symbolic_success_color.init(_("Color for symbolic success icons:"),
                                     "/theme/" + themeiconname + "/symbolicSuccessColor", colorsetsuccess);
        _symbolic_warning_color.init(_("Color for symbolic warning icons:"),
                                     "/theme/" + themeiconname + "/symbolicWarningColor", colorsetwarning);
        _symbolic_error_color.init(_("Color for symbolic error icons:"),
                                   "/theme/" + themeiconname + "/symbolicErrorColor", colorseterror);
    }
}
/* void sp_mix_colors(cairo_t *ct, int pos, SPColor a, SPColor b)
{
    double arcEnd=2*M_PI;
    cairo_set_source_rgba(ct, 1, 1, 1, 1);
    cairo_arc(ct,pos,13,12,0,arcEnd);
    cairo_fill(ct);
    cairo_set_source_rgba(ct, a.v.c[0], a.v.c[1], a.v.c[2], 0.5);
    cairo_arc(ct,pos,13,12,0,arcEnd);
    cairo_fill(ct);
    cairo_set_source_rgba(ct, b.v.c[0], b.v.c[1], b.v.c[2], 0.5);
    cairo_arc(ct,pos,13,12,0,arcEnd);
    cairo_fill(ct);
}

Glib::RefPtr< Gdk::Pixbuf > sp_mix_colors()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
    guint32 colorsetsuccess = prefs->getUInt("/theme/" + themeiconname + "/symbolicSuccessColor", 0x4AD589ff);
    guint32 colorsetwarning = prefs->getUInt("/theme/" + themeiconname + "/symbolicWarningColor", 0xF57900ff);
    guint32 colorseterror = prefs->getUInt("/theme/" + themeiconname + "/symbolicErrorColor", 0xCC0000ff);
    SPColor success(colorsetsuccess);
    SPColor warning(colorsetwarning);
    SPColor error(colorseterror);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 82, 26);
    cairo_t *ct = cairo_create(s);
    // success + warning
    sp_mix_colors(ct, 13, success, warning);
    sp_mix_colors(ct, 41, success, error);
    sp_mix_colors(ct, 69, warning, error);
    cairo_destroy(ct);
    cairo_surface_flush(s);

    Cairo::RefPtr<Cairo::Surface> sref = Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(s));
    Glib::RefPtr<Gdk::Pixbuf> pixbuf =
        Gdk::Pixbuf::create(sref, 0, 0, 82, 26);
    cairo_surface_destroy(s);
    return pixbuf;
} */

void InkscapePreferences::changeIconsColor(guint32 /*color*/)
{
    changeIconsColors();
    /*     _complementary_colors->set(sp_mix_colors()); */
}

void InkscapePreferences::initPageUI()
{
    Gtk::TreeModel::iterator iter_ui = this->AddPage(_page_ui, _("Interface"), PREFS_PAGE_UI);

    Glib::ustring languages[] = {_("System default"),
        _("Albanian (sq)"), _("Arabic (ar)"), _("Armenian (hy)"), _("Assamese (as)"), _("Azerbaijani (az)"),
        _("Basque (eu)"), _("Belarusian (be)"), _("Bulgarian (bg)"), _("Bengali (bn)"), _("Bengali/Bangladesh (bn_BD)"), _("Bodo (brx)"), _("Breton (br)"),
        _("Catalan (ca)"), _("Valencian Catalan (ca@valencia)"), _("Chinese/China (zh_CN)"),  _("Chinese/Taiwan (zh_TW)"), _("Croatian (hr)"), _("Czech (cs)"),
        _("Danish (da)"), _("Dogri (doi)"), _("Dutch (nl)"), _("Dzongkha (dz)"),
        _("German (de)"), _("Greek (el)"),
        _("English (en)"), _("English/Australia (en_AU)"), _("English/Canada (en_CA)"), _("English/Great Britain (en_GB)"), _("Esperanto (eo)"), _("Estonian (et)"),
        _("Farsi (fa)"), _("Finnish (fi)"), _("French (fr)"),
        _("Galician (gl)"), _("Gujarati (gu)"),
        _("Hebrew (he)"), _("Hindi (hi)"), _("Hungarian (hu)"),
        _("Icelandic (is)"), _("Indonesian (id)"), _("Irish (ga)"), _("Italian (it)"),
        _("Japanese (ja)"),
        _("Kannada (kn)"), _("Kashmiri in Perso-Arabic script (ks@aran)"), _("Kashmiri in Devanagari script (ks@deva)"), _("Khmer (km)"), _("Kinyarwanda (rw)"), _("Konkani (kok)"), _("Konkani in Latin script (kok@latin)"), _("Korean (ko)"),
        _("Latvian (lv)"), _("Lithuanian (lt)"),
        _("Macedonian (mk)"), _("Maithili (mai)"), _("Malayalam (ml)"), _("Manipuri (mni)"), _("Manipuri in Bengali script (mni@beng)"), _("Marathi (mr)"), _("Mongolian (mn)"),
        _("Nepali (ne)"), _("Norwegian Bokmål (nb)"), _("Norwegian Nynorsk (nn)"),
        _("Odia (or)"),
        _("Panjabi (pa)"), _("Polish (pl)"), _("Portuguese (pt)"), _("Portuguese/Brazil (pt_BR)"),
        _("Romanian (ro)"), _("Russian (ru)"),
        _("Sanskrit (sa)"), _("Santali (sat)"), _("Santali in Devanagari script (sat@deva)"), _("Serbian (sr)"), _("Serbian in Latin script (sr@latin)"),
        _("Sindhi (sd)"), _("Sindhi in Devanagari script (sd@deva)"), _("Slovak (sk)"), _("Slovenian (sl)"),  _("Spanish (es)"), _("Spanish/Mexico (es_MX)"), _("Swedish (sv)"),
        _("Tamil (ta)"), _("Telugu (te)"), _("Thai (th)"), _("Turkish (tr)"),
        _("Ukrainian (uk)"), _("Urdu (ur)"),
        _("Vietnamese (vi)")};
    Glib::ustring langValues[] = {"",
        "sq", "ar", "hy", "as", "az",
        "eu", "be", "bg", "bn", "bn_BD", "brx", "br",
        "ca", "ca@valencia", "zh_CN", "zh_TW", "hr", "cs",
        "da", "doi", "nl", "dz",
        "de", "el",
        "en", "en_AU", "en_CA", "en_GB", "eo", "et",
        "fa", "fi", "fr",
        "gl", "gu",
        "he", "hi", "hu",
        "is", "id", "ga", "it",
        "ja",
        "kn", "ks@aran", "ks@deva", "km", "rw", "kok", "kok@latin", "ko",
        "lv", "lt",
        "mk", "mai", "ml", "mni", "mni@beng", "mr", "mn",
        "ne", "nb", "nn",
        "or",
        "pa", "pl", "pt", "pt_BR",
        "ro", "ru",
        "sa", "sat", "sat@deva", "sr", "sr@latin",
        "sd", "sd@deva", "sk", "sl", "es", "es_MX", "sv",
        "ta", "te", "th", "tr",
        "uk", "ur",
        "vi" };

    {
        // sorting languages according to translated name
        int i = 0;
        int j = 0;
        int n = sizeof( languages ) / sizeof( Glib::ustring );
        Glib::ustring key_language;
        Glib::ustring key_langValue;
        for ( j = 1 ; j < n ; j++ ) {
            key_language = languages[j];
            key_langValue = langValues[j];
            i = j-1;
            while ( i >= 0
                    && ( ( languages[i] > key_language
                         && langValues[i] != "" )
                       || key_langValue == "" ) )
            {
                languages[i+1] = languages[i];
                langValues[i+1] = langValues[i];
                i--;
            }
            languages[i+1] = key_language;
            langValues[i+1] = key_langValue;
        }
    }

    _ui_languages.init( "/ui/language", languages, langValues, G_N_ELEMENTS(languages), languages[0]);
    _page_ui.add_line( false, _("Language (requires restart):"), _ui_languages, "",
                              _("Set the language for menus and number formats"), true);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    _ui_colorsliders_top.init( _("Work-around color sliders not drawing"), "/options/workarounds/colorsontop", false);
    _page_ui.add_line( false, "", _ui_colorsliders_top, "",
                       _("When on, will attempt to work around bugs in certain GTK themes drawing color sliders"), true);


    _misc_recent.init("/options/maxrecentdocuments/value", 0.0, 1000.0, 1.0, 1.0, 1.0, true, false);

    Gtk::Button* reset_recent = Gtk::manage(new Gtk::Button(_("Clear list")));
    reset_recent->signal_clicked().connect(sigc::mem_fun(*this, &InkscapePreferences::on_reset_open_recent_clicked));

    _page_ui.add_line( false, _("Maximum documents in Open _Recent:"), _misc_recent, "",
                              _("Set the maximum length of the Open Recent list in the File menu, or clear the list"), false, reset_recent);

    _page_ui.add_group_header(_("_Zoom correction factor (in %)"));
    _page_ui.add_group_note(_("Adjust the slider until the length of the ruler on your screen matches its real length. This information is used when zooming to 1:1, 1:2, etc., to display objects in their true sizes"));
    _ui_zoom_correction.init(300, 30, 0.01, 500.0, 1.0, 10.0, 1.0);
    _page_ui.add_line( true, "", _ui_zoom_correction, "", "", true);

    _ui_realworldzoom.init( _("Show zoom percentage corrected by factor"), "/options/zoomcorrection/shown", true);
    _page_ui.add_line( false, "", _ui_realworldzoom, "", _("Zoom percentage can be either by the physical units or by pixels."));

    _ui_partialdynamic.init( _("Enable dynamic relayout for incomplete sections"), "/options/workarounds/dynamicnotdone", false);
    _page_ui.add_line( false, "", _ui_partialdynamic, "",
                       _("When on, will allow dynamic layout of components that are not completely finished being refactored"), true);

    /* show infobox */
    _show_filters_info_box.init( _("Show filter primitives infobox (requires restart)"), "/options/showfiltersinfobox/value", true);
    _page_ui.add_line(false, "", _show_filters_info_box, "",
                        _("Show icons and descriptions for the filter primitives available at the filter effects dialog"));

    {
        Glib::ustring dockbarstyleLabels[] = {_("Icons only"), _("Text only"), _("Icons and text")};
        int dockbarstyleValues[] = {0, 1, 2};

        /* dockbar style */
        _dockbar_style.init( "/options/dock/dockbarstyle", dockbarstyleLabels, dockbarstyleValues, G_N_ELEMENTS(dockbarstyleLabels), 0);
        _page_ui.add_line(false, _("Dockbar style (requires restart):"),  _dockbar_style, "",
                        _("Selects whether the vertical bars on the dockbar will show text labels, icons, or both"), false);

        Glib::ustring switcherstyleLabels[] = {_("Text only"), _("Icons only"), _("Icons and text")}; /* see bug #1098437   */
        int switcherstyleValues[] = {0, 1, 2};

        /* switcher style */
        _switcher_style.init( "/options/dock/switcherstyle", switcherstyleLabels, switcherstyleValues, G_N_ELEMENTS(switcherstyleLabels), 0);
        _page_ui.add_line(false, _("Switcher style (requires restart):"),  _switcher_style, "",
                        _("Selects whether the dockbar switcher will show text labels, icons, or both"), false);
    }

    _ui_yaxisdown.init( _("Origin at upper left with y-axis pointing down (requires restart)"), "/options/yaxisdown", true);
    _page_ui.add_line( false, "", _ui_yaxisdown, "",
                       _("When off, origin is at lower left corner and y-axis points up"), true);

    _ui_rotationlock.init(_("Lock canvas rotation by default"), "/options/rotationlock", false);
    _page_ui.add_line(false, "", _ui_rotationlock, "",
                       _("When enabled, common actions which normally rotate the canvas no longer do so by default"), true);

    _page_ui.add_group_header(_("Handle size"));
        _mouse_grabsize.init("/options/grabsize/value", 1, 15, 1, 2, 3, 0);
    _page_ui.add_line(false, "", _mouse_grabsize, "", _("Set the relative size of node handles"), true);
                         
    _page_ui.add_group_header(_("Cursor size"));
    _ui_cursorscaling.init(_("Enable cursor scaling"), "/options/cursorscaling", true);
    _page_ui.add_line(false, "", _ui_cursorscaling, "", _("When off, cursor scaling is disabled. Cursor scaling may be broken when fractional scaling is enabled."), true);

    // Theme
    _page_theme.add_group_header(_("Theme"));
    _dark_theme.init(_("Use dark theme"), "/theme/preferDarkTheme", false);
    Glib::ustring current_theme = prefs->getString("/theme/gtkTheme");
    Glib::ustring default_theme = prefs->getString("/theme/defaultTheme");
    Glib::ustring theme = "";
    {
        using namespace Inkscape::IO::Resource;
        gchar *path;
        gchar **builtin_themes;
        guint i, j;
        const gchar *const *dirs;

        /* Builtin themes */
        builtin_themes = g_resources_enumerate_children("/org/gtk/libgtk/theme", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
        for (i = 0; builtin_themes[i] != NULL; i++) {
            if (g_str_has_suffix(builtin_themes[i], "/")) {
                theme = builtin_themes[i];
                theme.resize(theme.size() - 1);
                Glib::ustring theme_path = "/org/gtk/libgtk/theme";
                theme_path += "/" + theme;
                gchar **builtin_themes_files =
                    g_resources_enumerate_children(theme_path.c_str(), G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
                bool has_prefer_dark = false;
                if (builtin_themes_files != NULL) {
                    for (j = 0; builtin_themes_files[j] != NULL; j++) {
                        Glib::ustring file = builtin_themes_files[j];
                        if (file == "gtk-dark.css") {
                            has_prefer_dark = true;
                        }
                    }
                }
                g_strfreev(builtin_themes_files);
                dark_themes[theme] = has_prefer_dark;
            }
        }
        g_strfreev(builtin_themes);

        path = g_build_filename(g_get_user_data_dir(), "themes", NULL);
        _inkscape_fill_gtk(path, dark_themes);
        g_free(path);

        path = g_build_filename(g_get_home_dir(), ".themes", NULL);
        _inkscape_fill_gtk(path, dark_themes);
        g_free(path);

        dirs = g_get_system_data_dirs();
        for (i = 0; dirs[i]; i++) {
            path = g_build_filename(dirs[i], "themes", NULL);
            _inkscape_fill_gtk(path, dark_themes);
            g_free(path);
        }

        std::vector<Glib::ustring> labels;
        std::vector<Glib::ustring> values;
        std::map<Glib::ustring, bool>::iterator it = dark_themes.begin();
        // Iterate over the map using Iterator till end.
        for (std::pair<std::string, int> element : dark_themes) {
            Glib::ustring theme = element.first;
            it++;
            if (theme == default_theme) {
                continue;
            }
            values.emplace_back(theme);
            labels.emplace_back(theme);
        }
        Glib::ustring default_theme_label = _("Use system theme");
        default_theme_label += " (" + default_theme + ")";
        labels.emplace_back(default_theme_label);
        values.push_back(default_theme);
        _gtk_theme.init("/theme/gtkTheme", labels, values, "Adwaita");
        _page_theme.add_line(false, _("Change GTK theme:"), _gtk_theme, "", "", false);
        _gtk_theme.signal_changed().connect(sigc::mem_fun(*this, &InkscapePreferences::themeChange));
    }
    _sys_user_themes_dir_copy.init(g_build_filename(g_get_user_data_dir(), "themes", NULL), _("Open themes folder"));
    _page_theme.add_line(true, _("User themes:"), _sys_user_themes_dir_copy, "", _("Location of the user’s themes"), true, Gtk::manage(new Gtk::Box()));
    _contrast_theme.init("/theme/contrast", 1, 10, 1, 2, 10, 1);
    Gtk::Widget *space = new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL);
    space->set_size_request(_sb_width / 3, -1);
    _page_theme.add_line(false, _("_Contrast:"), _contrast_theme, "",
                         _("Make background brighter or darker to reduce contrast"), true, space);
    _contrast_theme.signal_button_release_event().connect(sigc::mem_fun(*this, &InkscapePreferences::contrastChange));
    _page_theme.add_line(true, "", _dark_theme, "", _("Use dark theme"), true);

    if (dark_themes[current_theme]) {
        _dark_theme.get_parent()->set_no_show_all(false);
        _dark_theme.get_parent()->show_all();
    } else {
        _dark_theme.get_parent()->set_no_show_all(true);
        _dark_theme.get_parent()->hide();
    }
    _dark_theme.signal_clicked().connect(sigc::mem_fun(*this, &InkscapePreferences::preferDarkThemeChange));
    // Icons
    _page_theme.add_group_header(_("Icons"));
    {
        using namespace Inkscape::IO::Resource;
        auto folders = get_foldernames(ICONS, { "application" });
        std::vector<Glib::ustring> labels;
        std::vector<Glib::ustring> values;
        Glib::ustring default_icon_theme = prefs->getString("/theme/defaultIconTheme");
        for (auto &folder : folders) {
            // from https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path#8520871
            // Maybe we can link boost path utilities
            // Remove directory if present.
            // Do this before extension removal in case the directory has a period character.
            const size_t last_slash_idx = folder.find_last_of("\\/");
            if (std::string::npos != last_slash_idx) {
                folder.erase(0, last_slash_idx + 1);
            }
            if (folder == default_icon_theme || (default_icon_theme == "Adwaita" && folder == "hicolor")) {
                continue;
            }
            labels.push_back(folder);
            values.push_back(folder);
        }
        std::sort(labels.begin(), labels.end());
        std::sort(values.begin(), values.end());
        labels.erase(unique(labels.begin(), labels.end()), labels.end());
        values.erase(unique(values.begin(), values.end()), values.end());
        values.push_back(default_icon_theme);
        if (default_icon_theme == "Adwaita") {
            default_icon_theme = "hicolor";
        }
        Glib::ustring default_icon_label = _("Use system icons");
        default_icon_label += " (" + default_icon_theme + ")";
        labels.emplace_back(default_icon_label);

        _icon_theme.init("/theme/iconTheme", labels, values, "hicolor");
        _page_theme.add_line(false, _("Change icon theme:"), _icon_theme, "", "", false);
        _icon_theme.signal_changed().connect(sigc::mem_fun(*this, &InkscapePreferences::symbolicThemeCheck));
        _sys_user_icons_dir_copy.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::ICONS, ""),
                             _("Open icons folder"));
        _page_theme.add_line(true, _("User icons: "), _sys_user_icons_dir_copy, "", _("Location of the user’s icons"), true, Gtk::manage(new Gtk::Box()));
    }
    Glib::ustring themeiconname = prefs->getString("/theme/iconTheme");
    _symbolic_icons.init(_("Use symbolic icons"), "/theme/symbolicIcons", false);
    _symbolic_icons.signal_clicked().connect(sigc::mem_fun(*this, &InkscapePreferences::toggleSymbolic));
    _page_theme.add_line(true, "", _symbolic_icons, "", "", true);
    _symbolic_base_colors.init(_("Use default colors for icons"), "/theme/symbolicDefaultColors", true);
    _symbolic_base_colors.signal_clicked().connect(sigc::mem_fun(*this, &InkscapePreferences::resetIconsColorsWrapper));
    _page_theme.add_line(true, "", _symbolic_base_colors, "", "", true);
    _symbolic_base_color.init(_("Color for symbolic icons:"), "/theme/" + themeiconname + "/symbolicBaseColor",
                              0x2E3436ff);
    _symbolic_success_color.init(_("Color for symbolic success icons:"),
                                 "/theme/" + themeiconname + "/symbolicSuccessColor", 0x4AD589ff);
    _symbolic_warning_color.init(_("Color for symbolic warning icons:"),
                                 "/theme/" + themeiconname + "/symbolicWarningColor", 0xF57900ff);
    _symbolic_error_color.init(_("Color for symbolic error icons:"), "/theme/" + themeiconname + "/symbolicErrorColor",
                               0xCC0000ff);
    _symbolic_base_color.get_style_context()->add_class("system_base_color");
    _symbolic_success_color.get_style_context()->add_class("system_success_color");
    _symbolic_warning_color.get_style_context()->add_class("system_warning_color");
    _symbolic_error_color.get_style_context()->add_class("system_error_color");
    _symbolic_base_color.get_style_context()->add_class("symboliccolors");
    _symbolic_success_color.get_style_context()->add_class("symboliccolors");
    _symbolic_warning_color.get_style_context()->add_class("symboliccolors");
    _symbolic_error_color.get_style_context()->add_class("symboliccolors");
    _symbolic_base_color.connectChanged(sigc::mem_fun(this, &InkscapePreferences::changeIconsColor));
    _symbolic_warning_color.connectChanged(sigc::mem_fun(this, &InkscapePreferences::changeIconsColor));
    _symbolic_success_color.connectChanged(sigc::mem_fun(this, &InkscapePreferences::changeIconsColor));
    _symbolic_error_color.connectChanged(sigc::mem_fun(this, &InkscapePreferences::changeIconsColor));
    /* _complementary_colors = Gtk::manage(new Gtk::Image()); */
    Gtk::Box *icon_buttons = Gtk::manage(new Gtk::Box());
    icon_buttons->pack_start(_symbolic_base_color, true, true, 4);
    _page_theme.add_line(false, "", *icon_buttons, _("Icon color"),
                         _("Base color for icons"), false);
    Gtk::Box *icon_buttons_hight = Gtk::manage(new Gtk::Box());
    icon_buttons_hight->pack_start(_symbolic_success_color, true, true, 4);
    icon_buttons_hight->pack_start(_symbolic_warning_color, true, true, 4);
    icon_buttons_hight->pack_start(_symbolic_error_color, true, true, 4);
    /* icon_buttons_hight->pack_start(*_complementary_colors, true, true, 4); */
    _page_theme.add_line(false, "", *icon_buttons_hight, _("Highlights"),
                         _("Highlight colors supported by some symbolic icon themes"),
                         false);
    Gtk::Box *icon_buttons_def = Gtk::manage(new Gtk::Box());
    resetIconsColors();
    changeIconsColor(0xffffffff);
    _page_theme.add_line(false, "", *icon_buttons_def, "",
                         _("Reset theme colors for some symbolic icon themes"),
                         false);
    {
        Glib::ustring sizeLabels[] = { C_("Icon size", "Larger"), C_("Icon size", "Large"), C_("Icon size", "Small"),
                                       C_("Icon size", "Smaller") };
        int sizeValues[] = { 3, 2, 0, 1 };
        // "Larger" is 3 to not break existing preference files. Should fix in GTK3

        _misc_small_tools.init("/toolbox/tools/small", sizeLabels, sizeValues, G_N_ELEMENTS(sizeLabels), 0);
        _page_theme.add_line(false, _("Toolbox icon size:"), _misc_small_tools, _("(requires restart)"),
                             _("Set the size for the tool icons."), false);

        _misc_small_toolbar.init("/toolbox/small", sizeLabels, sizeValues, G_N_ELEMENTS(sizeLabels), 0);
        _page_theme.add_line(false, _("Control bar icon size:"), _misc_small_toolbar, _("(requires restart)"),
                             _("Set the size for the icons in tools' control bars."), false);

        _misc_small_secondary.init("/toolbox/secondary", sizeLabels, sizeValues, G_N_ELEMENTS(sizeLabels), 1);
        _page_theme.add_line(false, _("Secondary toolbar icon size:"), _misc_small_secondary, _("(requires restart)"),
                             _("Set the size for the icons in secondary toolbars."), false);
    }
    {
        Glib::ustring menu_icons_labels[] = {_("Yes"), _("No"), _("Theme decides")};
        int menu_icons_values[] = {1, -1, 0};
        _menu_icons.init("/theme/menuIcons", menu_icons_labels, menu_icons_values, G_N_ELEMENTS(menu_icons_labels), 0);
        _page_theme.add_line(false, _("Show icons in menus:"), _menu_icons, _("(requires restart)"),
                             _("You can either enable or disable all icons in menus. By default, the setting for the 'show-icons' attribute in the 'menus.xml' file determines whether to display icons in menus."), false);
    }

    this->AddPage(_page_theme, _("Themeing"), iter_ui, PREFS_PAGE_UI_THEME);
    symbolicThemeCheck();
    // Windows
    _win_save_geom.init ( _("Save and restore window geometry for each document"), "/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_FILE, true, nullptr);
    _win_save_geom_prefs.init ( _("Remember and use last window's geometry"), "/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_LAST, false, &_win_save_geom);
    _win_save_geom_off.init ( _("Don't save window geometry"), "/options/savewindowgeometry/value", PREFS_WINDOW_GEOMETRY_NONE, false, &_win_save_geom);

    _win_save_dialog_pos_on.init ( _("Save and restore dialogs status"), "/options/savedialogposition/value", PREFS_DIALOGS_STATE_SAVE, true, nullptr);
    _win_save_dialog_pos_off.init ( _("Don't save dialogs status"), "/options/savedialogposition/value", PREFS_DIALOGS_STATE_NONE, false, &_win_save_dialog_pos_on);

    _win_dockable.init ( _("Dockable"), "/options/dialogtype/value", PREFS_DIALOGS_BEHAVIOR_DOCKABLE, true, nullptr);
    _win_floating.init ( _("Floating"), "/options/dialogtype/value", PREFS_DIALOGS_BEHAVIOR_FLOATING, false, &_win_dockable);

    _win_native.init ( _("Native open/save dialogs"), "/options/desktopintegration/value", 1, true, nullptr);
    _win_gtk.init ( _("GTK open/save dialogs"), "/options/desktopintegration/value", 0, false, &_win_native);

    _win_hide_task.init ( _("Dialogs are hidden in taskbar"), "/options/dialogsskiptaskbar/value", true);
    _win_save_viewport.init ( _("Save and restore documents viewport"), "/options/savedocviewport/value", true);
    _win_zoom_resize.init ( _("Zoom when window is resized"), "/options/stickyzoom/value", false);
    _win_ontop_none.init ( C_("Dialog on top", "None"), "/options/transientpolicy/value", PREFS_DIALOGS_WINDOWS_NONE, false, nullptr);
    _win_ontop_normal.init ( _("Normal"), "/options/transientpolicy/value", PREFS_DIALOGS_WINDOWS_NORMAL, true, &_win_ontop_none);
    _win_ontop_agressive.init ( _("Aggressive"), "/options/transientpolicy/value", PREFS_DIALOGS_WINDOWS_AGGRESSIVE, false, &_win_ontop_none);

    _win_dialogs_labels_auto.init( _("Automatic"), "/options/notebooklabels/value", PREFS_NOTEBOOK_LABELS_AUTO, true, nullptr);
    _win_dialogs_labels_off.init( _("Off"), "/options/notebooklabels/value", PREFS_NOTBOOK_LABELS_OFF, false, &_win_dialogs_labels_auto);

    {
        Glib::ustring defaultSizeLabels[] = {C_("Window size", "Default"),
                                             C_("Window size", "Small"),
                                             C_("Window size", "Large"),
                                             C_("Window size", "Maximized")};
        int defaultSizeValues[] = {PREFS_WINDOW_SIZE_NATURAL,
                                   PREFS_WINDOW_SIZE_SMALL,
                                   PREFS_WINDOW_SIZE_LARGE,
                                   PREFS_WINDOW_SIZE_MAXIMIZED};

        _win_default_size.init( "/options/defaultwindowsize/value", defaultSizeLabels, defaultSizeValues, G_N_ELEMENTS(defaultSizeLabels), PREFS_WINDOW_SIZE_NATURAL);
        _page_windows.add_line( false, _("Default window size:"),  _win_default_size, "",
                           _("Set the default window size"), false);
    }

    _page_windows.add_group_header( _("Saving window geometry (size and position)"));
    _page_windows.add_line( true, "", _win_save_geom_off, "",
                            _("Let the window manager determine placement of all windows"));
    _page_windows.add_line( true, "", _win_save_geom_prefs, "",
                            _("Remember and use the last window's geometry (saves geometry to user preferences)"));
    _page_windows.add_line( true, "", _win_save_geom, "",
                            _("Save and restore window geometry for each document (saves geometry in the document)"));

    _page_windows.add_group_header( _("Saving dialogs' status"));
    _page_windows.add_line( true, "", _win_save_dialog_pos_off, "",
                            _("Don't save dialogs' status"));
    _page_windows.add_line( true, "", _win_save_dialog_pos_on, "",
                            _("Save and restore dialogs' status (the last open windows dialogs are saved when it closes)"));



    _page_windows.add_group_header( _("Dialog behavior (requires restart)"));
    _page_windows.add_line( true, "", _win_dockable, "",
                            _("Dockable"));
    _page_windows.add_line( true, "", _win_floating, "",
                            _("Floating"));
#ifdef _WIN32
    _page_windows.add_group_header( _("Desktop integration"));
    _page_windows.add_line( true, "", _win_native, "",
                            _("Use Windows like open and save dialogs"));
    _page_windows.add_line( true, "", _win_gtk, "",
                            _("Use GTK open and save dialogs "));
#endif

#ifndef _WIN32 // non-Win32 special code to enable transient dialogs
    _page_windows.add_group_header( _("Dialogs on top:"));

    _page_windows.add_line( true, "", _win_ontop_none, "",
                            _("Dialogs are treated as regular windows"));
    _page_windows.add_line( true, "", _win_ontop_normal, "",
                            _("Dialogs stay on top of document windows"));
    _page_windows.add_line( true, "", _win_ontop_agressive, "",
                            _("Same as Normal but may work better with some window managers"));
#endif

    _page_windows.add_group_header( _("Dialog labels behavior (requires restart)"));
    _page_windows.add_line( true, "", _win_dialogs_labels_auto, "", _("Dialogs' name labels will hide or show according to the space"));
    _page_windows.add_line( true, "", _win_dialogs_labels_off, "", _("Dialogs' name labels will allways be hidden"));

    _page_windows.add_group_header( _("Miscellaneous"));
#ifndef _WIN32 // FIXME: Temporary Win32 special code to enable transient dialogs
    _page_windows.add_line( true, "", _win_hide_task, "",
                            _("Whether dialog windows are to be hidden in the window manager taskbar"));
#endif
    _page_windows.add_line( true, "", _win_zoom_resize, "",
                            _("Zoom drawing when document window is resized, to keep the same area visible (this is the default which can be changed in any window using the button above the right scrollbar)"));
    _page_windows.add_line( true, "", _win_save_viewport, "",
                            _("Save documents viewport (zoom and panning position). Useful to turn off when sharing version controlled files."));
    this->AddPage(_page_windows, _("Windows"), iter_ui, PREFS_PAGE_UI_WINDOWS);

    // Grids
    _page_grids.add_group_header( _("Line color when zooming out"));

    _grids_no_emphasize_on_zoom.init( _("Minor grid line color"), "/options/grids/no_emphasize_when_zoomedout", 1, true, nullptr);
    _page_grids.add_line( true, "", _grids_no_emphasize_on_zoom, "", _("The gridlines will be shown in minor grid line color"), false);
    _grids_emphasize_on_zoom.init( _("Major grid line color"), "/options/grids/no_emphasize_when_zoomedout", 0, false, &_grids_no_emphasize_on_zoom);
    _page_grids.add_line( true, "", _grids_emphasize_on_zoom, "", _("The gridlines will be shown in major grid line color"), false);

    _page_grids.add_group_header( _("Default grid settings"));

    _page_grids.add_line( true, "", _grids_notebook, "", "", false);
    _grids_notebook.append_page(_grids_xy,     CanvasGrid::getName( GRID_RECTANGULAR ));
    _grids_notebook.append_page(_grids_axonom, CanvasGrid::getName( GRID_AXONOMETRIC ));
        _grids_xy_units.init("/options/grids/xy/units");
        _grids_xy.add_line( false, _("Grid units:"), _grids_xy_units, "", "", false);
        _grids_xy_origin_x.init("/options/grids/xy/origin_x", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_xy_origin_y.init("/options/grids/xy/origin_y", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_xy_origin_x.set_digits(5);
        _grids_xy_origin_y.set_digits(5);
        _grids_xy.add_line( false, _("Origin X:"), _grids_xy_origin_x, "", _("X coordinate of grid origin"), false);
        _grids_xy.add_line( false, _("Origin Y:"), _grids_xy_origin_y, "", _("Y coordinate of grid origin"), false);
        _grids_xy_spacing_x.init("/options/grids/xy/spacing_x", -10000.0, 10000.0, 0.1, 1.0, 1.0, false, false);
        _grids_xy_spacing_y.init("/options/grids/xy/spacing_y", -10000.0, 10000.0, 0.1, 1.0, 1.0, false, false);
        _grids_xy_spacing_x.set_digits(5);
        _grids_xy_spacing_y.set_digits(5);
        _grids_xy.add_line( false, _("Spacing X:"), _grids_xy_spacing_x, "", _("Distance between vertical grid lines"), false);
        _grids_xy.add_line( false, _("Spacing Y:"), _grids_xy_spacing_y, "", _("Distance between horizontal grid lines"), false);

        _grids_xy_color.init(_("Minor grid line color:"), "/options/grids/xy/color", GRID_DEFAULT_COLOR);
        _grids_xy.add_line( false, _("Minor grid line color:"), _grids_xy_color, "", _("Color used for normal grid lines"), false);
        _grids_xy_empcolor.init(_("Major grid line color:"), "/options/grids/xy/empcolor", GRID_DEFAULT_EMPCOLOR);
        _grids_xy.add_line( false, _("Major grid line color:"), _grids_xy_empcolor, "", _("Color used for major (highlighted) grid lines"), false);
        _grids_xy_empspacing.init("/options/grids/xy/empspacing", 1.0, 1000.0, 1.0, 5.0, 5.0, true, false);
        _grids_xy.add_line( false, _("Major grid line every:"), _grids_xy_empspacing, "", "", false);
        _grids_xy_dotted.init( _("Show dots instead of lines"), "/options/grids/xy/dotted", false);
        _grids_xy.add_line( false, "", _grids_xy_dotted, "", _("If set, display dots at gridpoints instead of gridlines"), false);

    // CanvasAxonomGrid properties:
        _grids_axonom_units.init("/options/grids/axonom/units");
        _grids_axonom.add_line( false, _("Grid units:"), _grids_axonom_units, "", "", false);
        _grids_axonom_origin_x.init("/options/grids/axonom/origin_x", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_axonom_origin_y.init("/options/grids/axonom/origin_y", -10000.0, 10000.0, 0.1, 1.0, 0.0, false, false);
        _grids_axonom_origin_x.set_digits(5);
        _grids_axonom_origin_y.set_digits(5);
        _grids_axonom.add_line( false, _("Origin X:"), _grids_axonom_origin_x, "", _("X coordinate of grid origin"), false);
        _grids_axonom.add_line( false, _("Origin Y:"), _grids_axonom_origin_y, "", _("Y coordinate of grid origin"), false);
        _grids_axonom_spacing_y.init("/options/grids/axonom/spacing_y", -10000.0, 10000.0, 0.1, 1.0, 1.0, false, false);
        _grids_axonom_spacing_y.set_digits(5);
        _grids_axonom.add_line( false, _("Spacing Y:"), _grids_axonom_spacing_y, "", _("Base length of z-axis"), false);
        _grids_axonom_angle_x.init("/options/grids/axonom/angle_x", -360.0, 360.0, 1.0, 10.0, 30.0, false, false);
        _grids_axonom_angle_z.init("/options/grids/axonom/angle_z", -360.0, 360.0, 1.0, 10.0, 30.0, false, false);
        _grids_axonom.add_line( false, _("Angle X:"), _grids_axonom_angle_x, "", _("Angle of x-axis"), false);
        _grids_axonom.add_line( false, _("Angle Z:"), _grids_axonom_angle_z, "", _("Angle of z-axis"), false);
        _grids_axonom_color.init(_("Minor grid line color:"), "/options/grids/axonom/color", GRID_DEFAULT_COLOR);
        _grids_axonom.add_line( false, _("Minor grid line color:"), _grids_axonom_color, "", _("Color used for normal grid lines"), false);
        _grids_axonom_empcolor.init(_("Major grid line color:"), "/options/grids/axonom/empcolor", GRID_DEFAULT_EMPCOLOR);
        _grids_axonom.add_line( false, _("Major grid line color:"), _grids_axonom_empcolor, "", _("Color used for major (highlighted) grid lines"), false);
        _grids_axonom_empspacing.init("/options/grids/axonom/empspacing", 1.0, 1000.0, 1.0, 5.0, 5.0, true, false);
        _grids_axonom.add_line( false, _("Major grid line every:"), _grids_axonom_empspacing, "", "", false);

    this->AddPage(_page_grids, _("Grids"), iter_ui, PREFS_PAGE_UI_GRIDS);

    initKeyboardShortcuts(iter_ui);
}

static void profileComboChanged( Gtk::ComboBoxText* combo )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int rowNum = combo->get_active_row_number();
    if ( rowNum < 1 ) {
        prefs->setString("/options/displayprofile/uri", "");
    } else {
        Glib::ustring active = combo->get_active_text();

        Glib::ustring path = CMSSystem::getPathForProfile(active);
        if ( !path.empty() ) {
            prefs->setString("/options/displayprofile/uri", path);
        }
    }
}

static void proofComboChanged( Gtk::ComboBoxText* combo )
{
    Glib::ustring active = combo->get_active_text();
    Glib::ustring path = CMSSystem::getPathForProfile(active);

    if ( !path.empty() ) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString("/options/softproof/uri", path);
    }
}

static void gamutColorChanged( Gtk::ColorButton* btn ) {
    auto rgba = btn->get_rgba();
    auto r = rgba.get_red_u();
    auto g = rgba.get_green_u();
    auto b = rgba.get_blue_u();

    gchar* tmp = g_strdup_printf("#%02x%02x%02x", (r >> 8), (g >> 8), (b >> 8) );

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/options/softproof/gamutcolor", tmp);
    g_free(tmp);
}

void InkscapePreferences::initPageIO()
{
    Gtk::TreeModel::iterator iter_io = this->AddPage(_page_io, _("Input/Output"), PREFS_PAGE_IO);

    _save_use_current_dir.init( _("Use current directory for \"Save As ...\""), "/dialogs/save_as/use_current_dir", true);
    _page_io.add_line( false, "", _save_use_current_dir, "",
                         _("When this option is on, the \"Save as...\" and \"Save a Copy...\" dialogs will always open in the directory where the currently open document is; when it's off, each will open in the directory where you last saved a file using it"), true);

    _misc_default_metadata.init( _("Add default metadata to new documents"), "/metadata/addToNewFile", false);
    _page_io.add_line( false, "", _misc_default_metadata, "",
                           _("Add default metadata to new documents. Default metadata can be set from Document Properties->Metadata."), true);

    // Input devices options
    _mouse_sens.init ( "/options/cursortolerance/value", 0.0, 30.0, 1.0, 1.0, 8.0, true, false);
    _page_mouse.add_line( false, _("_Grab sensitivity:"), _mouse_sens, _("pixels (requires restart)"),
                           _("How close on the screen you need to be to an object to be able to grab it with mouse (in screen pixels)"), false);
    _mouse_thres.init ( "/options/dragtolerance/value", 0.0, 20.0, 1.0, 1.0, 4.0, true, false);
    _page_mouse.add_line( false, _("_Click/drag threshold:"), _mouse_thres, _("pixels"),
                           _("Maximum mouse drag (in screen pixels) which is considered a click, not a drag"), false);

    _mouse_use_ext_input.init( _("Use pressure-sensitive tablet (requires restart)"), "/options/useextinput/value", true);
    _page_mouse.add_line(false, "",_mouse_use_ext_input, "",
                        _("Use the capabilities of a tablet or other pressure-sensitive device. Disable this only if you have problems with the tablet (you can still use it as a mouse)"));

    _mouse_switch_on_ext_input.init( _("Switch tool based on tablet device (requires restart)"), "/options/switchonextinput/value", false);
    _page_mouse.add_line(false, "",_mouse_switch_on_ext_input, "",
                        _("Change tool as different devices are used on the tablet (pen, eraser, mouse)"));
    this->AddPage(_page_mouse, _("Input devices"), iter_io, PREFS_PAGE_IO_MOUSE);

    // SVG output options
    _svgoutput_usenamedcolors.init( _("Use named colors"), "/options/svgoutput/usenamedcolors", false);
    _page_svgoutput.add_line( false, "", _svgoutput_usenamedcolors, "", _("If set, write the CSS name of the color when available (e.g. 'red' or 'magenta') instead of the numeric value"), false);

    _page_svgoutput.add_group_header( _("XML formatting"));

    _svgoutput_inlineattrs.init( _("Inline attributes"), "/options/svgoutput/inlineattrs", false);
    _page_svgoutput.add_line( true, "", _svgoutput_inlineattrs, "", _("Put attributes on the same line as the element tag"), false);

    _svgoutput_indent.init("/options/svgoutput/indent", 0.0, 1000.0, 1.0, 2.0, 2.0, true, false);
    _page_svgoutput.add_line( true, _("_Indent, spaces:"), _svgoutput_indent, "", _("The number of spaces to use for indenting nested elements; set to 0 for no indentation"), false);

    _page_svgoutput.add_group_header( _("Path data"));

    int const numPathstringFormat = 3;
    Glib::ustring pathstringFormatLabels[numPathstringFormat] = {_("Absolute"), _("Relative"), _("Optimized")};
    int pathstringFormatValues[numPathstringFormat] = {0, 1, 2};

    _svgoutput_pathformat.init("/options/svgoutput/pathstring_format", pathstringFormatLabels, pathstringFormatValues, numPathstringFormat, 2);
    _page_svgoutput.add_line( true, _("Path string format:"), _svgoutput_pathformat, "", _("Path data should be written: only with absolute coordinates, only with relative coordinates, or optimized for string length (mixed absolute and relative coordinates)"), false);

    _svgoutput_forcerepeatcommands.init( _("Force repeat commands"), "/options/svgoutput/forcerepeatcommands", false);
    _page_svgoutput.add_line( true, "", _svgoutput_forcerepeatcommands, "", _("Force repeating of the same path command (for example, 'L 1,2 L 3,4' instead of 'L 1,2 3,4')"), false);

    _page_svgoutput.add_group_header( _("Numbers"));

    _svgoutput_numericprecision.init("/options/svgoutput/numericprecision", 1.0, 16.0, 1.0, 2.0, 8.0, true, false);
    _page_svgoutput.add_line( true, _("_Numeric precision:"), _svgoutput_numericprecision, "", _("Significant figures of the values written to the SVG file"), false);

    _svgoutput_minimumexponent.init("/options/svgoutput/minimumexponent", -32.0, -1, 1.0, 2.0, -8.0, true, false);
    _page_svgoutput.add_line( true, _("Minimum _exponent:"), _svgoutput_minimumexponent, "", _("The smallest number written to SVG is 10 to the power of this exponent; anything smaller is written as zero"), false);

    /* Code to add controls for attribute checking options */

    /* Add incorrect style properties options  */
    _page_svgoutput.add_group_header( _("Improper Attributes Actions"));

    _svgoutput_attrwarn.init( _("Print warnings"), "/options/svgoutput/incorrect_attributes_warn", true);
    _page_svgoutput.add_line( true, "", _svgoutput_attrwarn, "", _("Print warning if invalid or non-useful attributes found. Database files located in inkscape_data_dir/attributes."), false);
    _svgoutput_attrremove.init( _("Remove attributes"), "/options/svgoutput/incorrect_attributes_remove", false);
    _page_svgoutput.add_line( true, "", _svgoutput_attrremove, "", _("Delete invalid or non-useful attributes from element tag"), false);

    /* Add incorrect style properties options  */
    _page_svgoutput.add_group_header( _("Inappropriate Style Properties Actions"));

    _svgoutput_stylepropwarn.init( _("Print warnings"), "/options/svgoutput/incorrect_style_properties_warn", true);
    _page_svgoutput.add_line( true, "", _svgoutput_stylepropwarn, "", _("Print warning if inappropriate style properties found (i.e. 'font-family' set on a <rect>). Database files located in inkscape_data_dir/attributes."), false);
    _svgoutput_stylepropremove.init( _("Remove style properties"), "/options/svgoutput/incorrect_style_properties_remove", false);
    _page_svgoutput.add_line( true, "", _svgoutput_stylepropremove, "", _("Delete inappropriate style properties"), false);

    /* Add default or inherited style properties options  */
    _page_svgoutput.add_group_header( _("Non-useful Style Properties Actions"));

    _svgoutput_styledefaultswarn.init( _("Print warnings"), "/options/svgoutput/style_defaults_warn", true);
    _page_svgoutput.add_line( true, "", _svgoutput_styledefaultswarn, "", _("Print warning if redundant style properties found (i.e. if a property has the default value and a different value is not inherited or if value is the same as would be inherited). Database files located in inkscape_data_dir/attributes."), false);
    _svgoutput_styledefaultsremove.init( _("Remove style properties"), "/options/svgoutput/style_defaults_remove", false);
    _page_svgoutput.add_line( true, "", _svgoutput_styledefaultsremove, "", _("Delete redundant style properties"), false);

    _page_svgoutput.add_group_header( _("Check Attributes and Style Properties on"));

    _svgoutput_check_reading.init( _("Reading"), "/options/svgoutput/check_on_reading", false);
    _page_svgoutput.add_line( true, "", _svgoutput_check_reading, "", _("Check attributes and style properties on reading in SVG files (including those internal to Inkscape which will slow down startup)"), false);
    _svgoutput_check_editing.init( _("Editing"), "/options/svgoutput/check_on_editing", false);
    _page_svgoutput.add_line( true, "", _svgoutput_check_editing, "", _("Check attributes and style properties while editing SVG files (may slow down Inkscape, mostly useful for debugging)"), false);
    _svgoutput_check_writing.init( _("Writing"), "/options/svgoutput/check_on_writing", true);
    _page_svgoutput.add_line( true, "", _svgoutput_check_writing, "", _("Check attributes and style properties on writing out SVG files"), false);

    this->AddPage(_page_svgoutput, _("SVG output"), iter_io, PREFS_PAGE_IO_SVGOUTPUT);

    // SVG Export Options ==========================================

    // SVG 2 Fallbacks
    _page_svgexport.add_group_header( _("SVG 2"));
    _svgexport_insert_text_fallback.init( _("Insert SVG 1.1 fallback in text."),                                     "/options/svgexport/text_insertfallback",    true );
    _svgexport_insert_mesh_polyfill.init( _("Insert JavaScript code for mesh gradients."),                            "/options/svgexport/mesh_insertpolyfill",    true );
    _svgexport_insert_hatch_polyfill.init( _("Insert JavaScript code for SVG2 hatches."),                       "/options/svgexport/hatch_insertpolyfill",   true );

    _page_svgexport.add_line( false, "", _svgexport_insert_text_fallback,  "", _("Adds fallback options for non-SVG 2 renderers."), false);
    _page_svgexport.add_line( false, "", _svgexport_insert_mesh_polyfill,  "", _("Adds a JavaScript polyfill for rendering meshes in web browsers."), false);
    _page_svgexport.add_line( false, "", _svgexport_insert_hatch_polyfill,  "", _("Adds a JavaScript polyfill for rendering hatches in web browsers."), false);

    // SVG Export Options (SVG 2 -> SVG 1)
    _page_svgexport.add_group_header( _("SVG 2 to SVG 1.1"));

    _svgexport_remove_marker_auto_start_reverse.init( _("Use correct marker direction in SVG 1.1 renderers."),               "/options/svgexport/marker_autostartreverse", false);
    _svgexport_remove_marker_context_paint.init(      _("Use correct marker colors in SVG 1.1 renderers."), "/options/svgexport/marker_contextpaint",     false);

    _page_svgexport.add_line( false, "", _svgexport_remove_marker_auto_start_reverse, "", _("SVG 2 allows markers to automatically be reversed at the start of a path with 'auto_start_reverse'. This adds a rotated duplicate of the marker's definition."), false);
    _page_svgexport.add_line( false, "", _svgexport_remove_marker_context_paint,      "", _("SVG 2 allows markers to automatically match the stroke color by using 'context_paint' or 'context_fill'. This adjusts the markers own colors."),           false);

    this->AddPage(_page_svgexport, _("SVG export"), iter_io, PREFS_PAGE_IO_SVGEXPORT);


    // CMS options
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int const numIntents = 4;
    /* TRANSLATORS: see http://www.newsandtech.com/issues/2004/03-04/pt/03-04_rendering.htm */
    Glib::ustring intentLabels[numIntents] = {_("Perceptual"), _("Relative Colorimetric"), _("Saturation"), _("Absolute Colorimetric")};
    int intentValues[numIntents] = {0, 1, 2, 3};

    _page_cms.add_group_header( _("Display adjustment"));

    Glib::ustring tmpStr;
    for (auto &profile: ColorProfile::getBaseProfileDirs()) {
        gchar* part = g_strdup_printf( "\n%s", profile.filename.c_str() );
        tmpStr += part;
        g_free(part);
    }

    gchar* profileTip = g_strdup_printf(_("The ICC profile to use to calibrate display output.\nSearched directories:%s"), tmpStr.c_str());
    _page_cms.add_line( true, _("Display profile:"), _cms_display_profile, "",
                        profileTip, false);
    g_free(profileTip);
    profileTip = nullptr;

    _cms_from_display.init( _("Retrieve profile from display"), "/options/displayprofile/from_display", false);
    _page_cms.add_line( true, "", _cms_from_display, "",
#ifdef GDK_WINDOWING_X11
                        _("Retrieve profiles from those attached to displays via XICC"), false);
#else
                        _("Retrieve profiles from those attached to displays"), false);
#endif // GDK_WINDOWING_X11


    _cms_intent.init("/options/displayprofile/intent", intentLabels, intentValues, numIntents, 0);
    _page_cms.add_line( true, _("Display rendering intent:"), _cms_intent, "",
                        _("The rendering intent to use to calibrate display output"), false);

    _page_cms.add_group_header( _("Proofing"));

    _cms_softproof.init( _("Simulate output on screen"), "/options/softproof/enable", false);
    _page_cms.add_line( true, "", _cms_softproof, "",
                        _("Simulates output of target device"), false);

    _cms_gamutwarn.init( _("Mark out of gamut colors"), "/options/softproof/gamutwarn", false);
    _page_cms.add_line( true, "", _cms_gamutwarn, "",
                        _("Highlights colors that are out of gamut for the target device"), false);

    Glib::ustring colorStr = prefs->getString("/options/softproof/gamutcolor");

    Gdk::RGBA tmpColor( colorStr.empty() ? "#00ff00" : colorStr);
    _cms_gamutcolor.set_rgba( tmpColor );

    _page_cms.add_line( true, _("Out of gamut warning color:"), _cms_gamutcolor, "",
                        _("Selects the color used for out of gamut warning"), false);

    _page_cms.add_line( true, _("Device profile:"), _cms_proof_profile, "",
                        _("The ICC profile to use to simulate device output"), false);

    _cms_proof_intent.init("/options/softproof/intent", intentLabels, intentValues, numIntents, 0);
    _page_cms.add_line( true, _("Device rendering intent:"), _cms_proof_intent, "",
                        _("The rendering intent to use to calibrate device output"), false);

    _cms_proof_blackpoint.init( _("Black point compensation"), "/options/softproof/bpc", false);
    _page_cms.add_line( true, "", _cms_proof_blackpoint, "",
                        _("Enables black point compensation"), false);

    _cms_proof_preserveblack.init( _("Preserve black"), "/options/softproof/preserveblack", false);

#if !defined(cmsFLAGS_PRESERVEBLACK)
    _cms_proof_preserveblack.set_sensitive( false );
#endif // !defined(cmsFLAGS_PRESERVEBLACK)


    {
        std::vector<Glib::ustring> names = ::Inkscape::CMSSystem::getDisplayNames();
        Glib::ustring current = prefs->getString( "/options/displayprofile/uri" );

        gint index = 0;
        _cms_display_profile.append(_("<none>"));
        index++;
        for (auto & name : names) {
            _cms_display_profile.append( name );
            Glib::ustring path = CMSSystem::getPathForProfile(name);
            if ( !path.empty() && path == current ) {
                _cms_display_profile.set_active(index);
            }
            index++;
        }
        if ( current.empty() ) {
            _cms_display_profile.set_active(0);
        }

        names = ::Inkscape::CMSSystem::getSoftproofNames();
        current = prefs->getString("/options/softproof/uri");
        index = 0;
        for (auto & name : names) {
            _cms_proof_profile.append( name );
            Glib::ustring path = CMSSystem::getPathForProfile(name);
            if ( !path.empty() && path == current ) {
                _cms_proof_profile.set_active(index);
            }
            index++;
        }
    }

    _cms_gamutcolor.signal_color_set().connect( sigc::bind( sigc::ptr_fun(gamutColorChanged), &_cms_gamutcolor) );

    _cms_display_profile.signal_changed().connect( sigc::bind( sigc::ptr_fun(profileComboChanged), &_cms_display_profile) );
    _cms_proof_profile.signal_changed().connect( sigc::bind( sigc::ptr_fun(proofComboChanged), &_cms_proof_profile) );

    this->AddPage(_page_cms, _("Color management"), iter_io, PREFS_PAGE_IO_CMS);

    // Autosave options
    _save_autosave_enable.init( _("Enable autosave (requires restart)"), "/options/autosave/enable", true);
    _page_autosave.add_line(false, "", _save_autosave_enable, "", _("Automatically save the current document(s) at a given interval, thus minimizing loss in case of a crash"), false);
    _save_autosave_path.init("/options/autosave/path", true);
    if (prefs->getString("/options/autosave/path").empty()) {
        // Show the default fallback "tmp dir" if autosave path is not set.
        _save_autosave_path.set_text(Glib::build_filename(Glib::get_user_cache_dir(), "inkscape"));
    }
    _page_autosave.add_line(false, C_("Filesystem", "Autosave _directory:"), _save_autosave_path, "", _("The directory where autosaves will be written. This should be an absolute path (starts with / on UNIX or a drive letter such as C: on Windows). "), false);
    _save_autosave_interval.init("/options/autosave/interval", 1.0, 10800.0, 1.0, 10.0, 10.0, true, false);
    _page_autosave.add_line(false, _("_Interval (in minutes):"), _save_autosave_interval, "", _("Interval (in minutes) at which document will be autosaved"), false);
    _save_autosave_max.init("/options/autosave/max", 1.0, 100.0, 1.0, 10.0, 10.0, true, false);
    _page_autosave.add_line(false, _("_Maximum number of autosaves:"), _save_autosave_max, "", _("Maximum number of autosaved files; use this to limit the storage space used"), false);

    /* When changing the interval or enabling/disabling the autosave function,
     * update our running configuration
     *
     * FIXME!
     * AutoSave::restart() should be called AFTER the values have been changed
     * (which cannot be guaranteed from here) - use a PrefObserver somewhere.
     */
    _save_autosave_enable.signal_toggled(  ).connect( sigc::ptr_fun(Inkscape::AutoSave::restart), true);
    _save_autosave_interval.signal_changed().connect( sigc::ptr_fun(Inkscape::AutoSave::restart), true);

    this->AddPage(_page_autosave, _("Autosave"), iter_io, PREFS_PAGE_IO_AUTOSAVE);

    // No Result
    _page_notfound.add_group_header("No matches were found, try another search!");
}

void InkscapePreferences::initPageBehavior()
{
    Gtk::TreeModel::iterator iter_behavior = this->AddPage(_page_behavior, _("Behavior"), PREFS_PAGE_BEHAVIOR);

    _misc_simpl.init("/options/simplifythreshold/value", 0.0001, 1.0, 0.0001, 0.0010, 0.0010, false, false);
    _page_behavior.add_line( false, _("_Simplification threshold:"), _misc_simpl, "",
                           _("How strong is the Node tool's Simplify command by default. If you invoke this command several times in quick succession, it will act more and more aggressively; invoking it again after a pause restores the default threshold."), false);

    _markers_color_stock.init ( _("Color stock markers the same color as object"), "/options/markers/colorStockMarkers", true);
    _markers_color_custom.init ( _("Color custom markers the same color as object"), "/options/markers/colorCustomMarkers", false);
    _markers_color_update.init ( _("Update marker color when object color changes"), "/options/markers/colorUpdateMarkers", true);

    // Selecting options
    _sel_all.init ( _("Select in all layers"), "/options/kbselection/inlayer", PREFS_SELECTION_ALL, false, nullptr);
    _sel_current.init ( _("Select only within current layer"), "/options/kbselection/inlayer", PREFS_SELECTION_LAYER, true, &_sel_all);
    _sel_recursive.init ( _("Select in current layer and sublayers"), "/options/kbselection/inlayer", PREFS_SELECTION_LAYER_RECURSIVE, false, &_sel_all);
    _sel_hidden.init ( _("Ignore hidden objects and layers"), "/options/kbselection/onlyvisible", true);
    _sel_locked.init ( _("Ignore locked objects and layers"), "/options/kbselection/onlysensitive", true);
    _sel_layer_deselects.init ( _("Deselect upon layer change"), "/options/selection/layerdeselect", true);

    _page_select.add_line( false, "", _sel_layer_deselects, "",
                           _("Uncheck this to be able to keep the current objects selected when the current layer changes"));

    _page_select.add_group_header( _("Ctrl+A, Tab, Shift+Tab"));
    _page_select.add_line( true, "", _sel_all, "",
                           _("Make keyboard selection commands work on objects in all layers"));
    _page_select.add_line( true, "", _sel_current, "",
                           _("Make keyboard selection commands work on objects in current layer only"));
    _page_select.add_line( true, "", _sel_recursive, "",
                           _("Make keyboard selection commands work on objects in current layer and all its sublayers"));
    _page_select.add_line( true, "", _sel_hidden, "",
                           _("Uncheck this to be able to select objects that are hidden (either by themselves or by being in a hidden layer)"));
    _page_select.add_line( true, "", _sel_locked, "",
                           _("Uncheck this to be able to select objects that are locked (either by themselves or by being in a locked layer)"));

    _sel_cycle.init ( _("Wrap when cycling objects in z-order"), "/options/selection/cycleWrap", true);

    _page_select.add_group_header( _("Alt+Scroll Wheel"));
    _page_select.add_line( true, "", _sel_cycle, "",
                           _("Wrap around at start and end when cycling objects in z-order"));

    this->AddPage(_page_select, _("Selecting"), iter_behavior, PREFS_PAGE_BEHAVIOR_SELECTING);

    // Transforms options
    _trans_scale_stroke.init ( _("Scale stroke width"), "/options/transform/stroke", true);
    _trans_scale_corner.init ( _("Scale rounded corners in rectangles"), "/options/transform/rectcorners", false);
    _trans_gradient.init ( _("Transform gradients"), "/options/transform/gradient", true);
    _trans_pattern.init ( _("Transform patterns"), "/options/transform/pattern", false);
    _trans_optimized.init ( _("Optimized"), "/options/preservetransform/value", 0, true, nullptr);
    _trans_preserved.init ( _("Preserved"), "/options/preservetransform/value", 1, false, &_trans_optimized);

    _page_transforms.add_line( false, "", _trans_scale_stroke, "",
                               _("When scaling objects, scale the stroke width by the same proportion"));
    _page_transforms.add_line( false, "", _trans_scale_corner, "",
                               _("When scaling rectangles, scale the radii of rounded corners"));
    _page_transforms.add_line( false, "", _trans_gradient, "",
                               _("Move gradients (in fill or stroke) along with the objects"));
    _page_transforms.add_line( false, "", _trans_pattern, "",
                               _("Move patterns (in fill or stroke) along with the objects"));
    _page_transforms.add_group_header( _("Store transformation"));
    _page_transforms.add_line( true, "", _trans_optimized, "",
                               _("If possible, apply transformation to objects without adding a transform= attribute"));
    _page_transforms.add_line( true, "", _trans_preserved, "",
                               _("Always store transformation as a transform= attribute on objects"));

    this->AddPage(_page_transforms, _("Transforms"), iter_behavior, PREFS_PAGE_BEHAVIOR_TRANSFORMS);

    _dash_scale.init(_("Scale dashes with stroke"), "/options/dash/scale", true);
    _page_dashes.add_line(false, "", _dash_scale, "", _("When changing stroke width, scale dash array"));

    this->AddPage(_page_dashes, _("Dashes"), iter_behavior, PREFS_PAGE_BEHAVIOR_DASHES);

    // Scrolling options
    _scroll_wheel.init ( "/options/wheelscroll/value", 0.0, 1000.0, 1.0, 1.0, 40.0, true, false);
    _page_scrolling.add_line( false, _("Mouse _wheel scrolls by:"), _scroll_wheel, _("pixels"),
                           _("One mouse wheel notch scrolls by this distance in screen pixels (horizontally with Shift)"), false);
    _page_scrolling.add_group_header( _("Ctrl+arrows"));
    _scroll_arrow_px.init ( "/options/keyscroll/value", 0.0, 1000.0, 1.0, 1.0, 10.0, true, false);
    _page_scrolling.add_line( true, _("Sc_roll by:"), _scroll_arrow_px, _("pixels"),
                           _("Pressing Ctrl+arrow key scrolls by this distance (in screen pixels)"), false);
    _scroll_arrow_acc.init ( "/options/scrollingacceleration/value", 0.0, 5.0, 0.01, 1.0, 0.35, false, false);
    _page_scrolling.add_line( true, _("_Acceleration:"), _scroll_arrow_acc, "",
                           _("Pressing and holding Ctrl+arrow will gradually speed up scrolling (0 for no acceleration)"), false);
    _page_scrolling.add_group_header( _("Autoscrolling"));
    _scroll_auto_speed.init ( "/options/autoscrollspeed/value", 0.0, 5.0, 0.01, 1.0, 0.7, false, false);
    _page_scrolling.add_line( true, _("_Speed:"), _scroll_auto_speed, "",
                           _("How fast the canvas autoscrolls when you drag beyond canvas edge (0 to turn autoscroll off)"), false);
    _scroll_auto_thres.init ( "/options/autoscrolldistance/value", -600.0, 600.0, 1.0, 1.0, -10.0, true, false);
    _page_scrolling.add_line( true, _("_Threshold:"), _scroll_auto_thres, _("pixels"),
                           _("How far (in screen pixels) you need to be from the canvas edge to trigger autoscroll; positive is outside the canvas, negative is within the canvas"), false);
    _scroll_space.init ( _("Mouse move pans when Space is pressed"), "/options/spacebarpans/value", true);
    _page_scrolling.add_line( true, "", _scroll_space, "",
                            _("When on, pressing and holding Space and dragging pans canvas"));
    this->AddPage(_page_scrolling, _("Scrolling"), iter_behavior, PREFS_PAGE_BEHAVIOR_SCROLLING);

    // Snapping options
    _page_snapping.add_group_header( _("Snap defaults"));

    _snap_default.init( _("Enable snapping in new documents"), "/options/snapdefault/value", true);
    _page_snapping.add_line( true, "", _snap_default, "",
                             _("Initial state of snapping in new documents and SVG files that are opened with Inkscape for the first time. Snap status is subsequently saved per-document."));

    _page_snapping.add_group_header( _("Snap indicator"));

    _snap_indicator.init( _("Enable snap indicator"), "/options/snapindicator/value", true);
    _page_snapping.add_line( true, "", _snap_indicator, "",
                             _("After snapping, a symbol is drawn at the point that has snapped"));

    _snap_indicator.changed_signal.connect( sigc::mem_fun(_snap_persistence, &Gtk::Widget::set_sensitive) );

    _snap_persistence.init("/options/snapindicatorpersistence/value", 0.1, 10, 0.1, 1, 2, 1);
    _page_snapping.add_line( true, _("Snap indicator persistence (in seconds):"), _snap_persistence, "",
                             _("Controls how long the snap indicator message will be shown, before it disappears"), true);

    _page_snapping.add_group_header( _("What should snap"));

    _snap_closest_only.init( _("Only snap the node closest to the pointer"), "/options/snapclosestonly/value", false);
    _page_snapping.add_line( true, "", _snap_closest_only, "",
                             _("Only try to snap the node that is initially closest to the mouse pointer"));

    _snap_weight.init("/options/snapweight/value", 0, 1, 0.1, 0.2, 0.5, 1);
    _page_snapping.add_line( true, _("_Weight factor:"), _snap_weight, "",
                             _("When multiple snap solutions are found, then Inkscape can either prefer the closest transformation (when set to 0), or prefer the node that was initially the closest to the pointer (when set to 1)"), true);

    _snap_mouse_pointer.init( _("Snap the mouse pointer when dragging a constrained knot"), "/options/snapmousepointer/value", false);
    _page_snapping.add_line( true, "", _snap_mouse_pointer, "",
                             _("When dragging a knot along a constraint line, then snap the position of the mouse pointer instead of snapping the projection of the knot onto the constraint line"));

    _page_snapping.add_group_header( _("Delayed snap"));

    _snap_delay.init("/options/snapdelay/value", 0, 1, 0.1, 0.2, 0, 1);
    _page_snapping.add_line( true, _("Delay (in seconds):"), _snap_delay, "",
                             _("Postpone snapping as long as the mouse is moving, and then wait an additional fraction of a second. This additional delay is specified here. When set to zero or to a very small number, snapping will be immediate."), true);

    this->AddPage(_page_snapping, _("Snapping"), iter_behavior, PREFS_PAGE_BEHAVIOR_SNAPPING);

    // Steps options
    _steps_arrow.init ( "/options/nudgedistance/value", 0.0, 1000.0, 0.01, 2.0, UNIT_TYPE_LINEAR, "px");
    //nudgedistance is limited to 1000 in select-context.cpp: use the same limit here
    _page_steps.add_line( false, _("_Arrow keys move by:"), _steps_arrow, "",
                          _("Pressing an arrow key moves selected object(s) or node(s) by this distance"), false);
    _steps_scale.init ( "/options/defaultscale/value", 0.0, 1000.0, 0.01, 2.0, UNIT_TYPE_LINEAR, "px");
    //defaultscale is limited to 1000 in select-context.cpp: use the same limit here
    _page_steps.add_line( false, _("&gt; and &lt; _scale by:"), _steps_scale, "",
                          _("Pressing > or < scales selection up or down by this increment"), false);
    _steps_inset.init ( "/options/defaultoffsetwidth/value", 0.0, 3000.0, 0.01, 2.0, UNIT_TYPE_LINEAR, "px");
    _page_steps.add_line( false, _("_Inset/Outset by:"), _steps_inset, "",
                          _("Inset and Outset commands displace the path by this distance"), false);
    _steps_compass.init ( _("Compass-like display of angles"), "/options/compassangledisplay/value", true);
    _page_steps.add_line( false, "", _steps_compass, "",
                            _("When on, angles are displayed with 0 at north, 0 to 360 range, positive clockwise; otherwise with 0 at east, -180 to 180 range, positive counterclockwise"));
    int const num_items = 18;
    Glib::ustring labels[num_items] = {"90", "60", "45", "36", "30", "22.5", "18", "15", "12", "10", "7.5", "6", "5", "3", "2", "1", "0.5", C_("Rotation angle", "None")};
    int values[num_items] = {2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 24, 30, 36, 60, 90, 180, 360, 0};
    _steps_rot_snap.set_size_request(_sb_width);
    _steps_rot_snap.init("/options/rotationsnapsperpi/value", labels, values, num_items, 12);
    _page_steps.add_line( false, _("_Rotation snaps every:"), _steps_rot_snap, _("degrees"),
                           _("Rotating with Ctrl pressed snaps every that much degrees; also, pressing [ or ] rotates by this amount"), false);
    _steps_rot_relative.init ( _("Relative snapping of guideline angles"), "/options/relativeguiderotationsnap/value", false);
    _page_steps.add_line( false, "", _steps_rot_relative, "",
                            _("When on, the snap angles when rotating a guideline will be relative to the original angle"));
    _steps_zoom.init ( "/options/zoomincrement/value", 101.0, 500.0, 1.0, 1.0, M_SQRT2, true, true);
    _page_steps.add_line( false, _("_Zoom in/out by:"), _steps_zoom, _("%"),
                          _("Zoom tool click, +/- keys, and middle click zoom in and out by this multiplier"), false);
    _middle_mouse_zoom.init ( _("Zoom with middle mouse click"), "/options/middlemousezoom/value", true);
    _page_steps.add_line( true, "", _middle_mouse_zoom, "",
                            _("When on, clicking the middle mouse button (usually the mouse wheel) makes zoom."));
    _steps_rotate.init ( "/options/rotateincrement/value", 1, 90, 1.0, 5.0, 15, false, false);
    _page_steps.add_line( false, _("_Rotate canvas by:"), _steps_rotate, _("degrees"),
                          _("Rotate canvas clockwise and counter-clockwise by this amount."), false);
    this->AddPage(_page_steps, _("Steps"), iter_behavior, PREFS_PAGE_BEHAVIOR_STEPS);

    // Clones options
    _clone_option_parallel.init ( _("Move in parallel"), "/options/clonecompensation/value",
                                  SP_CLONE_COMPENSATION_PARALLEL, true, nullptr);
    _clone_option_stay.init ( _("Stay unmoved"), "/options/clonecompensation/value",
                                  SP_CLONE_COMPENSATION_UNMOVED, false, &_clone_option_parallel);
    _clone_option_transform.init ( _("Move according to transform"), "/options/clonecompensation/value",
                                  SP_CLONE_COMPENSATION_NONE, false, &_clone_option_parallel);
    _clone_option_unlink.init ( _("Are unlinked"), "/options/cloneorphans/value",
                                  SP_CLONE_ORPHANS_UNLINK, true, nullptr);
    _clone_option_delete.init ( _("Are deleted"), "/options/cloneorphans/value",
                                  SP_CLONE_ORPHANS_DELETE, false, &_clone_option_unlink);

    _page_clones.add_group_header( _("Moving original: clones and linked offsets"));
    _page_clones.add_line(true, "", _clone_option_parallel, "",
                           _("Clones are translated by the same vector as their original"));
    _page_clones.add_line(true, "", _clone_option_stay, "",
                           _("Clones preserve their positions when their original is moved"));
    _page_clones.add_line(true, "", _clone_option_transform, "",
                           _("Each clone moves according to the value of its transform= attribute; for example, a rotated clone will move in a different direction than its original"));
    _page_clones.add_group_header( _("Deleting original: clones"));
    _page_clones.add_line(true, "", _clone_option_unlink, "",
                           _("Orphaned clones are converted to regular objects"));
    _page_clones.add_line(true, "", _clone_option_delete, "",
                           _("Orphaned clones are deleted along with their original"));

    _page_clones.add_group_header( _("Duplicating original+clones/linked offset"));

    _clone_relink_on_duplicate.init ( _("Relink duplicated clones"), "/options/relinkclonesonduplicate/value", false);
    _page_clones.add_line(true, "", _clone_relink_on_duplicate, "",
                        _("When duplicating a selection containing both a clone and its original (possibly in groups), relink the duplicated clone to the duplicated original instead of the old original"));

    _page_clones.add_group_header( _("Unlinking clones"));
    _clone_to_curves.init ( _("Path operations unlink clones"), "/options/pathoperationsunlink/value", true);
    _page_clones.add_line(true, "", _clone_to_curves, "",
                        _("The following path operations will unlink clones: Stroke to path, Object to path, Boolean operations, Combine, Break apart"));

    //TRANSLATORS: Heading for the Inkscape Preferences "Clones" Page
    this->AddPage(_page_clones, _("Clones"), iter_behavior, PREFS_PAGE_BEHAVIOR_CLONES);

    // Clip paths and masks options
    _mask_mask_on_top.init ( _("When applying, use the topmost selected object as clippath/mask"), "/options/maskobject/topmost", true);
    _page_mask.add_line(false, "", _mask_mask_on_top, "",
                        _("Uncheck this to use the bottom selected object as the clipping path or mask"));
    _mask_mask_remove.init ( _("Remove clippath/mask object after applying"), "/options/maskobject/remove", true);
    _page_mask.add_line(false, "", _mask_mask_remove, "",
                        _("After applying, remove the object used as the clipping path or mask from the drawing"));

    _page_mask.add_group_header( _("Before applying"));

    _mask_grouping_none.init( _("Do not group clipped/masked objects"), "/options/maskobject/grouping", PREFS_MASKOBJECT_GROUPING_NONE, true, nullptr);
    _mask_grouping_separate.init( _("Put every clipped/masked object in its own group"), "/options/maskobject/grouping", PREFS_MASKOBJECT_GROUPING_SEPARATE, false, &_mask_grouping_none);
    _mask_grouping_all.init( _("Put all clipped/masked objects into one group"), "/options/maskobject/grouping", PREFS_MASKOBJECT_GROUPING_ALL, false, &_mask_grouping_none);

    _page_mask.add_line(true, "", _mask_grouping_none, "",
                        _("Apply clippath/mask to every object"));

    _page_mask.add_line(true, "", _mask_grouping_separate, "",
                        _("Apply clippath/mask to groups containing single object"));

    _page_mask.add_line(true, "", _mask_grouping_all, "",
                        _("Apply clippath/mask to group containing all objects"));

    _page_mask.add_group_header( _("After releasing"));

    _mask_ungrouping.init ( _("Ungroup automatically created groups"), "/options/maskobject/ungrouping", true);
    _page_mask.add_line(true, "", _mask_ungrouping, "",
                        _("Ungroup groups created when setting clip/mask"));

    this->AddPage(_page_mask, _("Clippaths and masks"), iter_behavior, PREFS_PAGE_BEHAVIOR_MASKS);


    _page_markers.add_group_header( _("Stroke Style Markers"));
    _page_markers.add_line( true, "", _markers_color_stock, "",
                           _("Stroke color same as object, fill color either object fill color or marker fill color"));
    _page_markers.add_line( true, "", _markers_color_custom, "",
                           _("Stroke color same as object, fill color either object fill color or marker fill color"));
    _page_markers.add_line( true, "", _markers_color_update, "",
                           _("Update marker color when object color changes"));

    this->AddPage(_page_markers, _("Markers"), iter_behavior, PREFS_PAGE_BEHAVIOR_MARKERS);


    _page_cleanup.add_group_header( _("Document cleanup"));
    _cleanup_swatches.init ( _("Remove unused swatches when doing a document cleanup"), "/options/cleanupswatches/value", false); // text label
    _page_cleanup.add_line( true, "", _cleanup_swatches, "",
                           _("Remove unused swatches when doing a document cleanup")); // tooltip
    this->AddPage(_page_cleanup, _("Cleanup"), iter_behavior, PREFS_PAGE_BEHAVIOR_CLEANUP);
}

void InkscapePreferences::initPageRendering()
{

    /* threaded blur */ //related comments/widgets/functions should be renamed and option should be moved elsewhere when inkscape is fully multi-threaded
    _filter_multi_threaded.init("/options/threading/numthreads", 1.0, 8.0, 1.0, 2.0, 4.0, true, false);
    _page_rendering.add_line( false, _("Number of _Threads:"), _filter_multi_threaded, _("(requires restart)"),
                           _("Configure number of processors/threads to use when rendering filters"), false);

    // rendering cache
    _rendering_cache_size.init("/options/renderingcache/size", 0.0, 4096.0, 1.0, 32.0, 64.0, true, false);
    _page_rendering.add_line( false, _("Rendering _cache size:"), _rendering_cache_size, C_("mebibyte (2^20 bytes) abbreviation","MiB"), _("Set the amount of memory per document which can be used to store rendered parts of the drawing for later reuse; set to zero to disable caching"), false);

    // rendering tile multiplier
    _rendering_tile_multiplier.init("/options/rendering/tile-multiplier", 1.0, 512.0, 1.0, 16.0, 16.0, true, false);
    _page_rendering.add_line( false, _("Rendering tile multiplier:"), _rendering_tile_multiplier, "",
                              _("On modern hardware, increasing this value (default is 16) can help to get a better performance when there are large areas with filtered objects (this includes blur and blend modes) in your drawing. Decrease the value to make zooming and panning in relevant areas faster on low-end hardware in drawings with few or no filters."), false);
    // rendering xray radius
    _rendering_xray_radius.init("/options/rendering/xray-radius", 1.0, 1500.0, 1.0, 100.0, 100.0, true, false);
    _page_rendering.add_line( false, _("X-ray radius:"), _rendering_xray_radius, "",
                             _("Radius of the circular area around the mouse cursor in X-ray mode"), false);

    // rendering outline overlay opcaity
    _rendering_outline_overlay_opacity.init("/options/rendering/outline-overlay-opacity", 1.0, 100.0, 1.0, 5.0, 50.0, true, false);
    _page_rendering.add_line( false, _("Outline overlay opacity:"), _rendering_outline_overlay_opacity, _("%"),
                             _("Opacity of the color on outline overlay render mode"), false);

    {
        // if these GTK constants ever change, consider adding a compatibility shim to SPCanvas::addIdle()
        static_assert(G_PRIORITY_HIGH_IDLE    == 100, "G_PRIORITY_HIGH_IDLE must be 100 to match preferences.xml");
        static_assert(G_PRIORITY_DEFAULT_IDLE == 200, "G_PRIORITY_DEFAULT_IDLE must be 200 to match preferences.xml");

        Glib::ustring redrawPriorityLabels[] = {_("Responsive"), _("Conservative")};
        int redrawPriorityValues[] = {G_PRIORITY_HIGH_IDLE, G_PRIORITY_DEFAULT_IDLE};

        // redraw priority
        _rendering_redraw_priority.init("/options/redrawpriority/value", redrawPriorityLabels, redrawPriorityValues, G_N_ELEMENTS(redrawPriorityLabels), 0);
        _page_rendering.add_line(false, _("Redraw while editing:"), _rendering_redraw_priority, "",
                                        _("Set how quickly the canvas display is updated while editing objects"), false);
    }

    /* blur quality */
    _blur_quality_best.init ( _("Best quality (slowest)"), "/options/blurquality/value",
                                  BLUR_QUALITY_BEST, false, nullptr);
    _blur_quality_better.init ( _("Better quality (slower)"), "/options/blurquality/value",
                                  BLUR_QUALITY_BETTER, false, &_blur_quality_best);
    _blur_quality_normal.init ( _("Average quality"), "/options/blurquality/value",
                                  BLUR_QUALITY_NORMAL, true, &_blur_quality_best);
    _blur_quality_worse.init ( _("Lower quality (faster)"), "/options/blurquality/value",
                                  BLUR_QUALITY_WORSE, false, &_blur_quality_best);
    _blur_quality_worst.init ( _("Lowest quality (fastest)"), "/options/blurquality/value",
                                  BLUR_QUALITY_WORST, false, &_blur_quality_best);

    _page_rendering.add_group_header( _("Gaussian blur quality for display"));
    _page_rendering.add_line( true, "", _blur_quality_best, "",
                           _("Best quality, but display may be very slow at high zooms (bitmap export always uses best quality)"));
    _page_rendering.add_line( true, "", _blur_quality_better, "",
                           _("Better quality, but slower display"));
    _page_rendering.add_line( true, "", _blur_quality_normal, "",
                           _("Average quality, acceptable display speed"));
    _page_rendering.add_line( true, "", _blur_quality_worse, "",
                           _("Lower quality (some artifacts), but display is faster"));
    _page_rendering.add_line( true, "", _blur_quality_worst, "",
                           _("Lowest quality (considerable artifacts), but display is fastest"));

    /* filter quality */
    _filter_quality_best.init ( _("Best quality (slowest)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_BEST, false, nullptr);
    _filter_quality_better.init ( _("Better quality (slower)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_BETTER, false, &_filter_quality_best);
    _filter_quality_normal.init ( _("Average quality"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_NORMAL, true, &_filter_quality_best);
    _filter_quality_worse.init ( _("Lower quality (faster)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_WORSE, false, &_filter_quality_best);
    _filter_quality_worst.init ( _("Lowest quality (fastest)"), "/options/filterquality/value",
                                  Inkscape::Filters::FILTER_QUALITY_WORST, false, &_filter_quality_best);

    _page_rendering.add_group_header( _("Filter effects quality for display"));
    _page_rendering.add_line( true, "", _filter_quality_best, "",
                           _("Best quality, but display may be very slow at high zooms (bitmap export always uses best quality)"));
    _page_rendering.add_line( true, "", _filter_quality_better, "",
                           _("Better quality, but slower display"));
    _page_rendering.add_line( true, "", _filter_quality_normal, "",
                           _("Average quality, acceptable display speed"));
    _page_rendering.add_line( true, "", _filter_quality_worse, "",
                           _("Lower quality (some artifacts), but display is faster"));
    _page_rendering.add_line( true, "", _filter_quality_worst, "",
                           _("Lowest quality (considerable artifacts), but display is fastest"));

    this->AddPage(_page_rendering, _("Rendering"), PREFS_PAGE_RENDERING);
}

void InkscapePreferences::initPageBitmaps()
{
    /* Note: /options/bitmapoversample removed with Cairo renderer */
    _page_bitmaps.add_group_header( _("Edit"));
    _misc_bitmap_autoreload.init(_("Automatically reload images"), "/options/bitmapautoreload/value", true);
    _page_bitmaps.add_line( false, "", _misc_bitmap_autoreload, "",
                           _("Automatically reload linked images when file is changed on disk"));
    _misc_bitmap_editor.init("/options/bitmapeditor/value", true);
    _page_bitmaps.add_line( false, _("_Bitmap editor:"), _misc_bitmap_editor, "", "", true);
    _misc_svg_editor.init("/options/svgeditor/value", true);
    _page_bitmaps.add_line( false, _("_SVG editor:"), _misc_svg_editor, "", "", true);

    _page_bitmaps.add_group_header( _("Export"));
    _importexport_export_res.init("/dialogs/export/defaultxdpi/value", 0.0, 6000.0, 1.0, 1.0, Inkscape::Util::Quantity::convert(1, "in", "px"), true, false);
    _page_bitmaps.add_line( false, _("Default export _resolution:"), _importexport_export_res, _("dpi"),
                            _("Default image resolution (in dots per inch) in the Export dialog"), false);
    _page_bitmaps.add_group_header( _("Create"));
    _bitmap_copy_res.init("/options/createbitmap/resolution", 1.0, 6000.0, 1.0, 1.0, Inkscape::Util::Quantity::convert(1, "in", "px"), true, false);
    _page_bitmaps.add_line( false, _("Resolution for Create Bitmap _Copy:"), _bitmap_copy_res, _("dpi"),
                            _("Resolution used by the Create Bitmap Copy command"), false);

    _page_bitmaps.add_group_header( _("Import"));
    _bitmap_ask.init(_("Ask about linking and scaling when importing bitmap images"), "/dialogs/import/ask", true);
    _page_bitmaps.add_line( true, "", _bitmap_ask, "",
                           _("Pop-up linking and scaling dialog when importing bitmap image."));
    _svg_ask.init(_("Ask about linking and scaling when importing SVG images"), "/dialogs/import/ask_svg", true);
    _page_bitmaps.add_line( true, "", _svg_ask, "",
                           _("Pop-up linking and scaling dialog when importing SVG image."));

    _svgoutput_usesodipodiabsref.init(_("Store absolute file path for linked images"),
                                      "/options/svgoutput/usesodipodiabsref", false);
    _page_bitmaps.add_line(
        true, "", _svgoutput_usesodipodiabsref, "",
        _("By default image links are stored as relative paths whenever possible. If this option is enabled Inkscape "
          "will additionally add an absolute path ('sodipody:absref' attribute) to the image, which is used as a "
          "fall-back for locating the linked image, for example if the SVG document is moved on disk. Note that this "
          "will expose your directory structure which can include personal information like your username."),
        false);

    {
        Glib::ustring labels[] = {_("Embed"), _("Link")};
        Glib::ustring values[] = {"embed", "link"};
        _bitmap_link.init("/dialogs/import/link", labels, values, G_N_ELEMENTS(values), "link");
        _page_bitmaps.add_line( false, _("Bitmap import/open mode:"), _bitmap_link, "", "", false);
    }

    {
        Glib::ustring labels[] = {_("Include"), _("Embed"), _("Link")};
        Glib::ustring values[] = {"include", "embed", "link"};
        _svg_link.init("/dialogs/import/import_mode_svg", labels, values, G_N_ELEMENTS(values), "include");
        _page_bitmaps.add_line( false, _("SVG import mode:"), _svg_link, "", "", false);
    }

    {
        Glib::ustring labels[] = {_("None (auto)"), _("Smooth (optimizeQuality)"), _("Blocky (optimizeSpeed)") };
        Glib::ustring values[] = {"auto", "optimizeQuality", "optimizeSpeed"};
        _bitmap_scale.init("/dialogs/import/scale", labels, values, G_N_ELEMENTS(values), "scale");
        _page_bitmaps.add_line( false, _("Image scale (image-rendering):"), _bitmap_scale, "", "", false);
    }

    /* Note: /dialogs/import/quality removed use of in r12542 */
    _importexport_import_res.init("/dialogs/import/defaultxdpi/value", 0.0, 6000.0, 1.0, 1.0, Inkscape::Util::Quantity::convert(1, "in", "px"), true, false);
    _page_bitmaps.add_line( false, _("Default _import resolution:"), _importexport_import_res, _("dpi"),
                            _("Default import resolution (in dots per inch) for bitmap and SVG import"), false);
    _importexport_import_res_override.init(_("Override file resolution"), "/dialogs/import/forcexdpi", false);
    _page_bitmaps.add_line( false, "", _importexport_import_res_override, "",
                            _("Use default bitmap resolution in favor of information from file"));

    _page_bitmaps.add_group_header( _("Render"));
    // rendering outlines for pixmap image tags
    _rendering_image_outline.init( _("Images in Outline Mode"), "/options/rendering/imageinoutlinemode", false);
    _page_bitmaps.add_line(false, "", _rendering_image_outline, "", _("When active will render images while in outline mode instead of a red box with an x. This is useful for manual tracing."));

    this->AddPage(_page_bitmaps, _("Imported Images"), PREFS_PAGE_BITMAPS);
}

void InkscapePreferences::initKeyboardShortcuts(Gtk::TreeModel::iterator iter_ui)
{
    // ------- Shortcut file --------
    auto labels_and_names = Inkscape::Shortcuts::get_file_names();
    _kb_filelist.init( "/options/kbshortcuts/shortcutfile", labels_and_names, labels_and_names[0].second);

    Glib::ustring tooltip(_("Select a file of predefined shortcuts and modifiers to use. Any customizations you create will be added separately to "));
    tooltip += Glib::ustring(IO::Resource::get_path(IO::Resource::USER, IO::Resource::KEYS, "default.xml"));

    _page_keyshortcuts.add_line( false, _("Keyboard file:"), _kb_filelist, "", tooltip.c_str(), false);


    // ---------- Tree --------
    _kb_store = Gtk::TreeStore::create( _kb_columns );
    _kb_store->set_sort_column ( GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING ); // only sort in onKBListKeyboardShortcuts()

    _kb_filter = Gtk::TreeModelFilter::create(_kb_store);
    _kb_filter->set_visible_func (sigc::mem_fun(*this, &InkscapePreferences::onKBSearchFilter));

    _kb_shortcut_renderer.property_editable() = true;

    _kb_tree.set_model(_kb_filter);
    _kb_tree.append_column(_("Name"), _kb_columns.name);
    _kb_tree.append_column(_("Shortcut"), _kb_shortcut_renderer);
    _kb_tree.append_column(_("Description"), _kb_columns.description);
    _kb_tree.append_column(_("ID"), _kb_columns.id);

    _kb_tree.set_expander_column(*_kb_tree.get_column(0));

    // Name
    _kb_tree.get_column(0)->set_resizable(true);
    _kb_tree.get_column(0)->set_clickable(true);
    _kb_tree.get_column(0)->set_fixed_width (200);

    // Shortcut
    _kb_tree.get_column(1)->set_resizable(true);
    _kb_tree.get_column(1)->set_clickable(true);
    _kb_tree.get_column(1)->set_fixed_width (150);
    //_kb_tree.get_column(1)->add_attribute(_kb_shortcut_renderer.property_text(), _kb_columns.shortcut);
    _kb_tree.get_column(1)->set_cell_data_func(_kb_shortcut_renderer, sigc::ptr_fun(InkscapePreferences::onKBShortcutRenderer));

    // Description
    auto desc_renderer = dynamic_cast<Gtk::CellRendererText*>(_kb_tree.get_column_cell_renderer(2));
    desc_renderer->property_wrap_mode() = Pango::WRAP_WORD;
    desc_renderer->property_wrap_width() = 600;
    _kb_tree.get_column(2)->set_resizable(true);
    _kb_tree.get_column(2)->set_clickable(true);
    _kb_tree.get_column(2)->set_expand(true);

    // ID
    _kb_tree.get_column(3)->set_resizable(true);
    _kb_tree.get_column(3)->set_clickable(true);

    _kb_shortcut_renderer.signal_accel_edited().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBTreeEdited) );
    _kb_shortcut_renderer.signal_accel_cleared().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBTreeCleared) );

    _kb_notebook.append_page(_kb_page_shortcuts, _("Shortcuts"));
    Gtk::ScrolledWindow* shortcut_scroller = new Gtk::ScrolledWindow();
    shortcut_scroller->add(_kb_tree);
    shortcut_scroller->set_hexpand();
    shortcut_scroller->set_vexpand();
    // -------- Search --------
    _kb_search.init("/options/kbshortcuts/value", true);
    _kb_page_shortcuts.add_line( false, _("Search:"), _kb_search, "", "", true);
    _kb_page_shortcuts.attach(*shortcut_scroller, 0, 3, 2, 1);

    _mod_store = Gtk::TreeStore::create( _mod_columns );
    _mod_tree.set_model(_mod_store);
    _mod_tree.append_column(_("Name"), _mod_columns.name);
    _mod_tree.append_column("hot", _mod_columns.and_modifiers);
    _mod_tree.append_column(_("ID"), _mod_columns.id);
    _mod_tree.set_tooltip_column(2);

    // In order to get tooltips on header, we must create our own label.
    auto and_keys_header = Gtk::manage(new Gtk::Label(_("Modifier")));
    and_keys_header->set_tooltip_text(_("All keys specified must be held down to activate this functionality."));
    and_keys_header->show();
    auto and_keys_column = _mod_tree.get_column(1);
    and_keys_column->set_widget(*and_keys_header);

    auto edit_bar = Gtk::manage(new Gtk::Box());
    _kb_mod_ctrl.set_label("Ctrl");
    _kb_mod_shift.set_label("Shift");
    _kb_mod_alt.set_label("Alt");
    _kb_mod_meta.set_label("Meta");
    _kb_mod_enabled.set_label(_("Enabled"));
    edit_bar->add(_kb_mod_ctrl);
    edit_bar->add(_kb_mod_shift);
    edit_bar->add(_kb_mod_alt);
    edit_bar->add(_kb_mod_meta);
    edit_bar->add(_kb_mod_enabled);
    _kb_mod_ctrl.signal_toggled().connect(sigc::mem_fun(*this, &InkscapePreferences::on_modifier_edited));
    _kb_mod_shift.signal_toggled().connect(sigc::mem_fun(*this, &InkscapePreferences::on_modifier_edited));
    _kb_mod_alt.signal_toggled().connect(sigc::mem_fun(*this, &InkscapePreferences::on_modifier_edited));
    _kb_mod_meta.signal_toggled().connect(sigc::mem_fun(*this, &InkscapePreferences::on_modifier_edited));
    _kb_mod_enabled.signal_toggled().connect(sigc::mem_fun(*this, &InkscapePreferences::on_modifier_enabled));
    _kb_page_modifiers.add_line(false, _("Change:"), *edit_bar, "", "", true);

    _mod_tree.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &InkscapePreferences::on_modifier_selection_changed));
    on_modifier_selection_changed();

    _kb_notebook.append_page(_kb_page_modifiers, _("Modifiers"));
    Gtk::ScrolledWindow* mod_scroller = new Gtk::ScrolledWindow();
    mod_scroller->add(_mod_tree);
    mod_scroller->set_hexpand();
    mod_scroller->set_vexpand();
    //_kb_page_modifiers.add(*mod_scroller);
    _kb_page_modifiers.attach(*mod_scroller, 0, 1, 2, 1);

    int row = 2;
    _page_keyshortcuts.attach(_kb_notebook, 0, row, 2, 1);

    row++;

    // ------ Reset/Import/Export -------
    auto box_buttons = Gtk::manage(new Gtk::ButtonBox);

    box_buttons->set_layout(Gtk::BUTTONBOX_END);
    box_buttons->set_spacing(4);

    box_buttons->set_hexpand();
    _page_keyshortcuts.attach(*box_buttons, 0, row, 3, 1);

    auto kb_reset = Gtk::manage(new Gtk::Button(_("Reset")));
    kb_reset->set_use_underline();
    kb_reset->set_tooltip_text(_("Remove all your customized keyboard shortcuts, and revert to the shortcuts in the shortcut file listed above"));
    box_buttons->pack_start(*kb_reset, true, true, 6);
    box_buttons->set_child_secondary(*kb_reset);

    auto kb_import = Gtk::manage(new Gtk::Button(_("Import ...")));
    kb_import->set_use_underline();
    kb_import->set_tooltip_text(_("Import custom keyboard shortcuts from a file"));
    box_buttons->pack_end(*kb_import, true, true, 6);

    auto kb_export = Gtk::manage(new Gtk::Button(_("Export ...")));
    kb_export->set_use_underline();
    kb_export->set_tooltip_text(_("Export custom keyboard shortcuts to a file"));
    box_buttons->pack_end(*kb_export, true, true, 6);

    kb_reset->signal_clicked().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBReset) );
    kb_import->signal_clicked().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBImport) );
    kb_export->signal_clicked().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBExport) );
    _kb_search.signal_key_release_event().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBSearchKeyEvent) );
    _kb_filelist.signal_changed().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBList) );
    _page_keyshortcuts.signal_realize().connect( sigc::mem_fun(*this, &InkscapePreferences::onKBRealize) );

    this->AddPage(_page_keyshortcuts, _("Keyboard"), iter_ui, PREFS_PAGE_UI_KEYBOARD_SHORTCUTS);

    _kb_shortcuts_loaded = false;
    Gtk::TreeStore::iterator iter_group = _kb_store->append();
    (*iter_group)[_kb_columns.name] = _("Loading ...");
    (*iter_group)[_kb_columns.shortcut] = "";
    (*iter_group)[_kb_columns.id] = "";
    (*iter_group)[_kb_columns.description] = "";
    (*iter_group)[_kb_columns.shortcutkey] = Gtk::AccelKey();
    (*iter_group)[_kb_columns.user_set] = 0;

    Gtk::TreeStore::iterator iter_mods = _mod_store->append();
    (*iter_mods)[_mod_columns.name] = _("Loading ...");
    (*iter_group)[_mod_columns.id] = "";
    (*iter_group)[_mod_columns.description] = _("It should have loaded by now. Hmmm.");
    (*iter_group)[_mod_columns.and_modifiers] = "";
}

void InkscapePreferences::onKBList()
{
    // New file path already stored in preferences.
    Inkscape::Shortcuts::getInstance().init();
    onKBListKeyboardShortcuts();
}

void InkscapePreferences::onKBReset()
{
    Inkscape::Shortcuts::getInstance().clear_user_shortcuts();
    onKBListKeyboardShortcuts();
}

void InkscapePreferences::onKBImport()
{
    if (Inkscape::Shortcuts::getInstance().import_shortcuts()) {
        onKBListKeyboardShortcuts();
    }
}

void InkscapePreferences::onKBExport()
{
    Inkscape::Shortcuts::getInstance().export_shortcuts();
}

bool InkscapePreferences::onKBSearchKeyEvent(GdkEventKey * /*event*/)
{
    _kb_filter->refilter();
    return FALSE;
}

void InkscapePreferences::onKBTreeCleared(const Glib::ustring& path)
{
    // Triggered by "Back" key.
    Gtk::TreeModel::iterator iter = _kb_filter->get_iter(path);
    Glib::ustring id = (*iter)[_kb_columns.id];
    // Gtk::AccelKey current_shortcut_key = (*iter)[_kb_columns.shortcutkey];

    // Remove current shortcut from user file (won't remove from other files).
    Inkscape::Shortcuts::getInstance().remove_user_shortcut(id);

    onKBListKeyboardShortcuts();
}

void InkscapePreferences::onKBTreeEdited (const Glib::ustring& path, guint accel_key, Gdk::ModifierType accel_mods, guint hardware_keycode)
{
    Inkscape::Shortcuts& shortcuts = Inkscape::Shortcuts::getInstance();

    Gtk::TreeModel::iterator iter = _kb_filter->get_iter(path);

    Glib::ustring id = (*iter)[_kb_columns.id];
    Glib::ustring current_shortcut = (*iter)[_kb_columns.shortcut];
    Gtk::AccelKey const current_shortcut_key = (*iter)[_kb_columns.shortcutkey];

    GdkEventKey event;
    event.keyval = accel_key;
    event.state = accel_mods;
    event.hardware_keycode = hardware_keycode;
    Gtk::AccelKey const new_shortcut_key =  shortcuts.get_from_event(&event, true);

    if (!new_shortcut_key.is_null() &&
        (new_shortcut_key.get_key() != current_shortcut_key.get_key() ||
         new_shortcut_key.get_mod() != current_shortcut_key.get_mod())
        ) {
        // check if there is currently a verb assigned to this shortcut; if yes ask if the shortcut should be reassigned
        Glib::ustring action_name;
        Inkscape::Verb *current_verb = shortcuts.get_verb_from_shortcut(new_shortcut_key);
        if (current_verb) {
            action_name = _(current_verb->get_name());
            Glib::ustring::size_type pos = 0;
            while ((pos = action_name.find('_', pos)) != action_name.npos) { // strip mnemonics
                action_name.erase(pos, 1);
            }
        } else {
            Glib::ustring accel = Gtk::AccelGroup::name(accel_key, accel_mods);
            auto *app = InkscapeApplication::instance()->gtk_app();
            std::vector<Glib::ustring> actions = app->get_actions_for_accel(accel);
            if (!actions.empty()) {
                action_name = actions[0];
            }
        }

        if (!action_name.empty()) {
            // Warn user about duplicated shortcuts.
            Glib::ustring message =
                Glib::ustring::compose(_("Keyboard shortcut \"%1\"\nis already assigned to \"%2\""),
                                       shortcuts.get_label(new_shortcut_key), action_name);
            Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
            dialog.set_title(_("Reassign shortcut?"));
            dialog.set_secondary_text(_("Are you sure you want to reassign this shortcut?"));
            dialog.set_transient_for(*dynamic_cast<Gtk::Window *>(get_toplevel()));
            int response = dialog.run();
            if (response != Gtk::RESPONSE_YES) {
                return;
            }
        }

        // Add the new shortcut.
        shortcuts.add_user_shortcut(id, new_shortcut_key);

        onKBListKeyboardShortcuts();
    }
}

bool InkscapePreferences::onKBSearchFilter(const Gtk::TreeModel::const_iterator& iter)
{
    Glib::ustring search = _kb_search.get_text().lowercase();
    if (search.empty()) {
        return TRUE;
    }

    Glib::ustring name = (*iter)[_kb_columns.name];
    Glib::ustring desc = (*iter)[_kb_columns.description];
    Glib::ustring shortcut = (*iter)[_kb_columns.shortcut];
    Glib::ustring id = (*iter)[_kb_columns.id];

    if (id.empty()) {
        return TRUE;    // Keep all group nodes visible
    }

    return (name.lowercase().find(search) != name.npos
            || shortcut.lowercase().find(search) != name.npos
            || desc.lowercase().find(search) != name.npos
            || id.lowercase().find(search) != name.npos);
}

void InkscapePreferences::onKBRealize()
{
    if (!_kb_shortcuts_loaded /*&& _current_page == &_page_keyshortcuts*/) {
        _kb_shortcuts_loaded = true;
        onKBListKeyboardShortcuts();
    }
}

InkscapePreferences::ModelColumns &InkscapePreferences::onKBGetCols()
{
    static InkscapePreferences::ModelColumns cols;
    return cols;
}

void InkscapePreferences::onKBShortcutRenderer(Gtk::CellRenderer *renderer, Gtk::TreeIter const &iter) {

    Glib::ustring shortcut = (*iter)[onKBGetCols().shortcut];
    unsigned int user_set = (*iter)[onKBGetCols().user_set];
    Gtk::CellRendererAccel *accel = dynamic_cast<Gtk::CellRendererAccel *>(renderer);
    if (user_set) {
        accel->property_markup() = Glib::ustring("<span foreground=\"blue\"> " + shortcut + " </span>").c_str();
    } else {
        accel->property_markup() = Glib::ustring("<span> " + shortcut + " </span>").c_str();
    }
}

void InkscapePreferences::on_modifier_selection_changed()
{
    _kb_is_updated = true;
    Gtk::TreeStore::iterator iter = _mod_tree.get_selection()->get_selected();
    bool selected = (iter);
    _kb_mod_ctrl.set_sensitive(selected);
    _kb_mod_shift.set_sensitive(selected);
    _kb_mod_alt.set_sensitive(selected);
    _kb_mod_meta.set_sensitive(selected);
    _kb_mod_enabled.set_sensitive(selected);

    _kb_mod_ctrl.set_active(false);
    _kb_mod_shift.set_active(false);
    _kb_mod_alt.set_active(false);
    _kb_mod_meta.set_active(false);
    _kb_mod_enabled.set_active(false);

    if (selected) {
        Glib::ustring modifier_id = (*iter)[_mod_columns.id];
        auto modifier = Modifiers::Modifier::get(modifier_id.c_str());
        Inkscape::Modifiers::KeyMask mask = Inkscape::Modifiers::NEVER;
        if(modifier != nullptr) {
            mask = modifier->get_and_mask();
        } else {
            _kb_mod_enabled.set_sensitive(false);
        }
        if(mask != Inkscape::Modifiers::NEVER) {
            _kb_mod_enabled.set_active(true);
            _kb_mod_ctrl.set_active(mask & Inkscape::Modifiers::CTRL);
            _kb_mod_shift.set_active(mask & Inkscape::Modifiers::SHIFT);
            _kb_mod_alt.set_active(mask & Inkscape::Modifiers::ALT);
            _kb_mod_meta.set_active(mask & Inkscape::Modifiers::META);
        } else {
            _kb_mod_ctrl.set_sensitive(false);
            _kb_mod_shift.set_sensitive(false);
            _kb_mod_alt.set_sensitive(false);
            _kb_mod_meta.set_sensitive(false);
        }
    }
    _kb_is_updated = false;
}

void InkscapePreferences::on_modifier_enabled()
{
    auto active = _kb_mod_enabled.get_active();
    _kb_mod_ctrl.set_sensitive(active);
    _kb_mod_shift.set_sensitive(active);
    _kb_mod_alt.set_sensitive(active);
    _kb_mod_meta.set_sensitive(active);
    on_modifier_edited();
}

void InkscapePreferences::on_modifier_edited()
{
    Gtk::TreeStore::iterator iter = _mod_tree.get_selection()->get_selected();
    if (!iter || _kb_is_updated) return;

    Glib::ustring modifier_id = (*iter)[_mod_columns.id];
    auto modifier = Modifiers::Modifier::get(modifier_id.c_str());
    if(!_kb_mod_enabled.get_active()) {
        modifier->set_user(Inkscape::Modifiers::NEVER, Inkscape::Modifiers::NOT_SET);
    } else {
        Inkscape::Modifiers::KeyMask mask = 0;
        if(_kb_mod_ctrl.get_active()) mask |= Inkscape::Modifiers::CTRL;
        if(_kb_mod_shift.get_active()) mask |= Inkscape::Modifiers::SHIFT;
        if(_kb_mod_alt.get_active()) mask |= Inkscape::Modifiers::ALT;
        if(_kb_mod_meta.get_active()) mask |= Inkscape::Modifiers::META;
        modifier->set_user(mask, Inkscape::Modifiers::NOT_SET);
    }
    (*iter)[_mod_columns.and_modifiers] = modifier->get_label();
}

void InkscapePreferences::onKBListKeyboardShortcuts()
{
    Inkscape::Shortcuts& shortcuts = Inkscape::Shortcuts::getInstance();

    // Save the current selection
    Gtk::TreeStore::iterator iter = _kb_tree.get_selection()->get_selected();
    Glib::ustring selected_id = "";
    if (iter) {
        selected_id = (*iter)[_kb_columns.id];
    }

    _kb_store->clear();
    _mod_store->clear();

    std::vector<Verb *>verbs = Inkscape::Verb::getList();

    for (auto verb : verbs) {

        if (!verb) {
            continue;
        }
        if (!verb->get_name()){
            continue;
        }

        Gtk::TreeStore::Path path;
        if (_kb_store->iter_is_valid(_kb_store->get_iter("0"))) {
            path = _kb_store->get_path(_kb_store->get_iter("0"));
        }

        // Find this group in the tree
        Glib::ustring group = verb->get_group() ? _(verb->get_group()) : _("Misc");
        Glib::ustring verb_id = verb->get_id();
        if (verb_id .compare(0,26,"org.inkscape.effect.filter") == 0) {
            group = _("Filters");
        }
        Gtk::TreeStore::iterator iter_group;
        bool found = false;
        while (path) {
            iter_group = _kb_store->get_iter(path);
            if (!_kb_store->iter_is_valid(iter_group)) {
                break;
            }
            Glib::ustring name = (*iter_group)[_kb_columns.name];
            if ((*iter_group)[_kb_columns.name] == group) {
                found = true;
                break;
            }
            path.next();
        }

        if (!found) {
            // Add the group if not there
            iter_group = _kb_store->append();
            (*iter_group)[_kb_columns.name] = group;
            (*iter_group)[_kb_columns.shortcut] = "";
            (*iter_group)[_kb_columns.id] = "";
            (*iter_group)[_kb_columns.description] = "";
            (*iter_group)[_kb_columns.shortcutkey] = Gtk::AccelKey();
            (*iter_group)[_kb_columns.user_set] = 0;
        }

        // Remove the key accelerators from the verb name
        Glib::ustring name = _(verb->get_name());
        std::string::size_type k = 0;
        while((k=name.find('_',k))!=name.npos) {
            name.erase(k, 1);
        }

        // Get the shortcut label
        Gtk::AccelKey shortcut_key = shortcuts.get_shortcut_from_verb(verb);
        Glib::ustring shortcut_label = "";
        if (!shortcut_key.is_null()) {
            shortcut_label = Glib::Markup::escape_text(shortcuts.get_label(shortcut_key));
        }
        // Add the verb to the group
        Gtk::TreeStore::iterator row = _kb_store->append(iter_group->children());
        (*row)[_kb_columns.name] =  name;
        (*row)[_kb_columns.shortcut] = shortcut_label;
        (*row)[_kb_columns.description] = verb->get_short_tip() ? _(verb->get_short_tip()) : "";
        (*row)[_kb_columns.shortcutkey] = shortcut_key;
        (*row)[_kb_columns.id] = verb->get_id();
        (*row)[_kb_columns.user_set] = shortcuts.is_user_set(verb);

        if (selected_id == verb->get_id()) {
            Gtk::TreeStore::Path sel_path = _kb_filter->convert_child_path_to_path(_kb_store->get_path(row));
            _kb_tree.expand_to_path(sel_path);
            _kb_tree.get_selection()->select(sel_path);
        }
    }

    // Gio::Actions

    auto iapp = InkscapeApplication::instance();
    auto gapp = iapp->gtk_app();

    // std::vector<Glib::ustring> actions = shortcuts.list_all_actions(); // All actions (app, win, doc)

    // Simpler and better to get action list from extra data (contains "detailed action names").
    InkActionExtraData& action_data = iapp->get_action_extra_data();
    std::vector<Glib::ustring> actions = action_data.get_actions();

    // Sort actions by section
    auto action_sort =
        [&](Glib::ustring &a, Glib::ustring &b) {
            return action_data.get_section_for_action(a) < action_data.get_section_for_action(b);
        };
    std::sort (actions.begin(), actions.end(), action_sort);

    Glib::ustring old_section;
    Gtk::TreeStore::iterator iter_group;

    // Fill sections
    for (auto action : actions) {

        Glib::ustring section = action_data.get_section_for_action(action);
        if (section.empty()) section = "Misc";
        if (section != old_section) {
            iter_group = _kb_store->append();
            Glib::ustring name = "Actions: " + section;
            (*iter_group)[_kb_columns.name] = name;
            (*iter_group)[_kb_columns.shortcut] = "";
            (*iter_group)[_kb_columns.description] = "";
            (*iter_group)[_kb_columns.shortcutkey] = Gtk::AccelKey();
            (*iter_group)[_kb_columns.id] = "";
            (*iter_group)[_kb_columns.user_set] = 0;
            old_section = section;
        }

        // Find accelerators
        std::vector<Glib::ustring> accels = gapp->get_accels_for_action(action);
        Glib::ustring shortcut_label;
        for (auto accel : accels) {
            // Convert to more user friendly notation.

            // ::get_label shows key pad and numeric keys identically.
            if (accel.find("KP") != Glib::ustring::npos) {
                shortcut_label += "Num Pad ";
            }
            unsigned int key = 0;
            Gdk::ModifierType mod = Gdk::ModifierType(0);
            Gtk::AccelGroup::parse(accel, key, mod);
            shortcut_label += Gtk::AccelGroup::get_label(key, mod) + ", ";
        }

        if (shortcut_label.size() > 1) {
            shortcut_label.erase(shortcut_label.size()-2);
        }

        // Find primary (i.e. first) shortcut.
        Gtk::AccelKey shortcut_key;
        if (accels.size() > 0) {
            unsigned int key = 0;
            Gdk::ModifierType mod = Gdk::ModifierType(0);
            Gtk::AccelGroup::parse(accels[0], key, mod);
            shortcut_key = Gtk::AccelKey(key, mod);
        }

        // Add the action to the group
        Gtk::TreeStore::iterator row = _kb_store->append(iter_group->children());
        (*row)[_kb_columns.name] = action_data.get_label_for_action(action);
        (*row)[_kb_columns.shortcut] = shortcut_label;
        (*row)[_kb_columns.description] = action_data.get_tooltip_for_action(action);
        (*row)[_kb_columns.shortcutkey] = shortcut_key;
        (*row)[_kb_columns.id] =  action;
        (*row)[_kb_columns.user_set] = shortcuts.is_user_set(action);

        if (selected_id == action) {
            Gtk::TreeStore::Path sel_path = _kb_filter->convert_child_path_to_path(_kb_store->get_path(row));
            _kb_tree.expand_to_path(sel_path);
            _kb_tree.get_selection()->select(sel_path);
        }
    }

    std::string old_mod_group;
    Gtk::TreeStore::iterator iter_mod_group;

    // Modifiers (mouse specific keys)
    for(auto modifier: Inkscape::Modifiers::Modifier::getList()) {
        auto cat_name = modifier->get_category();
        if (cat_name != old_mod_group) {
            iter_mod_group = _mod_store->append();
            (*iter_mod_group)[_mod_columns.name] = cat_name.c_str();
            (*iter_mod_group)[_mod_columns.id] = "";
            (*iter_mod_group)[_mod_columns.description] = "";
            (*iter_mod_group)[_mod_columns.and_modifiers] = "";
            (*iter_mod_group)[_mod_columns.user_set] = 0;
            old_mod_group = cat_name;
        }

        // Find accelerators
        Gtk::TreeStore::iterator iter_modifier = _mod_store->append(iter_mod_group->children());
        (*iter_modifier)[_mod_columns.name] = modifier->get_name();
        (*iter_modifier)[_mod_columns.id] = modifier->get_id();
        (*iter_modifier)[_mod_columns.description] = modifier->get_description();
        (*iter_modifier)[_mod_columns.and_modifiers] = modifier->get_label();
        (*iter_modifier)[_mod_columns.user_set] = modifier->is_set_user();
    }

    // re-order once after updating (then disable ordering again to increase performance)
    _kb_store->set_sort_column (_kb_columns.id, Gtk::SORT_ASCENDING );
    _kb_store->set_sort_column ( GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, Gtk::SORT_ASCENDING );

    if (selected_id.empty()) {
        _kb_tree.expand_to_path(_kb_store->get_path(_kb_store->get_iter("0:1")));
    }

    // Update all GUI text that includes shortcuts.
    for (auto win : gapp->get_windows()) {
        shortcuts.update_gui_text_recursive(win);
    }
}

void InkscapePreferences::initPageSpellcheck()
{
#if WITH_GSPELL

    _spell_ignorenumbers.init( _("Ignore words with digits"), "/dialogs/spellcheck/ignorenumbers", true);
    _page_spellcheck.add_line( false, "", _spell_ignorenumbers, "",
                           _("Ignore words containing digits, such as \"R2D2\""), true);

    _spell_ignoreallcaps.init( _("Ignore words in ALL CAPITALS"), "/dialogs/spellcheck/ignoreallcaps", false);
    _page_spellcheck.add_line( false, "", _spell_ignoreallcaps, "",
                           _("Ignore words in all capitals, such as \"IUPAC\""), true);

    this->AddPage(_page_spellcheck, _("Spellcheck"), PREFS_PAGE_SPELLCHECK);
#endif
}

template <typename string_type>
static void appendList(Glib::ustring& tmp, const std::vector<string_type> &listing)
{
    for (auto const & str : listing) {
        tmp += str;
        tmp += "\n";
    }
}


void InkscapePreferences::initPageSystem()
{
    _misc_latency_skew.init("/debug/latency/skew", 0.5, 2.0, 0.01, 0.10, 1.0, false, false);
    _page_system.add_line( false, _("Latency _skew:"), _misc_latency_skew, _("(requires restart)"),
                           _("Factor by which the event clock is skewed from the actual time (0.9766 on some systems)"), false);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _misc_namedicon_delay.init( _("Pre-render named icons"), "/options/iconrender/named_nodelay", false);
    _page_system.add_line( false, "", _misc_namedicon_delay, "",
                           _("When on, named icons will be rendered before displaying the ui. This is for working around bugs in GTK+ named icon notification"), true);

    _page_system.add_group_header( _("System info"));

    _sys_user_prefs.set_text(prefs->getPrefsFilename());
    _sys_user_prefs.set_editable(false);
    Gtk::Button* reset_prefs = Gtk::manage(new Gtk::Button(_("Reset Preferences")));
    reset_prefs->signal_clicked().connect(sigc::mem_fun(*this, &InkscapePreferences::on_reset_prefs_clicked));

    _page_system.add_line(true, _("User preferences: "), _sys_user_prefs, "",
                          _("Location of the user’s preferences file"), true, reset_prefs);

    _sys_user_config.init((char const *)Inkscape::IO::Resource::profile_path(""), _("Open preferences folder"));
    _page_system.add_line(true, _("User config: "), _sys_user_config, "", _("Location of users configuration"), true);

    auto extensions_folder = IO::Resource::get_path_string(IO::Resource::USER, IO::Resource::EXTENSIONS);
    _sys_user_extension_dir.init(extensions_folder,
                                 _("Open extensions folder"));
    _page_system.add_line(true, _("User extensions: "), _sys_user_extension_dir, "",
                          _("Location of the user’s extensions"), true);

    _sys_user_fonts_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::FONTS, ""),
                             _("Open fonts folder"));
    _page_system.add_line(true, _("User fonts: "), _sys_user_fonts_dir, "", _("Location of the user’s fonts"), true);

    _sys_user_themes_dir.init(g_build_filename(g_get_user_data_dir(), "themes", NULL), _("Open themes folder"));
    _page_system.add_line(true, _("User themes: "), _sys_user_themes_dir, "", _("Location of the user’s themes"), true);

    _sys_user_icons_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::ICONS, ""),
                             _("Open icons folder"));
    _page_system.add_line(true, _("User icons: "), _sys_user_icons_dir, "", _("Location of the user’s icons"), true);

    _sys_user_templates_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::TEMPLATES, ""),
                                 _("Open templates folder"));
    _page_system.add_line(true, _("User templates: "), _sys_user_templates_dir, "",
                          _("Location of the user’s templates"), true);

    _sys_user_symbols_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::SYMBOLS, ""),
                               _("Open symbols folder"));

    _page_system.add_line(true, _("User symbols: "), _sys_user_symbols_dir, "", _("Location of the user’s symbols"),
                          true);

    _sys_user_paint_servers_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::PAINT, ""),
                                _("Open paint servers folder"));

    _page_system.add_line(true, _("User paint servers: "), _sys_user_paint_servers_dir, "",
                        _("Location of the user’s paint servers"), true);

    _sys_user_palettes_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::PALETTES, ""),
                                _("Open palettes folder"));
    _page_system.add_line(true, _("User palettes: "), _sys_user_palettes_dir, "", _("Location of the user’s palettes"),
                          true);

    _sys_user_keys_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::KEYS, ""),
                            _("Open keyboard shortcuts folder"));
    _page_system.add_line(true, _("User keys: "), _sys_user_keys_dir, "",
                          _("Location of the user’s keyboard mapping files"), true);

    _sys_user_ui_dir.init((char const *)IO::Resource::get_path(IO::Resource::USER, IO::Resource::UIS, ""),
                          _("Open user interface folder"));
    _page_system.add_line(true, _("User UI: "), _sys_user_ui_dir, "",
                          _("Location of the user’s user interface description files"), true);

    _sys_user_cache.set_text(g_get_user_cache_dir());
    _sys_user_cache.set_editable(false);
    _page_system.add_line(true, _("User cache: "), _sys_user_cache, "", _("Location of user’s cache"), true);

    Glib::ustring tmp_dir = prefs->getString("/options/autosave/path");
    if (tmp_dir.empty()) {
        tmp_dir = Glib::build_filename(Glib::get_user_cache_dir(), "inkscape");
    }
    _sys_tmp_files.set_text(tmp_dir);
    _sys_tmp_files.set_editable(false);
    _page_system.add_line(true, _("Temporary files: "), _sys_tmp_files, "", _("Location of the temporary files used for autosave"), true);

    _sys_data.set_text(get_inkscape_datadir());
    _sys_data.set_editable(false);
    _page_system.add_line(true, _("Inkscape data: "), _sys_data, "", _("Location of Inkscape data"), true);

    extensions_folder = IO::Resource::get_path_string(IO::Resource::SYSTEM, IO::Resource::EXTENSIONS);
    _sys_extension_dir.set_text(extensions_folder);
    _sys_extension_dir.set_editable(false);
    _page_system.add_line(true, _("Inkscape extensions: "), _sys_extension_dir, "", _("Location of the Inkscape extensions"), true);

    Glib::ustring tmp;
    auto system_data_dirs = Glib::get_system_data_dirs();
    appendList(tmp, system_data_dirs);
    _sys_systemdata.get_buffer()->insert(_sys_systemdata.get_buffer()->end(), tmp);
    _sys_systemdata.set_editable(false);
    _sys_systemdata_scroll.add(_sys_systemdata);
    _sys_systemdata_scroll.set_size_request(100, 80);
    _sys_systemdata_scroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _sys_systemdata_scroll.set_shadow_type(Gtk::SHADOW_IN);
    _page_system.add_line(true,  _("System data: "), _sys_systemdata_scroll, "", _("Locations of system data"), true);

    _sys_fontdirs_custom.init("/options/font/custom_fontdirs", 50);
    _page_system.add_line(true, _("Custom Font directories"), _sys_fontdirs_custom, "", _("Load additional fonts from custom locations (one path per line)"), true);

    tmp = "";
    auto icon_theme = Gtk::IconTheme::get_default();
    auto paths = icon_theme->get_search_path();
    appendList( tmp, paths );
    _sys_icon.get_buffer()->insert(_sys_icon.get_buffer()->end(), tmp);
    _sys_icon.set_editable(false);
    _sys_icon_scroll.add(_sys_icon);
    _sys_icon_scroll.set_size_request(100, 80);
    _sys_icon_scroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    _sys_icon_scroll.set_shadow_type(Gtk::SHADOW_IN);
    _page_system.add_line(true,  _("Icon theme: "), _sys_icon_scroll, "", _("Locations of icon themes"), true);

    this->AddPage(_page_system, _("System"), PREFS_PAGE_SYSTEM);
}

bool InkscapePreferences::GetSizeRequest(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    DialogPage* page = row[_page_list_columns._col_page];
    _page_frame.add(*page);
    this->show_all_children();
    Gtk::Requisition sreq_minimum;
    Gtk::Requisition sreq_natural;
    get_preferred_size(sreq_minimum, sreq_natural);
    _minimum_width  = std::max(_minimum_width,  sreq_minimum.width);
    _minimum_height = std::max(_minimum_height, sreq_minimum.height);
    _natural_width  = std::max(_natural_width,  sreq_natural.width);
    _natural_height = std::max(_natural_height, sreq_natural.height);
    _page_frame.remove();
    return false;
}

bool InkscapePreferences::PresentPage(const Gtk::TreeModel::iterator& iter)
{
    Gtk::TreeModel::Row row = *iter;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int desired_page = prefs->getInt("/dialogs/preferences/page", 0);
    _init = false;
    if (desired_page == row[_page_list_columns._col_id])
    {
        auto const path = _page_list.get_model()->get_path(*iter);
        _page_list.expand_to_path(path);
        _page_list.get_selection()->select(iter);
        if (desired_page == PREFS_PAGE_UI_THEME)
            symbolicThemeCheck();
        return true;
    }
    return false;
}

void InkscapePreferences::on_reset_open_recent_clicked()
{
    Glib::RefPtr<Gtk::RecentManager> manager = Gtk::RecentManager::get_default();
    std::vector< Glib::RefPtr< Gtk::RecentInfo > > recent_list = manager->get_items();

    // Remove only elements that were added by Inkscape
    // TODO: This should likely preserve items that were also accessed by other apps.
    //       However there does not seem to be straightforward way to delete only an application from an item.
    for (auto e : recent_list) {
        if (e->has_application(g_get_prgname())
            || e->has_application("org.inkscape.Inkscape")
            || e->has_application("inkscape")
#ifdef _WIN32
            || e->has_application("inkscape.exe")
#endif
           ) {
            manager->remove_item(e->get_uri());
        }
    }
}

void InkscapePreferences::on_reset_prefs_clicked()
{
    Inkscape::Preferences::get()->reset();
}

void InkscapePreferences::show_not_found()
{
    if (_current_page)
        _page_frame.remove();
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _current_page = &_page_notfound;
    _page_title.set_markup("<span size='large'><b>No Results</b></span>");
    _page_frame.add(*_current_page);
    _current_page->show();
    this->show_all_children();
    if (prefs->getInt("/dialogs/preferences/page", 0) == PREFS_PAGE_UI_THEME) {
        symbolicThemeCheck();
    }
}

void InkscapePreferences::show_nothing_on_page()
{
    _page_frame.remove();
    _page_title.set_text("");
}

void InkscapePreferences::on_pagelist_selection_changed()
{
    // show new selection
    Glib::RefPtr<Gtk::TreeSelection> selection = _page_list.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter)
    {
        if (_current_page)
            _page_frame.remove();
        Gtk::TreeModel::Row row = *iter;
        _current_page = row[_page_list_columns._col_page];
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (!_init) {
            prefs->setInt("/dialogs/preferences/page", row[_page_list_columns._col_id]);
        }
        Glib::ustring col_name_escaped = Glib::Markup::escape_text( row[_page_list_columns._col_name] );
        _page_title.set_markup("<span size='large'><b>" + col_name_escaped + "</b></span>");
        _page_frame.add(*_current_page);
        _current_page->show();
        this->show_all_children();
        if (prefs->getInt("/dialogs/preferences/page", 0) == PREFS_PAGE_UI_THEME) {
            symbolicThemeCheck();
        }
    }
}

void InkscapePreferences::_presentPages()
{
    _search.set_text("");
    _page_list.get_model()->foreach_iter(sigc::mem_fun(*this, &InkscapePreferences::PresentPage));
}

} // namespace Dialog
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
