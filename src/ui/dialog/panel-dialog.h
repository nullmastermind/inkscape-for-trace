// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A panel holding dialog
 */
/* Authors:
 *   C 2007 Gustav Broberg <broberg@kth.se>
 *   C 2012 Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_PANEL_DIALOG_H
#define INKSCAPE_PANEL_DIALOG_H

#include <glibmm/i18n.h>
#include <gtkmm/dialog.h>

#include "verbs.h"
#include "dialog.h"
#include "ui/dialog/swatches.h"
#include "ui/dialog/floating-behavior.h"
#include "ui/dialog/dock-behavior.h"
#include "inkscape.h"
#include "desktop.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

/**
 * Auxiliary class for the link between UI::Dialog::PanelDialog and UI::Dialog::Dialog.
 *
 * PanelDialog handles signals emitted when a desktop changes, either changing to a
 * different desktop or a new document.
 */
class PanelDialogBase {
public:
    PanelDialogBase(UI::Widget::Panel &panel, char const */*prefs_path*/, int const /*verb_num*/) :
      _panel (panel) { }

    virtual void present() = 0;
    virtual ~PanelDialogBase() = default;

    virtual UI::Widget::Panel &getPanel() { return _panel; }

protected:

    inline virtual void _propagateDocumentReplaced(SPDesktop* desktop, SPDocument *document);
    inline virtual void _propagateDesktopActivated(SPDesktop *);
    inline virtual void _propagateDesktopDeactivated(SPDesktop *);

    UI::Widget::Panel &_panel;
    sigc::connection _document_replaced_connection;
};

/**
 * Bridges UI::Widget::Panel and UI::Dialog::Dialog.
 *
 * Where Dialog handles window behaviour, such as closing, position, etc, and where
 * Panel is the actual container for dialog child widgets (and from where the dialog
 * content is made), PanelDialog links these two classes together to create a
 * dockable and floatable dialog. The link with Dialog is made via PanelDialogBase.
 */
template <typename Behavior>
class PanelDialog : public PanelDialogBase, public Inkscape::UI::Dialog::Dialog {

public:
    /**
     * Constructor.
     * 
     * @param contents panel with the actual dialog content.
     * @param prefs_path characteristic path for loading/saving dialog position.
     * @param verb_num the dialog verb.
     */
    PanelDialog(UI::Widget::Panel &contents, char const *prefs_path, int const verb_num);

    ~PanelDialog() override = default;

    template <typename T>
    static PanelDialog<Behavior> *create();

    inline void present() override;

private:
    template <typename T>
    static PanelDialog<Behavior> *_create();

    inline void _presentDialog();

    PanelDialog() = delete;
    PanelDialog(PanelDialog<Behavior> const &d) = delete;                      // no copy
    PanelDialog<Behavior>& operator=(PanelDialog<Behavior> const &d) = delete; // no assign
};


void PanelDialogBase::_propagateDocumentReplaced(SPDesktop *desktop, SPDocument *document)
{
    _panel.signalDocumentReplaced().emit(desktop, document);
}

void PanelDialogBase::_propagateDesktopActivated(SPDesktop *desktop)
{
    _document_replaced_connection =
        desktop->connectDocumentReplaced(sigc::mem_fun(*this, &PanelDialogBase::_propagateDocumentReplaced));
    _panel.signalActivateDesktop().emit(desktop);
}

void PanelDialogBase::_propagateDesktopDeactivated(SPDesktop *desktop)
{
    _document_replaced_connection.disconnect();
    _panel.signalDeactiveDesktop().emit(desktop);
}


template <typename B>
PanelDialog<B>::PanelDialog(Widget::Panel &panel, char const *prefs_path, int const verb_num) :
    PanelDialogBase(panel, prefs_path, verb_num),
    Dialog(&B::create, prefs_path, verb_num)
{
    Gtk::Box *vbox = get_vbox();
    _panel.signalResponse().connect(sigc::mem_fun(*this, &PanelDialog::_handleResponse));
    _panel.signalPresent().connect(sigc::mem_fun(*this, &PanelDialog::_presentDialog));

    vbox->pack_start(_panel, true, true, 0);

    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    _propagateDesktopActivated(desktop);

    _document_replaced_connection =
        desktop->connectDocumentReplaced(sigc::mem_fun(*this, &PanelDialog::_propagateDocumentReplaced));

    show_all_children();
}

template <typename B> template <typename P>
PanelDialog<B> *PanelDialog<B>::create()
{
    return _create<P>();
}

template <typename B> template <typename P>
PanelDialog<B> *PanelDialog<B>::_create()
{
    UI::Widget::Panel &panel = P::getInstance();
    return new PanelDialog<B>(panel, panel.getPrefsPath(), panel.getVerb());
}

template <typename B>
void PanelDialog<B>::present()
{
    _panel.present();
}

template <typename B>
void PanelDialog<B>::_presentDialog()
{
    Dialog::present();
}

template <> inline
void PanelDialog<Behavior::FloatingBehavior>::present()
{
    Dialog::present();
    _panel.present();
}

template <> inline
void PanelDialog<Behavior::FloatingBehavior>::_presentDialog()
{
}

/**
 * Specialized factory method for panel dialogs with floating behavior in order to make them work as
 * singletons, i.e. allow them track the current active desktop.
 */
template <>
template <typename P>
PanelDialog<Behavior::FloatingBehavior> *PanelDialog<Behavior::FloatingBehavior>::create()
{
    auto instance = _create<P>();

    INKSCAPE.signal_activate_desktop.connect(
            sigc::mem_fun(*instance, &PanelDialog<Behavior::FloatingBehavior>::_propagateDesktopActivated)
    );
    INKSCAPE.signal_deactivate_desktop.connect(        
            sigc::mem_fun(*instance, &PanelDialog<Behavior::FloatingBehavior>::_propagateDesktopDeactivated)
    );

    return instance;
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif //INKSCAPE_PANEL_DIALOG_H

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
