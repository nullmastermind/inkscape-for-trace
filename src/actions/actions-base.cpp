// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions tied to the application and independent of GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-base.h"
#include "actions-helper.h"

#include "inkscape-application.h"

#include "inkscape.h"             // Inkscape::Application
#include "inkscape-version.h"     // Inkscape version
#include "path-prefix.h"          // Extension directory
#include "extension/init.h"       // List verbs
#include "verbs.h"                // List verbs
#include "selection.h"            // Selection
#include "object/sp-root.h"       // query_all()
#include "file.h"                 // dpi convert method
#include "io/resource.h"

void
print_inkscape_version()
{
    std::cout << "Inkscape " << Inkscape::version_string << std::endl;
    std::cerr << "    Pango version: " << pango_version_string() << std::endl;
}

void
print_system_data_directory()
{
    std::cout << INKSCAPE_SYSTEMDIR << std::endl;
}

void
print_user_data_directory()
{
    std::cout << Inkscape::IO::Resource::profile_path("") << std::endl;
}

void
print_verb_list()
{
    // This really shouldn't go here, we should init the app.
    // But, since we're just exiting in this path, there is
    // no harm, and this is really a better place to put
    // everything else.
    Inkscape::Extension::init();  // extension/init.h
    Inkscape::Verb::list();       // verbs.h
}

// Helper function for query_x(), query_y(), query_width(), and query_height().
void
query_dimension(InkscapeApplication* app, bool extent, Geom::Dim2 const axis)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }

    if (selection->isEmpty()) {
        selection->add(document->getRoot());
    }

    bool first = true;
    auto items = selection->items();
    for (auto item : items) {
        if (!first) {
            std::cout << ",";
        }
        first = false;
        Geom::OptRect area = item->documentVisualBounds();
        if (area) {
            if (extent) {
                std::cout << area->dimensions()[axis];
            } else {
                std::cout << area->min()[axis];
            }
        } else {
            std::cout << "0";
        }
    }
    std::cout << std::endl;
}

void
query_x(InkscapeApplication* app)
{
    query_dimension(app, false, Geom::X);
}

void
query_y(InkscapeApplication* app)
{
    query_dimension(app, false, Geom::Y);
}

void
query_width(InkscapeApplication* app)
{
    query_dimension(app, true, Geom::X);
}

void
query_height(InkscapeApplication* app)
{
    query_dimension(app, true, Geom::Y);
}

// Helper for query_all()
void
query_all_recurse (SPObject *o)
{
    SPItem *item = dynamic_cast<SPItem*>(o);
    if (item && item->getId()) {
        Geom::OptRect area = item->documentVisualBounds();
        if (area) {
            std::cout << item->getId()               << ","
                      << area->min()[Geom::X]        << ","
                      << area->min()[Geom::Y]        << ","
                      << area->dimensions()[Geom::X] << ","
                      << area->dimensions()[Geom::Y] << std::endl;
        }

        for (auto& child: o->children) {
            query_all_recurse (&child);
        }
    }
}

void
query_all(InkscapeApplication* app)
{
    SPDocument* doc = app->get_active_document();
    if (!doc) {
        std::cerr << "query_all: no document!" << std::endl;
        return;
    }

    SPObject *o = doc->getRoot();
    if (o) {
        query_all_recurse(o);
    }
}

void
pdf_page(int page)
{
    INKSCAPE.set_pdf_page(page);
}

void
convert_dpi_method(Glib::ustring method)
{
    if (method == "none") {
        sp_file_convert_dpi_method_commandline = FILE_DPI_UNCHANGED;
    } else if (method == "scale-viewbox") {
        sp_file_convert_dpi_method_commandline = FILE_DPI_VIEWBOX_SCALED;
    } else if (method == "scale-document") {
        sp_file_convert_dpi_method_commandline = FILE_DPI_DOCUMENT_SCALED;
    } else {
        std::cerr << "dpi_convert_method: invalid option" << std::endl;
    }
}

void
no_convert_baseline()
{
    sp_no_convert_text_baseline_spacing = true;
}

// Temporary: Verbs are to be replaced by Gio::Actions!
void
verbs(Glib::ustring verblist, InkscapeApplication* app)
{
    auto tokens = Glib::Regex::split_simple("\\s*;\\s*", verblist);
    for (auto token : tokens) {
        std::vector<Glib::ustring> parts = Glib::Regex::split_simple("\\s*:\\s*", token); // Second part is always ignored... we could implement it but better to switch to Gio::Actions
        if (!parts.empty() && !parts[0].empty()) {
            Inkscape::Verb* verb = Inkscape::Verb::getbyid(parts[0].c_str());
            if (verb == nullptr) {
                std::cerr << "verbs_action: Invalid verb: " << parts[0] << std::endl;
                break;
            }
            // Inkscape::ActionContext context = INKSCAPE.action_context_for_document(*document);
            SPAction* action = verb->get_action(INKSCAPE.active_action_context());
            sp_action_perform(action, nullptr);  // Data is ignored!
        }
    }
}

void
vacuum_defs(InkscapeApplication* app)
{
    SPDocument* document = nullptr;
    Inkscape::Selection* selection = nullptr;
    if (!get_document_and_selection(app, &document, &selection)) {
        return;
    }
    document->vacuumDocument();
}

void
quit_inkscape(InkscapeApplication* app)
{
    app->on_quit();
}

std::vector<std::vector<Glib::ustring>> raw_data_base =
{
    {"inkscape-version",          "InkscapeVersion",         "Base",       N_("Print Inkscape version and exit.")                   },
    {"system-data-directory",     "InkscapeSystemDir",       "Base",       N_("Print system data directory and exit.")              },
    {"user-data-directory",       "InkscapeUserDir",         "Base",       N_("Print user data directory and exit.")                },
    {"action-list",               "InkscapeActions",         "Base",       N_("Print a list of actions and exit.")                  },
    {"verb-list",                 "InkscapeVerbs",           "Base",       N_("Print a list of verbs and exit.")                    },
    {"verb",                      "Verb",                    "Base",       N_("Execute verb(s).")                                   },
    {"vacuum-defs",               "VacuumDefs",              "Base",       N_("Remove unused definitions (gradients, etc.).")       },
    {"quit-inkscape",             "QuitInkscape",            "Base",       N_("Immediately quit Inkscape.")                         },

    {"open-page",                 "ImportPageNumber",        "Import",     N_("Import page number.")                                },
    {"convert-dpi-method",        "ImportDPIMethod",         "Import",     N_("Import DPI convert method.")                         },
    {"no-convert-baseline",       "ImportBaselineConvert",   "Import",     N_("Import convert text baselines.")                     },

    {"query-x",                   "QueryX",                  "Query",      N_("Query 'x' value(s) of selected objects.")            },
    {"query-y",                   "QueryY",                  "Query",      N_("Query 'y' value(s) of selected objects.")            },
    {"query-width",               "QueryWidth",              "Query",      N_("Query 'width' value(s) of object(s).")               },
    {"query-height",              "QueryHeight",             "Query",      N_("Query 'height' value(s) of object(s).")              },
    {"query-all",                 "QueryAll",                "Query",      N_("Query 'x', 'y', 'width', and 'height'.")             }
};

template<class T>
void
add_actions_base(ConcreteInkscapeApplication<T>* app)
{
    // Note: "radio" actions are just an easy way to set type without using templating.
    app->add_action(               "inkscape-version",                                    sigc::ptr_fun(&print_inkscape_version)                 );
    app->add_action(               "system-data-directory",                               sigc::ptr_fun(&print_system_data_directory)            );
    app->add_action(               "user-data-directory",                                 sigc::ptr_fun(&print_user_data_directory)              );
    app->add_action(               "action-list",       sigc::mem_fun(app, &ConcreteInkscapeApplication<T>::print_action_list)                    );
    app->add_action(               "verb-list",                                           sigc::ptr_fun(&print_verb_list)                        );
    app->add_action_radio_string(  "verb",               sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&verbs),                     app), "null");
    app->add_action(               "vacuum-defs",        sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&vacuum_defs),               app)        );
    app->add_action(               "quit-inkscape",      sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&quit_inkscape),             app)        );

    app->add_action_radio_integer( "open-page",                                           sigc::ptr_fun(&pdf_page),                             0);
    app->add_action_radio_string(  "convert-dpi-method",                                  sigc::ptr_fun(&convert_dpi_method),              "none");
    app->add_action(               "no-convert-baseline",                                 sigc::ptr_fun(&no_convert_baseline)                    );


    app->add_action(               "query-x",            sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&query_x),                   app)        );
    app->add_action(               "query-y",            sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&query_y),                   app)        );
    app->add_action(               "query-width",        sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&query_width),               app)        );
    app->add_action(               "query-height",       sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&query_height),              app)        );
    app->add_action(               "query-all",          sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&query_all),                 app)        );

    app->get_action_extra_data().add_data(raw_data_base);
}

template void add_actions_base(ConcreteInkscapeApplication<Gio::Application>* app);
template void add_actions_base(ConcreteInkscapeApplication<Gtk::Application>* app);

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
