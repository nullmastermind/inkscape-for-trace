/*
 * A bare minimum example of deriving from Inkscape::UI:Widget::Panel.
 *
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) Tavmjong Bah <tavmjong@free.fr>
 *
 * Released under the GNU GPL, read the file 'COPYING' for more information.
 */

#ifndef SEEN_PROTOTYPE_PANEL_H
#define SEEN_PROTOTYPE_PANEL_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <iostream>
#include "ui/widget/panel.h"
#include "ui/dialog/desktop-tracker.h"

// Only to display status.
#include <gtkmm/label.h>

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * A panel that does almost nothing!
 */
class Prototype : public UI::Widget::Panel
{
public:
    virtual ~Prototype();

    static Prototype& getInstance() { return *new Prototype(); };

    virtual void present();

private:

    // No default constructor, noncopyable, nonassignable
    Prototype();
    Prototype(Prototype const &d);
    Prototype operator=(Prototype const &d);

    // Signals and handlers
    sigc::connection connectionDocumentReplaced;
    sigc::connection connectionDesktopChanged;
    sigc::connection connectionSelectionChanged;

    void handleDocumentReplaced(SPDesktop* desktop, SPDocument *document);
    void handleDesktopChanged(SPDesktop* desktop);
    void handleSelectionChanged();

    DesktopTracker desktopTracker;

    // Just for example
    Gtk::Label label;
    void updateLabel();
};

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape

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
