// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for output tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-output.h"
#include "actions-helper.h"
#include "inkscape-application.h"

#include "inkscape.h"             // Inkscape::Application

// Actions for command line output (should be integrated with file dialog).

// These actions are currently stateless and result in changes to an instance of the
// InkFileExportCmd class owned by the application.

void
export_type(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    app->file_export()->export_type = s.get();
    // std::cout << "export-type: " << s.get() << std::endl;
}

void
export_filename(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_filename = s.get();
    // std::cout << "export-filename: " << s.get() << std::endl;
}

void
export_overwrite(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_overwrite = b.get();
    // std::cout << "export-overwrite: " << std::boolalpha << b.get() << std::endl;
}

void
export_area(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_area = s.get();
    // std::cout << "export-area: " << s.get() << std::endl;
}

void
export_area_drawing(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_area_drawing = b.get();
    // std::cout << "export-area-drawing: " << std::boolalpha << b.get() << std::endl;
}

void
export_area_page(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_area_page = b.get();
    // std::cout << "export-area-page: " << std::boolalpha << b.get() << std::endl;
}

void
export_margin(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_margin = i.get();
    // std::cout << "export-margin: " << i.get() << std::endl;
}

void
export_area_snap(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_area_snap = b.get();
    // std::cout << "export-area-snap: " << std::boolalpha << b.get() << std::endl;
}

void
export_width(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_width = i.get();
    // std::cout << "export-width: " << i.get() << std::endl;
}

void
export_height(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_height = i.get();
    // std::cout << "export-height: " << i.get() << std::endl;
}

void
export_id(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_id = s.get();
    // std::cout << "export-id: " << s.get() << std::endl;
}

void
export_id_only(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_id_only = b.get();
    // std::cout << "export-id-only: " << std::boolalpha << b.get() << std::endl;
}

void
export_plain_svg(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_plain_svg = b.get();
    // std::cout << "export-plain-svg: " << std::boolalpha << b.get() << std::endl;
}

void
export_dpi(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_dpi = i.get();
    // std::cout << "export-dpi: " << i.get() << std::endl;
}

void
export_ignore_filters(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_ignore_filters = b.get();
    // std::cout << "export-ignore-filters: " << std::boolalpha << b.get() << std::endl;
}

void
export_text_to_path(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_text_to_path = b.get();
    // std::cout << "export-text-to-path: " << std::boolalpha << b.get() << std::endl;
}

void
export_ps_level(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_ps_level = i.get();
    // std::cout << "export-dpi: " << i.get() << std::endl;
}

void
export_pdf_level(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    app->file_export()->export_pdf_level = s.get();
    // std::cout << "export-pdf-level" << s.get() << std::endl;
}

void
export_latex(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_latex = b.get();
    // std::cout << "export-latex: " << std::boolalpha << b.get() << std::endl;
}

void
export_use_hints(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_use_hints = b.get();
    // std::cout << "export-use-hints: " << std::boolalpha << b.get() << std::endl;
}

void
export_background(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_background = s.get();
    // std::cout << "export-background: " << s.get() << std::endl;
}

void
export_background_opacity(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    app->file_export()->export_background_opacity = d.get();
    // std::cout << d.get() << std::endl;
}

void
export_do(InkscapeApplication *app)
{
    SPDocument* document = app->get_active_document();
    std::string filename;
    if (document->getDocumentURI()) {
        filename = document->getDocumentURI();
    }
    app->file_export()->do_export(document, filename);
}

std::vector<std::vector<Glib::ustring>> raw_data_output =
{
   {"export-type",               "ExportType",              "Export",     N_("Export file type.")                                  },
   {"export-filename",           "ExportFileName",          "Export",     N_("Export file name.")                                  },
   {"export-overwrite",          "ExportOverWrite",         "Export",     N_("Export over-write file.")                            },

   {"export-area",               "ExportArea",              "Export",     N_("Export area.")                                       },
   {"export-area-drawing",       "ExportAreaDrawing",       "Export",     N_("Export drawing area.")                               },
   {"export-area-page",          "ExportAreaPage",          "Export",     N_("Export page area.")                                  },
   {"export-margin",             "ExportMargin",            "Export",     N_("Export margin.")                                     },
   {"export-area-snap",          "ExportAreaSnap",          "Export",     N_("Export snap area to integer values.")                },
   {"export-width",              "ExportWidth",             "Export",     N_("Export width.")                                      },
   {"export-height",             "ExportHeight",            "Export",     N_("Export height.")                                     },

   {"export-id",                 "ExportID",                "Export",     N_("Export id(s).")                                      },
   {"export-id-only",            "ExportIDOnly",            "Export",     N_("Export id(s) only.")                                 },

   {"export-plain-svg",          "ExportPlanSVG",           "Export",     N_("Export as plain SVG.")                               },
   {"export-dpi",                "ExportDPI",               "Export",     N_("Export DPI.")                                        },
   {"export-ignore-filters",     "ExportIgnoreFilters",     "Export",     N_("Export ignore filters.")                             },
   {"export-text-to-path",       "ExportTextToPath",        "Export",     N_("Export convert text to paths.")                      },
   {"export-ps-level",           "ExportPSLevel",           "Export",     N_("Export PostScript level.")                           },
   {"export-pdf-version",        "ExportPSVersion",         "Export",     N_("Export PDF version.")                                },
   {"export-latex",              "ExportLaTeX",             "Export",     N_("Export LaTeX.")                                      },
   {"export-use-hints",          "ExportUseHInts",          "Export",     N_("Export using saved hints.")                          },
   {"export-background",         "ExportBackground",        "Export",     N_("Export background color.")                           },
   {"export-background-opacity", "ExportBackgroundOpacity", "Export",     N_("Export background opacity.")                         },

   {"export-do",                 "ExportDo",                "Export",     N_("Do export.")                                         }
};

template <class T>
void
add_actions_output(ConcreteInkscapeApplication<T>* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);
    Glib::VariantType BString(Glib::VARIANT_TYPE_BYTESTRING);

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)

    // Matches command line options
    app->add_action_with_parameter( "export-type",              String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_type),         app));
    app->add_action_with_parameter( "export-filename",          String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_filename),     app)); // MAY NOT WORK DUE TO std::string
    app->add_action_with_parameter( "export-overwrite",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_overwrite),    app));

    app->add_action_with_parameter( "export-area",              String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area),         app));
    app->add_action_with_parameter( "export-area-drawing",      Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area_drawing), app));
    app->add_action_with_parameter( "export-area-page",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area_page),    app));
    app->add_action_with_parameter( "export-margin",            Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_margin),       app));
    app->add_action_with_parameter( "export-area-snap",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area_snap),    app));
    app->add_action_with_parameter( "export-width",             Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_width),        app));
    app->add_action_with_parameter( "export-height",            Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_height),       app));

    app->add_action_with_parameter( "export-id",                String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_id),           app));
    app->add_action_with_parameter( "export-id-only",           Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_id_only),      app));

    app->add_action_with_parameter( "export-plain-svg",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_plain_svg),    app));
    app->add_action_with_parameter( "export-dpi",               Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_dpi),          app));
    app->add_action_with_parameter( "export-ignore-filters",    Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_plain_svg),    app));
    app->add_action_with_parameter( "export-text-to-path",      Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_text_to_path), app));
    app->add_action_with_parameter( "export-ps-level",          Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_ps_level),     app));
    app->add_action_with_parameter( "export-pdf-version",       String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_pdf_level),    app));
    app->add_action_with_parameter( "export-latex",             Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_latex),        app));
    app->add_action_with_parameter( "export-use-hints",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_use_hints),    app));
    app->add_action_with_parameter( "export-background",        String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_background),   app));
    app->add_action_with_parameter( "export-background-opacity",Double, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_background_opacity), app));

    // Extra
    app->add_action(                "export-do",                        sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_do),           app));
#else
    std::cerr << "add_actions: Some actions require Glibmm 2.52, compiled with: " << glib_major_version << "." << glib_minor_version << std::endl;
#endif

    app->get_action_extra_data().add_data(raw_data_output);
}


template void add_actions_output(ConcreteInkscapeApplication<Gio::Application>* app);
template void add_actions_output(ConcreteInkscapeApplication<Gtk::Application>* app);



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
