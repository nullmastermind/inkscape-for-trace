// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for the start screen
 *
 * Copyright (C) Martin Owens 2020 <doctormo@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <gtkmm.h>

class SPDocument;

namespace Inkscape {
namespace UI {
namespace Dialog {

class StartScreen : public Gtk::Dialog {

public:
    StartScreen();
    ~StartScreen() override;

    SPDocument* get_document() { return _document; }

protected:
    bool on_key_press_event(GdkEventKey* event) override;

private:
    void notebook_next(Gtk::Widget *button);
    Gtk::TreeModel::Row active_combo(std::string widget_name);
    void set_active_combo(std::string widget_name, std::string unique_id);
    void show_toggle();
    void enlist_recent_files();
    void enlist_keys();
    void filter_themes();
    void keyboard_changed();
    void notebook_switch(Gtk::Widget *tab, guint page_num);

    void theme_changed();
    void canvas_changed();
    void refresh_theme(Glib::ustring theme_name);
    void refresh_dark_switch();

    void new_now();
    void load_now();
    void on_recent_changed();
    void on_kind_changed(Gtk::Widget *tab, guint page_num);


private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Window   *window  = nullptr;
    Gtk::Notebook *tabs    = nullptr;
    Gtk::Notebook *kinds   = nullptr;
    Gtk::Fixed    *banners = nullptr;
    Gtk::ComboBox *themes  = nullptr;
    Gtk::TreeView *recent_treeview = nullptr;
    Gtk::Button   *load_btn = nullptr;

    SPDocument* _document = nullptr;
};


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // STARTSCREEN_H
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
