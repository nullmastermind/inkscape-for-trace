// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "effect.h"

#include "execution-env.h"
#include "inkscape.h"
#include "timer.h"

#include "helper/action.h"
#include "implementation/implementation.h"
#include "prefdialog/prefdialog.h"
#include "ui/view/view.h"



/* Inkscape::Extension::Effect */

namespace Inkscape {
namespace Extension {

Effect * Effect::_last_effect = nullptr;
Inkscape::XML::Node * Effect::_effects_list = nullptr;
Inkscape::XML::Node * Effect::_filters_list = nullptr;

#define  EFFECTS_LIST  "effects-list"
#define  FILTERS_LIST  "filters-list"

Effect::Effect (Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory)
    : Extension(in_repr, in_imp, base_directory)
    , _id_noprefs(Glib::ustring(get_id()) + ".noprefs")
    , _name_noprefs(Glib::ustring(_(get_name())) + _(" (No preferences)"))
    , _verb(get_id(), get_name(), nullptr, nullptr, this, true)
    , _verb_nopref(_id_noprefs.c_str(), _name_noprefs.c_str(), nullptr, nullptr, this, false)
    , _menu_node(nullptr), _workingDialog(true)
    , _prefDialog(nullptr)
{
    Inkscape::XML::Node * local_effects_menu = nullptr;

    // This is a weird hack
    if (!strcmp(this->get_id(), "org.inkscape.filter.dropshadow"))
        return;

    bool hidden = false;

    no_doc = false;
    no_live_preview = false;

    if (repr != nullptr) {

        for (Inkscape::XML::Node *child = repr->firstChild(); child != nullptr; child = child->next()) {
            if (!strcmp(child->name(), INKSCAPE_EXTENSION_NS "effect")) {
                if (child->attribute("needs-document") && !strcmp(child->attribute("needs-document"), "false")) {
                    no_doc = true;
                }
                if (child->attribute("needs-live-preview") && !strcmp(child->attribute("needs-live-preview"), "false")) {
                    no_live_preview = true;
                }
                if (child->attribute("implements-custom-gui") && !strcmp(child->attribute("implements-custom-gui"), "true")) {
                    _workingDialog = false;
                }
                for (Inkscape::XML::Node *effect_child = child->firstChild(); effect_child != nullptr; effect_child = effect_child->next()) {
                    if (!strcmp(effect_child->name(), INKSCAPE_EXTENSION_NS "effects-menu")) {
                        // printf("Found local effects menu in %s\n", this->get_name());
                        local_effects_menu = effect_child->firstChild();
                        if (effect_child->attribute("hidden") && !strcmp(effect_child->attribute("hidden"), "true")) {
                            hidden = true;
                        }
                    }
                    if (!strcmp(effect_child->name(), INKSCAPE_EXTENSION_NS "menu-name") ||
                            !strcmp(effect_child->name(), INKSCAPE_EXTENSION_NS "_menu-name")) {
                        // printf("Found local effects menu in %s\n", this->get_name());
                        _verb.set_name(effect_child->firstChild()->content());
                    }
                    if (!strcmp(effect_child->name(), INKSCAPE_EXTENSION_NS "menu-tip") ||
                            !strcmp(effect_child->name(), INKSCAPE_EXTENSION_NS "_menu-tip")) {
                        // printf("Found local effects menu in %s\n", this->get_name());
                        _verb.set_tip(effect_child->firstChild()->content());
                    }
                } // children of "effect"
                break; // there can only be one effect
            } // find "effect"
        } // children of "inkscape-extension"
    } // if we have an XML file

    // \TODO this gets called from the Inkscape::Application constructor, where it initializes the menus.
    // But in the constructor, our object isn't quite there yet!
    if (Inkscape::Application::exists() && INKSCAPE.use_gui()) {
        if (_effects_list == nullptr)
            _effects_list = find_menu(INKSCAPE.get_menus(), EFFECTS_LIST);
        if (_filters_list == nullptr)
            _filters_list = find_menu(INKSCAPE.get_menus(), FILTERS_LIST);
    }

    if ((_effects_list != nullptr || _filters_list != nullptr)) {
        Inkscape::XML::Document *xml_doc;
        xml_doc = _effects_list->document();
        _menu_node = xml_doc->createElement("verb");
        _menu_node->setAttribute("verb-id", this->get_id());

        if (!hidden) {
            if (_filters_list &&
                local_effects_menu &&
                local_effects_menu->attribute("name") &&
                !strcmp(local_effects_menu->attribute("name"), ("Filters"))) {
                merge_menu(_filters_list->parent(), _filters_list, local_effects_menu->firstChild(), _menu_node);
            } else if (_effects_list) {
                merge_menu(_effects_list->parent(), _effects_list, local_effects_menu, _menu_node);
            }
        }
    }

    return;
}

void
Effect::merge_menu (Inkscape::XML::Node * base,
                    Inkscape::XML::Node * start,
                    Inkscape::XML::Node * pattern,
                    Inkscape::XML::Node * merge) {
    Glib::ustring mergename;
    Inkscape::XML::Node * tomerge = nullptr;
    Inkscape::XML::Node * submenu = nullptr;

    if (pattern == nullptr) {
        // Merge the verb name
        tomerge = merge;
        mergename = get_translation(get_name());
    } else {
        gchar const *menuname = pattern->attribute("name");
        if (menuname == nullptr) menuname = pattern->attribute("_name");
        if (menuname == nullptr) return;

        Inkscape::XML::Document *xml_doc;
        xml_doc = base->document();
        tomerge = xml_doc->createElement("submenu");
        if (_translation_enabled) {
            mergename = get_translation(menuname);
        } else {
            // Even if the extension author requested the extension not to be translated,
            // it still seems desirable to be able to put the extension into the existing (translated) submenus.
            mergename = _(menuname);
        }
        tomerge->setAttribute("name", mergename);
    }

    int position = -1;

    if (start != nullptr) {
        Inkscape::XML::Node * menupass;
        for (menupass = start; menupass != nullptr && strcmp(menupass->name(), "separator"); menupass = menupass->next()) {
            gchar const * compare_char = nullptr;
            if (!strcmp(menupass->name(), "verb")) {
                gchar const * verbid = menupass->attribute("verb-id");
                Inkscape::Verb * verb = Inkscape::Verb::getbyid(verbid);
                if (verb == nullptr) {
					g_warning("Unable to find verb '%s' which is referred to in the menus.", verbid);
                    continue;
                }
                compare_char = verb->get_name();
            } else if (!strcmp(menupass->name(), "submenu")) {
                compare_char = menupass->attribute("name");
                if (compare_char == nullptr)
                    compare_char = menupass->attribute("_name");
            }

            position = menupass->position() + 1;

            /* This will cause us to skip tags we don't understand */
            if (compare_char == nullptr) {
                continue;
            }

            Glib::ustring compare(_(compare_char));

            if (mergename == compare) {
                Inkscape::GC::release(tomerge);
                tomerge = nullptr;
                submenu = menupass;
                break;
            }

            if (mergename < compare) {
                position = menupass->position();
                break;
            }
        } // for menu items
    } // start != NULL

    if (tomerge != nullptr) {
        if (position != -1) {
            base->addChildAtPos(tomerge, position);
        } else {
            base->appendChild(tomerge);
        }
        Inkscape::GC::release(tomerge);
    }

    if (pattern != nullptr) {
        if (submenu == nullptr)
            submenu = tomerge;
        merge_menu(submenu, submenu->firstChild(), pattern->firstChild(), merge);
    }

    return;
}

Effect::~Effect ()
{
    if (get_last_effect() == this)
        set_last_effect(nullptr);
    if (_menu_node)
        Inkscape::GC::release(_menu_node);
    return;
}

bool
Effect::check ()
{
    if (!Extension::check()) {
        _verb.sensitive(nullptr, false);
        _verb.set_tip(Extension::getErrorReason().c_str()); // TODO: insensitive menuitems don't show a tooltip
        return false;
    }
    return true;
}

bool
Effect::prefs (Inkscape::UI::View::View * doc)
{
    if (_prefDialog != nullptr) {
        _prefDialog->raise();
        return true;
    }

    if (widget_visible_count() == 0) {
        effect(doc);
        return true;
    }

    if (!loaded())
        set_state(Extension::STATE_LOADED);
    if (!loaded()) return false;

    Glib::ustring name = get_translation(this->get_name());
    _prefDialog = new PrefDialog(name, nullptr, this);
    _prefDialog->show();

    return true;
}

/**
    \brief  The function that 'does' the effect itself
    \param  doc  The Inkscape::UI::View::View to do the effect on

    This function first insures that the extension is loaded, and if not,
    loads it.  It then calls the implementation to do the actual work.  It
    also resets the last effect pointer to be this effect.  Finally, it
    executes a \c SPDocumentUndo::done to commit the changes to the undo
    stack.
*/
void
Effect::effect (Inkscape::UI::View::View * doc)
{
    //printf("Execute effect\n");
    if (!loaded())
        set_state(Extension::STATE_LOADED);
    if (!loaded()) return;
    ExecutionEnv executionEnv(this, doc, nullptr, _workingDialog, true);
    execution_env = &executionEnv;
    timer->lock();
    executionEnv.run();
    if (executionEnv.wait()) {
        executionEnv.commit();
    } else {
        executionEnv.cancel();
    }
    timer->unlock();

    return;
}

/** \brief  Sets which effect was called last
    \param in_effect  The effect that has been called

    This function sets the static variable \c _last_effect and it
    ensures that the last effect verb is sensitive.

    If the \c in_effect variable is \c NULL then the last effect
    verb is made insesitive.
*/
void
Effect::set_last_effect (Effect * in_effect)
{
    if (in_effect == nullptr) {
        Inkscape::Verb::get(SP_VERB_EFFECT_LAST)->sensitive(nullptr, false);
        Inkscape::Verb::get(SP_VERB_EFFECT_LAST_PREF)->sensitive(nullptr, false);
    } else if (_last_effect == nullptr) {
        Inkscape::Verb::get(SP_VERB_EFFECT_LAST)->sensitive(nullptr, true);
        Inkscape::Verb::get(SP_VERB_EFFECT_LAST_PREF)->sensitive(nullptr, true);
    }

    _last_effect = in_effect;
    return;
}

Inkscape::XML::Node *
Effect::find_menu (Inkscape::XML::Node * menustruct, const gchar *name)
{
    if (menustruct == nullptr) return nullptr;
    for (Inkscape::XML::Node * child = menustruct;
            child != nullptr;
            child = child->next()) {
        if (!strcmp(child->name(), name)) {
            return child;
        }
        Inkscape::XML::Node * firstchild = child->firstChild();
        if (firstchild != nullptr) {
            Inkscape::XML::Node *found = find_menu (firstchild, name);
            if (found)
                return found;
        }
    }
    return nullptr;
}


Gtk::VBox *
Effect::get_info_widget()
{
    return Extension::get_info_widget();
}

PrefDialog *
Effect::get_pref_dialog ()
{
    return _prefDialog;
}

void
Effect::set_pref_dialog (PrefDialog * prefdialog)
{
    _prefDialog = prefdialog;
    return;
}

SPAction *
Effect::EffectVerb::make_action (Inkscape::ActionContext const & context)
{
    return make_action_helper(context, &perform, static_cast<void *>(this));
}

/** \brief  Decode the verb code and take appropriate action */
void
Effect::EffectVerb::perform( SPAction *action, void * data )
{
    g_return_if_fail(ensure_desktop_valid(action));
    Inkscape::UI::View::View * current_view = sp_action_get_view(action);

    Effect::EffectVerb * ev = reinterpret_cast<Effect::EffectVerb *>(data);
    Effect * effect = ev->_effect;

    if (effect == nullptr) return;

    if (ev->_showPrefs) {
        effect->prefs(current_view);
    } else {
        effect->effect(current_view);
    }

    return;
}

} }  /* namespace Inkscape, Extension */

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
