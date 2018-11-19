// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "selection.h"

// Helper function: returns true if both document and selection found. Maybe this should
// work on current view. Or better, application could return the selection of the current view.
bool
get_document_and_selection(InkscapeApplication* app, SPDocument** document, Inkscape::Selection** selection)
{
    *document = app->get_active_document();
    if (!(*document)) {
        std::cerr << "get_document_and_selection: No document!" << std::endl;
        return false;
    }

    // To do: get selection from active view (which could be from desktop or not).
    Inkscape::ActionContext context = INKSCAPE.action_context_for_document(*document);
    *selection = context.getSelection();
    if (!*selection) {
        std::cerr << "get_document_and_selection: No selection!" << std::endl;
        return false;
    }

    return true;
}

void
select_clear(InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    selection->clear();
}

void
select_via_id(Glib::ustring ids, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto tokens = Glib::Regex::split_simple("\\s*,\\s*", ids);
    for (auto id : tokens) {
        SPObject* object = document->getObjectById(id);
        if (object) {
            selection->add(object);
        } else {
            std::cerr << "select_via_id: Did not find object with id: " << id << std::endl;
        }
    }
}

void
select_via_class(Glib::ustring klass, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectByClass(klass);
    selection->add(objects.start(), objects.end());
}

void
select_via_element(Glib::ustring element, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectByElement(element);
    selection->add(objects.start(), objects.end());
}

void
select_via_selector(Glib::ustring selector, InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    auto objects = document->getObjectBySelector(selector);
    selection->add(objects.start(), objects.end());
}

void
select_all(Inkscape* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
}

void
add_actions_selection(InkscapeApplication* app)
{
    app->add_action(               "select-clear",       sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&selecton_clear),            app)        );
    app->add_action_radio_string(  "select-via-id",      sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_via_id),             app), "null");
    app->add_action_radio_string(  "select-via-class",   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_via_id),             app), "null");
    app->add_action_radio_string(  "select-via-element", sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_via_element),        app), "null");
    app->add_action_radio_string(  "select-via-selector",sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&select_via_selector),       app), "null");
}




/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
