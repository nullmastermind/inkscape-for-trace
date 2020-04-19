// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Dialog for adding a live path effect.
 *
 * Author:
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DIALOG_LIVEPATHEFFECT_ADD_H
#define INKSCAPE_DIALOG_LIVEPATHEFFECT_ADD_H

#include "live_effects/effect-enum.h"
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/flowboxchild.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/overlay.h>
#include <gtkmm/popover.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/stylecontext.h>
#include <gtkmm/switch.h>
#include <gtkmm/viewport.h>

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Dialog {
      
/**
 * A dialog widget to list the live path effects that can be added
 *
 */
class LivePathEffectAdd {
  public:
    LivePathEffectAdd();
    ~LivePathEffectAdd() = default;
    ;

    /**
     * Show the dialog
     */
    static void show(SPDesktop *desktop);
    /**
     * Returns true is the "Add" button was pressed
     */
    static bool isApplied() { return instance()._applied; }

    static const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *getActiveData();

  protected:
    /**
     * Close button was clicked
     */
    void onClose();
    bool on_filter(Gtk::FlowBoxChild *child);
    int on_sort(Gtk::FlowBoxChild *child1, Gtk::FlowBoxChild *child2);
    void on_search();
    void on_focus(Gtk::Widget *widg);
    bool pop_description(GdkEventCrossing *evt, Glib::RefPtr<Gtk::Builder> builder_effect);
    bool hide_pop_description(GdkEventCrossing *evt);
    bool fav_toggler(GdkEventButton *evt, Glib::RefPtr<Gtk::Builder> builder_effect);
    bool apply(GdkEventButton *evt, Glib::RefPtr<Gtk::Builder> builder_effect,
               const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *to_add);
    bool on_press_enter(GdkEventKey *key, Glib::RefPtr<Gtk::Builder> builder_effect,
                        const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *to_add);
    bool expand(GdkEventButton *evt, Glib::RefPtr<Gtk::Builder> builder_effect);
    bool show_fav_toggler(GdkEventButton *evt);
    void viewChanged(gint mode);
    bool mouseover(GdkEventCrossing *evt, GtkWidget *wdg);
    bool mouseout(GdkEventCrossing *evt, GtkWidget *wdg);
    void reload_effect_list();
    /**
     * Add button was clicked
     */
    void onAdd();
    /**
     * Tree was clicked
     */
    void onButtonEvent(GdkEventButton* evt);

    /**
     * Key event
     */
    void onKeyEvent(GdkEventKey* evt);
private:
  Gtk::Button _add_button;
  Gtk::Button _close_button;
  Gtk::Dialog *_LPEDialogSelector;
  Glib::RefPtr<Gtk::Builder> _builder;
  Gtk::FlowBox *_LPESelectorFlowBox;
  Gtk::Popover *_LPESelectorEffectInfoPop;
  Gtk::EventBox *_LPESelectorEffectEventFavShow;
  Gtk::EventBox *_LPESelectorEffectInfoEventBox;
  Gtk::RadioButton *_LPESelectorEffectRadioList;
  Gtk::RadioButton *_LPESelectorEffectRadioPackLess;
  Gtk::RadioButton *_LPESelectorEffectRadioPackMore;
  Gtk::Switch *_LPEExperimental;
  Gtk::SearchEntry *_LPEFilter;
  Gtk::ScrolledWindow *_LPEScrolled;
  Gtk::Label *_LPEInfo;
  Gtk::Box *_LPESelector;
  guint _visiblelpe;
  Glib::ustring _item_type;
  bool _has_clip;
  bool _has_mask;
  Gtk::FlowBoxChild *_lasteffect;
  const LivePathEffect::EnumEffectData<LivePathEffect::EffectType> *_to_add;
  bool _showfavs;
  bool _applied;
  class Effect;
  const LivePathEffect::EnumEffectDataConverter<LivePathEffect::EffectType> &converter;
  static LivePathEffectAdd &instance()
  {
      static LivePathEffectAdd instance_;
      return instance_;
  }
  LivePathEffectAdd(LivePathEffectAdd const &) = delete;            // no copy
  LivePathEffectAdd &operator=(LivePathEffectAdd const &) = delete; // no assign
};
 
} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_DIALOG_LIVEPATHEFFECT_ADD_H

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
