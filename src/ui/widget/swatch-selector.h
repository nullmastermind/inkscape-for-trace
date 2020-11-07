// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_SWATCH_SELECTOR_H
#define SEEN_SP_SWATCH_SELECTOR_H

#include <gtkmm/box.h>
#include "ui/selected-color.h"

class SPDocument;
class SPGradient;
struct SPColorSelector;

namespace Inkscape {
namespace UI {
namespace Widget {
class GradientSelector;

class SwatchSelector : public Gtk::Box
{
public:
    SwatchSelector();
    ~SwatchSelector() override;

    void connectchangedHandler( GCallback handler, void *data );

    void setVector(SPDocument *doc, SPGradient *vector);

    GradientSelector *getGradientSelector();

private:
    void _grabbedCb();
    void _draggedCb();
    void _releasedCb();
    void _changedCb();

    GradientSelector *_gsel;
    Inkscape::UI::SelectedColor _selected_color;
    bool _updating_color;
};


} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // SEEN_SP_SWATCH_SELECTOR_H

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

