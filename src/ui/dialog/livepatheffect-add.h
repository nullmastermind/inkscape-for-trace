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
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/flowboxchild.h>
#include <gtkmm/label.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/stylecontext.h>

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Dialog {
      
/**
 * A dialog widget to list the live path effects that can be added
 *
 */
class LivePathEffectAdd : public Gtk::Dialog {
public:
    LivePathEffectAdd();
    ~LivePathEffectAdd() override = default;

    /**
     * Show the dialog
     */
    static void show(SPDesktop *desktop);
    static bool isApplied() { return false; }

    static const Util::EnumData<LivePathEffect::EffectType> *getActiveData() { return NULL; };

  protected:
    /**
     * Close button was clicked
     */
    void onClose();
    bool on_filter(Gtk::FlowBoxChild *child);
    void on_search();
    void on_activate(Gtk::FlowBoxChild *child);
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
  Gtk::SearchEntry *_LPEFilter;
  Gtk::Label *_LPEInfo;
  Gtk::Box *_LPESelector;
  guint _visiblelpe;
  class Effect;
  const LivePathEffect::EnumEffectDataConverter<LivePathEffect::EffectType> &converter;
  static LivePathEffectAdd &instance()
  {
      static LivePathEffectAdd instance_;
      return instance_;
  }
    LivePathEffectAdd(LivePathEffectAdd const &) = delete; // no copy
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
