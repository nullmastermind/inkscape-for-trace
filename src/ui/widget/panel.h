// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2004 Bryce Harrington
 * Copyright (C) 2005 Jon A. Cruz
 * Copyright (C) 2012 Kris De Gussem
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_UI_WIDGET_PANEL_H
#define SEEN_INKSCAPE_UI_WIDGET_PANEL_H

#include <gtkmm/box.h>
#include <map>

class SPDesktop;
class SPDocument;

namespace Gtk {
    class Button;
    class ButtonBox;
}

struct InkscapeApplication;

namespace Inkscape {

class Selection;

namespace UI {

namespace Widget {

/**
 * A generic dockable container.
 *
 * Inkscape::UI::Widget::Panel is a base class from which dockable dialogs
 * are created. A new dockable dialog is created by deriving a class from panel.
 * Child widgets are private data members of Panel (no need to use pointers and
 * new).
 *
 * @see UI::Dialog::DesktopTracker to handle desktop change, selection change and selected object modifications.
 * @see UI::Dialog::DialogManager manages the dialogs within inkscape.
 */
class Panel : public Gtk::Box {
public:
    static void prep();

    /**
     * Construct a Panel.
     *
     * @param prefs_path characteristic path to load/save dialog position.
     * @param verb_num the dialog verb.
     */
    Panel(gchar const *prefs_path = nullptr, int verb_num = 0);
    ~Panel() override;

    gchar const *getPrefsPath() const;
    
    int const &getVerb() const;

    virtual void present();  //< request to be present

    void restorePanelPrefs();

    virtual void setDesktop(SPDesktop *desktop);
    SPDesktop *getDesktop() { return _desktop; }

    /* Signal accessors */
    virtual sigc::signal<void, int> &signalResponse();
    virtual sigc::signal<void> &signalPresent();

    /* Methods providing a Gtk::Dialog like interface for adding buttons that emit Gtk::RESPONSE
     * signals on click. */
    Gtk::Button* addResponseButton (const Glib::ustring &button_text, int response_id, bool pack_start=false);
    void setResponseSensitive(int response_id, bool setting);

    /* Return signals. Signals emitted by PanelDialog. */
    virtual sigc::signal<void, SPDesktop *, SPDocument *> &signalDocumentReplaced();
    virtual sigc::signal<void, SPDesktop *> &signalActivateDesktop();
    virtual sigc::signal<void, SPDesktop *> &signalDeactiveDesktop();

protected:
    /**
     * Returns a pointer to a Gtk::Box containing the child widgets.
     */
    Gtk::Box *_getContents() { return &_contents; }
    virtual void _apply();

    virtual void _handleResponse(int response_id);

    /* Helper methods */
    Inkscape::Selection *_getSelection();

    /**
     * Stores characteristic path for loading/saving the dialog position.
     */
    Glib::ustring const _prefs_path;

    /* Signals */
    sigc::signal<void, int> _signal_response;
    sigc::signal<void>      _signal_present;
    sigc::signal<void, SPDesktop *, SPDocument *> _signal_document_replaced;
    sigc::signal<void, SPDesktop *> _signal_activate_desktop;
    sigc::signal<void, SPDesktop *> _signal_deactive_desktop;

private:
    SPDesktop       *_desktop;

    int              _verb_num;

    Gtk::VBox        _contents;
    Gtk::ButtonBox  *_action_area;  //< stores response buttons

    /* A map to store which widget that emits a certain response signal */
    typedef std::map<int, Gtk::Widget *> ResponseMap;
    ResponseMap _response_map;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // SEEN_INKSCAPE_UI_WIDGET_PANEL_H

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
