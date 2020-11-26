// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INK_DIALOG_BASE_H
#define INK_DIALOG_BASE_H

/** @file
 * @brief A base class for all dialogs.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>
#include <gtkmm/box.h>

#include "inkscape-application.h"

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * DialogBase is the base class for the dialog system.
 *
 * Each dialog has a reference to the application, in order to update it's inner focus
 * (be it of the active desktop, document, selection, etc.) in the update() method.
 *
 * DialogsBase derived classes' instances live in DialogNotebook classes and are managed by
 * DialogContainer classes. DialogContainer instances can have at most one type of dialog,
 * differentiated by the associated verb.
 */
class DialogBase : public Gtk::Box
{
    using parent_type = Gtk::Box;

public:
    DialogBase(gchar const *prefs_path = nullptr, int verb_num = 0);
    ~DialogBase() override{};

    /**
     * The update() method is essential to state management. DialogBase implementations get updated whenever
     * a new focus event happens if they are in a DialogWindow or if they are in the currently focused window.
     */
    virtual void update() {}

    void on_map() override
    {
        update();
        parent_type::on_map();
    }

    // Getters and setters
    std::string get_name() { return _name; };
    gchar const *getPrefsPath() const { return _prefs_path.data(); }
    int const &getVerb() const { return _verb_num; }
    SPDesktop *getDesktop();

    void blink();

protected:
    std::string _name;               // Gtk widget name (must be set!)
    Glib::ustring const _prefs_path; // Stores characteristic path for loading/saving the dialog position.
    int _verb_num;                   // Dialog associated verb value
    InkscapeApplication *_app; // Used for state management

private:
    bool blink_off(); // timer callback
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INK_DIALOG_BASE_H

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
