// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Filter Editor dialog
 */
/* Authors:
 *   Marc Jeanmougin
 *
 * Copyright (C) 2017 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_LPE_SELECTOR_H
#define INKSCAPE_UI_LPE_SELECTOR_H

#include <gtkmm/gtkmm.h>

namespace Inkscape {
namespace UI {

class LPESelector : public Gtk::Box {
  public:
    LPESelector();
    ~LPESelector() override;

    static LPESelector &getInstance() { return *new LPESelector(); }

    //    void set_attrs_locked(const bool);
  private:
    Glib::RefPtr<Gtk::Builder> builder;
    Glib::RefPtr<Glib::Object> FilterStore;
    Gtk::Box *LPESelector;
};
} // namespace UI
} // namespace Inkscape
#endif
