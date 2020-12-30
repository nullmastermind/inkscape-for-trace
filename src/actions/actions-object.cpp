// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for working with objects without GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-object.h"
#include "actions-helper.h"
#include "document-undo.h"
#include "inkscape-application.h"

#include "inkscape.h"             // Inkscape::Application
#include "selection.h"            // Selection
#include "path/path-simplify.h"


// No sanity checking is done... should probably add.
void
object_set_attribute(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", s.get());
    if (tokens.size() != 2) {
        std::cerr << "action:object_set_attribute: requires 'attribute name, attribute value'" << std::endl;
        return;
    }

    auto selection = app->get_active_selection();
    if (selection->isEmpty()) {
        std::cerr << "action:object_set_attribute: selection empty!" << std::endl;
        return;
    }

    // Should this be a selection member function?
    auto items = selection->items();
    for (auto i = items.begin(); i != items.end(); ++i) {
        Inkscape::XML::Node *repr = (*i)->getRepr();
        repr->setAttribute(tokens[0], tokens[1]);
    }

    // Needed to update repr (is this the best way?).
    Inkscape::DocumentUndo::done(app->get_active_document(), 0, "ActionObjectSetAttribute");
}


// No sanity checking is done... should probably add.
void
object_set_property(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", s.get());
    if (tokens.size() != 2) {
        std::cerr << "action:object_set_property: requires 'property name, property value'" << std::endl;
        return;
    }

    auto selection = app->get_active_selection();
    if (selection->isEmpty()) {
        std::cerr << "action:object_set_property: selection empty!" << std::endl;
        return;
    }

    // Should this be a selection member function?
    auto items = selection->items();
    for (auto i = items.begin(); i != items.end(); ++i) {
        Inkscape::XML::Node *repr = (*i)->getRepr();
        SPCSSAttr *css = sp_repr_css_attr(repr, "style");
        sp_repr_css_set_property(css, tokens[0].c_str(), tokens[1].c_str());
        sp_repr_css_set(repr, css, "style");
        sp_repr_css_attr_unref(css);
    }

    // Needed to update repr (is this the best way?).
    Inkscape::DocumentUndo::done(app->get_active_document(), 0, "ActionObjectSetProperty");
}


void
object_unlink_clones(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document  = app->get_active_document();
    selection->setDocument(document);

    selection->unlink();
}


void
object_to_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document  = app->get_active_document();
    selection->setDocument(document);

    selection->toCurves();  // TODO: Rename toPaths()
}


void
object_stroke_to_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document  = app->get_active_document();
    selection->setDocument(document);

    selection->strokesToPaths();
}


void
object_simplify_path(InkscapeApplication *app)
{
    auto selection = app->get_active_selection();

    // We should not have to do this!
    auto document  = app->get_active_document();
    selection->setDocument(document);

    selection->simplifyPaths();
}


std::vector<std::vector<Glib::ustring>> raw_data_object =
{
    // clang-format off
    {"app.object-set-attribute",      N_("Set Attribute"),         "Object",     N_("Set or update an attribute of selected objects; usage: object-set-attribute:attribute name, attribute value;")},
    {"app.object-set-property",       N_("Set Property"),          "Object",     N_("Set or update a property on selected objects; usage: object-set-property:property name, property value;")},
    {"app.object-unlink-clones",      N_("Unlink Clones"),         "Object",     N_("Unlink clones and symbols")                          },
    {"app.object-to-path",            N_("Object To Path"),        "Object",     N_("Convert shapes to paths")                            },
    {"app.object-stroke-to-path",     N_("Stroke to Path"),        "Object",     N_("Convert strokes to paths")                           },
    {"app.object-simplify-path",      N_("Simplify Path"),         "Object",     N_("Simplify paths, reducing node counts")               }
    // clang-format on
};

void
add_actions_object(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);

    auto *gapp = app->gio_app();

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)

    // clang-format off
    gapp->add_action_with_parameter( "object-set-attribute",     String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_set_attribute),      app));
    gapp->add_action_with_parameter( "object-set-property",      String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_set_property),       app));
    gapp->add_action(                "object-unlink-clones",             sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_unlink_clones),      app));
    gapp->add_action(                "object-to-path",                   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_to_path),            app));
    gapp->add_action(                "object-stroke-to-path",            sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_stroke_to_path),     app));
    gapp->add_action(                "object-simplify-path",             sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&object_simplify_path),      app));
    // clang-format on

#endif

    app->get_action_extra_data().add_data(raw_data_object);
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
