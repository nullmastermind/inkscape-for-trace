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
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    app->file_export()->export_dpi = d.get();
    // std::cout << "export-dpi: " << d.get() << std::endl;
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
    // std::cout << "export-ps-level: " << i.get() << std::endl;
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
export_png_color_mode(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_png_color_mode = s.get();
    // std::cout << s.get() << std::endl;
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
    // clang-format off
    {"app.export-type",               N_("Export Type"),               "Export",     N_("Export file type.")                     },
    {"app.export-filename",           N_("Export File Name"),          "Export",     N_("Export file name.")                     },
    {"app.export-overwrite",          N_("Export Overwrite"),          "Export",     N_("Export over-write file.")               },

    {"app.export-area",               N_("Export Area"),               "Export",     N_("Export area.")                          },
    {"app.export-area-drawing",       N_("Export Area Drawing"),       "Export",     N_("Export drawing area.")                  },
    {"app.export-area-page",          N_("Export Area Page"),          "Export",     N_("Export page area.")                     },
    {"app.export-margin",             N_("Export Margin"),             "Export",     N_("Export margin.")                        },
    {"app.export-area-snap",          N_("Export Area Snap"),          "Export",     N_("Export snap area to integer values.")   },
    {"app.export-width",              N_("Export Width"),              "Export",     N_("Export width.")                         },
    {"app.export-height",             N_("Export Height"),             "Export",     N_("Export height.")                        },

    {"app.export-id",                 N_("Export ID"),                 "Export",     N_("Export id(s).")                         },
    {"app.export-id-only",            N_("Export ID Only"),            "Export",     N_("Export id(s) only.")                    },

    {"app.export-plain-svg",          N_("Export Plain SVG"),          "Export",     N_("Export as plain SVG.")                  },
    {"app.export-dpi",                N_("Export DPI"),                "Export",     N_("Export DPI.")                           },
    {"app.export-ignore-filters",     N_("Export Ignore Filters"),     "Export",     N_("Export ignore filters.")                },
    {"app.export-text-to-path",       N_("Export Text to Path"),       "Export",     N_("Export convert text to paths.")         },
    {"app.export-ps-level",           N_("Export PS Level"),           "Export",     N_("Export PostScript level.")              },
    {"app.export-pdf-version",        N_("Export PDF Version"),        "Export",     N_("Export PDF version.")                   },
    {"app.export-latex",              N_("Export LaTeX"),              "Export",     N_("Export LaTeX.")                         },
    {"app.export-use-hints",          N_("Export Use Hints"),          "Export",     N_("Export using saved hints.")             },
    {"app.export-background",         N_("Export Background"),         "Export",     N_("Export background color.")              },
    {"app.export-background-opacity", N_("Export Background Opacity"), "Export",     N_("Export background opacity.")            },
    {"app.export-png-color-mode",     N_("Export PNG Color Mode"),     "Export",     N_("Export png color mode.")                },

    {"app.export-do",                 N_("Do Export"),                 "Export",     N_("Do export.")                            }
    // clang-format on
};

void
add_actions_output(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);
    Glib::VariantType BString(Glib::VARIANT_TYPE_BYTESTRING);

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)
    auto *gapp = app->gio_app();

    // Matches command line options
    // clang-format off
    gapp->add_action_with_parameter( "export-type",              String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_type),         app));
    gapp->add_action_with_parameter( "export-filename",          String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_filename),     app)); // MAY NOT WORK DUE TO std::string
    gapp->add_action_with_parameter( "export-overwrite",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_overwrite),    app));

    gapp->add_action_with_parameter( "export-area",              String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area),         app));
    gapp->add_action_with_parameter( "export-area-drawing",      Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area_drawing), app));
    gapp->add_action_with_parameter( "export-area-page",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area_page),    app));
    gapp->add_action_with_parameter( "export-margin",            Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_margin),       app));
    gapp->add_action_with_parameter( "export-area-snap",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_area_snap),    app));
    gapp->add_action_with_parameter( "export-width",             Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_width),        app));
    gapp->add_action_with_parameter( "export-height",            Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_height),       app));

    gapp->add_action_with_parameter( "export-id",                String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_id),           app));
    gapp->add_action_with_parameter( "export-id-only",           Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_id_only),      app));

    gapp->add_action_with_parameter( "export-plain-svg",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_plain_svg),    app));
    gapp->add_action_with_parameter( "export-dpi",               Double, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_dpi),          app));
    gapp->add_action_with_parameter( "export-ignore-filters",    Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_plain_svg),    app));
    gapp->add_action_with_parameter( "export-text-to-path",      Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_text_to_path), app));
    gapp->add_action_with_parameter( "export-ps-level",          Int,    sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_ps_level),     app));
    gapp->add_action_with_parameter( "export-pdf-version",       String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_pdf_level),    app));
    gapp->add_action_with_parameter( "export-latex",             Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_latex),        app));
    gapp->add_action_with_parameter( "export-use-hints",         Bool,   sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_use_hints),    app));
    gapp->add_action_with_parameter( "export-background",        String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_background),   app));
    gapp->add_action_with_parameter( "export-background-opacity",Double, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_background_opacity), app));
    gapp->add_action_with_parameter( "export-png-color-mode",    String, sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_png_color_mode), app));

    // Extra
    gapp->add_action(                "export-do",                        sigc::bind<InkscapeApplication*>(sigc::ptr_fun(&export_do),           app));
    // clang-format on
#else
    std::cerr << "add_actions: Some actions require Glibmm 2.52, compiled with: " << glib_major_version << "." << glib_minor_version << std::endl;
#endif

    app->get_action_extra_data().add_data(raw_data_output);
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
