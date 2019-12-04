// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape UI action implementation
 *//*
 * Authors:
 * see git history
 * Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "helper/action.h"
#include "ui/icon-loader.h"

#include <gtkmm/toolbutton.h>

#include "debug/logger.h"
#include "debug/timestamp.h"
#include "debug/simple-event.h"
#include "debug/event-tracker.h"
#include "ui/view/view.h"
#include "desktop.h"
#include "document.h"
#include "verbs.h"

static void sp_action_finalize (GObject *object);

G_DEFINE_TYPE(SPAction, sp_action, G_TYPE_OBJECT);

/**
 * SPAction vtable initialization.
 */
static void
sp_action_class_init (SPActionClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    object_class->finalize = sp_action_finalize;
}

/**
 * Callback for SPAction object initialization.
 */
static void
sp_action_init (SPAction *action)
{
    action->sensitive = 0;
    action->active = 0;
    action->context = Inkscape::ActionContext();
    action->id = action->name = action->tip = nullptr;
    action->image = nullptr;
    
    new (&action->signal_perform) sigc::signal<void>();
    new (&action->signal_set_sensitive) sigc::signal<void, bool>();
    new (&action->signal_set_active) sigc::signal<void, bool>();
    new (&action->signal_set_name) sigc::signal<void, Glib::ustring const &>();
}

/**
 * Called before SPAction object destruction.
 */
static void
sp_action_finalize (GObject *object)
{
    SPAction *action = SP_ACTION(object);

    g_free (action->image);
    g_free (action->tip);
    g_free (action->name);
    g_free (action->id);

    action->signal_perform.~signal();
    action->signal_set_sensitive.~signal();
    action->signal_set_active.~signal();
    action->signal_set_name.~signal();

    G_OBJECT_CLASS(sp_action_parent_class)->finalize (object);
}

/**
 * Create new SPAction object and set its properties.
 */
SPAction *
sp_action_new(Inkscape::ActionContext const &context,
              const gchar *id,
              const gchar *name,
              const gchar *tip,
              const gchar *image,
              Inkscape::Verb * verb)
{
    SPAction *action = (SPAction *)g_object_new(SP_TYPE_ACTION, nullptr);

    action->context = context;
    action->sensitive = TRUE;
    action->id = g_strdup (id);
    action->name = g_strdup (name);
    action->tip = g_strdup (tip);
    action->image = g_strdup (image);
    action->verb = verb;

    return action;
}

namespace {

using Inkscape::Debug::SimpleEvent;
using Inkscape::Debug::Event;
using Inkscape::Debug::timestamp;

typedef SimpleEvent<Event::INTERACTION> ActionEventBase;

class ActionEvent : public ActionEventBase {
public:
    ActionEvent(SPAction const *action)
    : ActionEventBase("action")
    {
        _addProperty("timestamp", timestamp());
        SPDocument *document = action->context.getDocument();
        if (document) {
            _addProperty("document", document->serial());
        }
        _addProperty("verb", action->id);
    }
};

}

/**
 * Executes an action.
 * @param action   The action to be executed.
 * @param data     ignored.
 */
void sp_action_perform(SPAction *action, void * /*data*/)
{
    g_return_if_fail (action != nullptr);
    g_return_if_fail (SP_IS_ACTION (action));

    Inkscape::Debug::EventTracker<ActionEvent> tracker(action);
    action->signal_perform.emit();
}

/**
 * Change activation in all actions that can be taken with the action.
 */
void
sp_action_set_active (SPAction *action, unsigned int active)
{
    g_return_if_fail (action != nullptr);
    g_return_if_fail (SP_IS_ACTION (action));

    action->signal_set_active.emit(active);
}

/**
 * Change sensitivity in all actions that can be taken with the action.
 */
void
sp_action_set_sensitive (SPAction *action, unsigned int sensitive)
{
    g_return_if_fail (action != nullptr);
    g_return_if_fail (SP_IS_ACTION (action));

    action->signal_set_sensitive.emit(sensitive);
}

void
sp_action_set_name (SPAction *action, Glib::ustring const &name)
{
    g_return_if_fail (action != nullptr);
    g_return_if_fail (SP_IS_ACTION (action));

    g_free(action->name);
    action->name = g_strdup(name.data());
    action->signal_set_name.emit(name);
}

/**
 * Return Document associated with the action.
 */
SPDocument *
sp_action_get_document (SPAction *action)
{
    g_return_val_if_fail (SP_IS_ACTION (action), NULL);
    return action->context.getDocument();
}

/**
 * Return Selection associated with the action
 */
Inkscape::Selection *
sp_action_get_selection (SPAction *action)
{
    g_return_val_if_fail (SP_IS_ACTION (action), NULL);
    return action->context.getSelection();
}

/**
 * Return View associated with the action, if any.
 */
Inkscape::UI::View::View *
sp_action_get_view (SPAction *action)
{
    g_return_val_if_fail (SP_IS_ACTION (action), NULL);
    return action->context.getView();
}

/**
 * Return Desktop associated with the action, if any.
 */
SPDesktop *
sp_action_get_desktop (SPAction *action)
{
    // TODO: this slightly horrible storage of a UI::View::View*, and 
    // casting to an SPDesktop*, is only done because that's what was
    // already the norm in the Inkscape codebase. This seems wrong. Surely
    // we should store an SPDesktop* in the first place? Is there a case
    // of actions being carried out on a View that is not an SPDesktop?
      return static_cast<SPDesktop *>(sp_action_get_view(action));
}

/**
 * \brief Create a toolbutton whose "clicked" signal performs an Inkscape verb
 *
 * \param[in] verb_code The code (e.g., SP_VERB_EDIT_SELECT_ALL) for the verb we want
 *
 * \todo This should really attach the toolbutton to an application action instead of
 *       hooking up the "clicked" signal.  This should probably wait until we've
 *       migrated to Gtk::Application
 */
Gtk::ToolButton *
SPAction::create_toolbutton_for_verb(unsigned int             verb_code,
                                     Inkscape::ActionContext &context)
{
    // Get display properties for the verb
    auto verb = Inkscape::Verb::get(verb_code);
    auto target_action = verb->get_action(context);
    auto icon_name = verb->get_image() ? verb->get_image() : Glib::ustring();

    // Create a button with the required display properties
    auto button = Gtk::manage(new Gtk::ToolButton(verb->get_tip()));
    auto icon_widget = sp_get_icon_image(icon_name, "/toolbox/small");
    button->set_icon_widget(*icon_widget);
    button->set_tooltip_text(verb->get_tip());

    // Hook up signal handler
    auto button_clicked_cb = sigc::bind(sigc::ptr_fun(&sp_action_perform),
                                        target_action, nullptr);
    button->signal_clicked().connect(button_clicked_cb);

    return button;
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
