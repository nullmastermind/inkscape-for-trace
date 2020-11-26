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

#include "document.h"
#include "inkscape-application.h"
#include "verbs.h"

// Only for use in demonstration widget.
#include "object/sp-root.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

Prototype::Prototype()
    : DialogBase("/dialogs/prototype", SP_VERB_DIALOG_PROTOTYPE)
{
    // A widget for demonstration that displays the current SVG's id.
    _label = Gtk::manage(new Gtk::Label(_name));
    _label->set_line_wrap();

    _debug_button.set_name("PrototypeDebugButton");
    _debug_button.set_hexpand();
    _debug_button.signal_clicked().connect(sigc::mem_fun(*this, &Prototype::on_click));

    _debug_button.add(*_label);
    add(_debug_button);
}

void Prototype::update()
{
    if (!_app) {
        std::cerr << "Prototype::update(): _app is null" << std::endl;
        return;
    }

    handleDocumentReplaced(_app->get_active_document());
    handleSelectionChanged(_app->get_active_selection());
}

/*
 * When Inkscape is first opened, a default document is shown. If another document is immediately
 * opened, it will replace the default document in the same desktop. This function handles the
 * change. Bug: This is called twice for some reason.
 */
void Prototype::handleDocumentReplaced(SPDocument * document)
{
    if (!document || !document->getRoot()) {
        return;
    }

    const gchar *root_id = document->getRoot()->getId();
    Glib::ustring label_string("Document's SVG id: ");
    label_string += (root_id ? root_id : "null");
    _label->set_label(label_string);
}

/*
 * Handle a change in which objects are selected in a document.
 */
void Prototype::handleSelectionChanged(Inkscape::Selection *selection)
{
    if (!selection) {
        return;
    }

    // Update demonstration widget.
    Glib::ustring label = _label->get_text() + "\nSelection changed to ";
    SPObject* object = selection->single();
    if (object) {
        label = label + object->getId();
    } else {
        object = selection->activeContext();

        if (object) {
            label = label + object->getId();
        } else {
            label = label + "unknown";
        }
    }

    _label->set_label(label);
}

void Prototype::on_click()
{
    Gtk::Window *window = dynamic_cast<Gtk::Window *>(get_toplevel());
    if (window) {
        std::cout << "Dialog is part of: " << window->get_name() << "  (" << window->get_title() << ")" << std::endl;
    } else {
        std::cerr << "Prototype::on_click(): Dialog not attached to window!" << std::endl;
    }
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
