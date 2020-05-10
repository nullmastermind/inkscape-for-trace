// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for the about screen
 *
 * Copyright (C) Martin Owens 2019 <doctormo@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <gtkmm.h>
#include <gtkmm/builder.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

class AboutDialog {

  public:
    static void show_about();

  private:
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // ABOUTDIALOG_H

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
