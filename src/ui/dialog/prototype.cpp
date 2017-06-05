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

#include "ui/dialog/prototype.h"
#include "verbs.h"
#include "desktop.h"
#include "document.h"
#include "selection.h"

// Only for use in demonstration widget.
#include "sp-root.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

Prototype::Prototype() :
    // UI::Widget::Panel("Prototype Label", "/dialogs/prototype", SP_VERB_DIALOG_PROTOTYPE,
    //                "Prototype Apply Label", true),
    UI::Widget::Panel("Prototype Label", "/dialogs/prototype", SP_VERB_DIALOG_PROTOTYPE),

    desktopTracker() //,
    // desktopChangedConnection()
{
    std::cout << "Prototype::Prototype()" << std::endl;

    // A widget for demonstration that displays the current SVG's id.
    _getContents()->pack_start(label);  // Panel::_getContents()

    // desktop is set by Panel constructor so this should never be NULL.
    // Note, we need to use getDesktop() since _desktop is private in Panel.h.
    // It should probably be protected instead... but need to verify in doesn't break anything.
    if (getDesktop() == NULL) {
        std::cerr << "Prototype::Prototype: desktop is NULL!" << std::endl;
    }

    connectionDesktopChanged = desktopTracker.connectDesktopChanged(
        sigc::mem_fun(*this, &Prototype::handleDesktopChanged) );
    desktopTracker.connect(GTK_WIDGET(gobj()));

    // This results in calling handleDocumentReplaced twice. Fix me!
    connectionDocumentReplaced = getDesktop()->connectDocumentReplaced(
        sigc::mem_fun(this, &Prototype::handleDocumentReplaced));

    // Alternative mechanism but results in calling handleDocumentReplaced four times.
    // signalDocumentReplaced().connect(
    //    sigc::mem_fun(this, &Prototype::handleDocumentReplaced));

    connectionSelectionChanged = getDesktop()->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &Prototype::handleSelectionChanged)));

    updateLabel();
}

Prototype::~Prototype()
{
    // Never actually called.
    std::cout << "Prototype::~Prototype()" << std::endl;
    connectionDesktopChanged.disconnect();
    connectionDocumentReplaced.disconnect();
    connectionSelectionChanged.disconnect();
}

/*
 * Called when a dialog is displayed, including when a dialog is reopened.
 * (When a dialog is closed, it is not destroyed so the contructor is not called.
 * This function can handle any reinitialization needed.)
 */
void
Prototype::present()
{
    std::cout << "Prototype::present()" << std::endl;
    UI::Widget::Panel::present();
}

/*
 * When Inkscape is first opened, a default document is shown. If another document is immediately
 * opened, it will replace the default document in the same desktop. This function handles the
 * change. Bug: This is called twice for some reason.
 */
void
Prototype::handleDocumentReplaced(SPDesktop *desktop, SPDocument * /* document */)
{
    std::cout << "Prototype::handleDocumentReplaced()" << std::endl;
    if (getDesktop() != desktop) {
        std::cerr << "Prototype::handleDocumentReplaced(): Error: panel desktop not equal to existing desktop!" << std::endl;
    }

    connectionSelectionChanged.disconnect();

    connectionSelectionChanged = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &Prototype::handleSelectionChanged)));

    // Update demonstration widget.
    updateLabel();
}

/*
 * When a dialog is floating, it is connected to the active desktop.
 */
void
Prototype::handleDesktopChanged(SPDesktop* desktop) {
    std::cout << "Prototype::handleDesktopChanged(): " << desktop << std::endl;

    if (getDesktop() == desktop) {
        // This will happen after construction of Prototype. We've already
        // set up signals so just return.
        std::cout << "  getDesktop() == desktop" << std::endl;
        return;
    }

    // Connections are disconnect safe.
    connectionSelectionChanged.disconnect();
    connectionDocumentReplaced.disconnect();

    setDesktop( desktop );

    connectionSelectionChanged = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &Prototype::handleSelectionChanged)));
    connectionDocumentReplaced = desktop->connectDocumentReplaced(
        sigc::mem_fun(this, &Prototype::handleDocumentReplaced));

    // Update demonstration widget.
    updateLabel();
}

/*
 * Handle a change in which objects are selected in a document.
 */
void
Prototype::handleSelectionChanged() {
    std::cout << "Prototype::handleSelectionChanged()" << std::endl;

    // Update demonstration widget.
    label.set_label("Selection Changed!");
}

/*
 * Update label... just a utility function for this example.
 */
void
Prototype::updateLabel() {

    const gchar* root_id = getDesktop()->getDocument()->getRoot()->getId();
    Glib::ustring label_string("Document's SVG id: ");
    label_string += (root_id?root_id:"null");
    label.set_label(label_string);
}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
