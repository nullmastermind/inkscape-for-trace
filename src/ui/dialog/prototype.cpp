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

#include "prototype.h"

#include "desktop.h"
#include "document.h"
#include "selection.h"
#include "verbs.h"

// Only for use in demonstration widget.
#include "object/sp-root.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

// Note that in order for a dialog to be restored, it must be listed in SPDesktop::show_dialogs().

Prototype::Prototype()
    : UI::Widget::Panel("/dialogs/prototype", SP_VERB_DIALOG_PROTOTYPE)

{
    std::cout << "Prototype::Prototype()" << std::endl;

    // A widget for demonstration that displays the current SVG's id.
    _getContents()->pack_start(label);  // Panel::_getContents()
}

Prototype::~Prototype()
{
    // Never actually called.
    std::cout << "Prototype::~Prototype()" << std::endl;

    // might not be necessary because we expect Panel::on_unmap being
    // called first and triggering setDesktop(nullptr)
    connectionDocumentReplaced.disconnect();
    connectionSelectionChanged.disconnect();
}

/*
 * Called when a dialog is displayed, including when a dialog is reopened.
 * (When a dialog is closed, it is not destroyed so the constructor is not called.
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

    if (!desktop)
        return;

    connectionSelectionChanged = desktop->getSelection()->connectChanged(
        sigc::hide(sigc::mem_fun(this, &Prototype::handleSelectionChanged)));

    // Update demonstration widget.
    updateLabel();
}

/*
 * When a dialog is floating, it is connected to the active desktop.
 */
void
Prototype::setDesktop(SPDesktop* desktop) {
    std::cout << "Prototype::handleDesktopChanged(): " << desktop << std::endl;

    // Connections are disconnect safe.
    connectionDocumentReplaced.disconnect();

    Panel::setDesktop(desktop);

    if (desktop) {
        connectionDocumentReplaced = desktop->connectDocumentReplaced(
        sigc::mem_fun(this, &Prototype::handleDocumentReplaced));
    }

    handleDocumentReplaced(desktop, nullptr);
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
