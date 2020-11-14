// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A bare minimum example of deriving from Inkscape::UI:Widget::Panel.
 *
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) Tavmjong Bah <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_PROTOTYPE_PANEL_H
#define SEEN_PROTOTYPE_PANEL_H

#include <iostream>

#include "selection.h"
#include "ui/dialog/dialog-base.h"

// Only to display status.
#include <gtkmm/label.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * A panel that does almost nothing!
 */
class Prototype : public DialogBase
{
public:
    ~Prototype() override { std::cout << "Prototype::~Prototype()" << std::endl; }
    static Prototype &getInstance() { return *new Prototype(); }

    void update() override;

private:
    // No default constructor, noncopyable, nonassignable
    Prototype();
    Prototype(Prototype const &d) = delete;
    Prototype operator=(Prototype const &d) = delete;

    // Handlers
    void handleDocumentReplaced(SPDocument *document);
    void handleSelectionChanged(Inkscape::Selection *selection);

    // Just for example
    Gtk::Label *_label;
    Gtk::Button _debug_button; // For printing to console.
    virtual void on_click();
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // SEEN_PROTOTYPE_PANEL_H

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
