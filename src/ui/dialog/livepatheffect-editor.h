// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Live Path Effect editing dialog
 */
/* Author:
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2007 Author
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_LIVE_PATH_EFFECT_H
#define INKSCAPE_UI_DIALOG_LIVE_PATH_EFFECT_H

#include <gtkmm/buttonbox.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/treeview.h>

#include "live_effects/effect-enum.h"
#include "object/sp-item.h"
#include "selection.h"
#include "ui/dialog/dialog-base.h"
#include "ui/widget/combo-enums.h"
#include "ui/widget/frame.h"

class SPDesktop;
class SPLPEItem;

namespace Inkscape {

namespace LivePathEffect {
    class Effect;
    class LPEObjectReference;
}

namespace UI {
namespace Dialog {

class LivePathEffectEditor : public DialogBase
{
public:
    LivePathEffectEditor();
    ~LivePathEffectEditor() override;

    static LivePathEffectEditor &getInstance() { return *new LivePathEffectEditor(); }

    void onSelectionChanged(Inkscape::Selection *sel);
    void onSelectionModified(Inkscape::Selection *sel);
    virtual void on_effect_selection_changed();
    void setDesktop(SPDesktop *desktop);

    void update() override;

private:

    sigc::connection selection_changed_connection;
    sigc::connection selection_modified_connection;

    // void add_entry(const char* name );
    void effect_list_reload(SPLPEItem *lpeitem);

    void set_sensitize_all(bool sensitive);
    void showParams(LivePathEffect::Effect& effect);
    void showText(Glib::ustring const &str);
    void selectInList(LivePathEffect::Effect* effect);

    // callback methods for buttons on grids page.
    void onAdd();
    void onRemove();
    void onUp();
    void onDown();

    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
      public:
        ModelColumns()
        {
            add(col_name);
            add(lperef);
            add(col_visible);
        }
        ~ModelColumns() override = default;

        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<LivePathEffect::LPEObjectReference *> lperef;
        Gtk::TreeModelColumn<bool> col_visible;
    };

    bool lpe_list_locked;
    bool selection_changed_lock;
    //Inkscape::UI::Widget::ComboBoxEnum<LivePathEffect::EffectType> combo_effecttype;

    Gtk::Widget * effectwidget;
    Gtk::Label status_label;
    UI::Widget::Frame effectcontrol_frame;
    Gtk::Box effectapplication_hbox;
    Gtk::Box effectcontrol_vbox;
    Gtk::EventBox effectcontrol_eventbox;
    Gtk::Box effectlist_vbox;
    ModelColumns columns;
    Gtk::ScrolledWindow scrolled_window;
    Gtk::TreeView effectlist_view;
    Glib::RefPtr<Gtk::ListStore> effectlist_store;
    Glib::RefPtr<Gtk::TreeSelection> effectlist_selection;

    void on_visibility_toggled( Glib::ustring const& str);
    bool _on_button_release(GdkEventButton* button_event);
    Gtk::ButtonBox toolbar_hbox;
    Gtk::Button button_add;
    Gtk::Button button_remove;
    Gtk::Button button_up;
    Gtk::Button button_down;

    SPDesktop * current_desktop;
    Inkscape::Selection *_getSelection() { return current_desktop ? current_desktop->getSelection() : nullptr; }

    SPLPEItem * current_lpeitem;

    LivePathEffect::LPEObjectReference * current_lperef;

    friend void lpeeditor_selection_changed (Inkscape::Selection * selection, gpointer data);
    friend void lpeeditor_selection_modified (Inkscape::Selection * selection, guint /*flags*/, gpointer data);

    LivePathEffectEditor(LivePathEffectEditor const &d);
    LivePathEffectEditor& operator=(LivePathEffectEditor const &d);
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_DIALOG_LIVE_PATH_EFFECT_H

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
