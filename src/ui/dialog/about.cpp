// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for the about screen
 *
 * Copyright (C) Martin Owens 2019 <doctormo@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <fstream>
#include <streambuf>

#include "about.h"
#include "document.h"
#include "inkscape-version.h"

#include "io/resource.h"
#include "ui/view/svg-view-widget.h"

#include "util/units.h"

using namespace Inkscape::IO;
using namespace Inkscape::UI::View;

namespace Inkscape {
namespace UI {
namespace Dialog {

static Gtk::Window *window = nullptr;
static Gtk::Notebook *tabs = nullptr;

void close_about_screen() {
    window->hide();
}

void show_license() {
    tabs->set_current_page(2);
}
void show_credits() {
    tabs->set_current_page(1);
}

void AboutDialog::show_about() {

    if(!window) {
        // Load glade file here
        Glib::ustring gladefile = Resource::get_filename(Resource::UIS, "inkscape-about.glade");
        Glib::RefPtr<Gtk::Builder> builder;
        try {
            builder = Gtk::Builder::create_from_file(gladefile);
        } catch (const Glib::Error &ex) {
            g_error("Glade file loading failed for about screen dialog");
            return;
        }
        builder->get_widget("about-screen-window", window);
        builder->get_widget("tabs", tabs);
        if(!tabs || !window) {
            g_error("Window or tabs in glade file are missing or do not have the right ids.");
            return;
        }
        // Automatic signal handling (requires -rdynamic compile flag)
        //gtk_builder_connect_signals(builder->gobj(), NULL);

        // When automatic handling fails
        Gtk::Button *close_button;
        builder->get_widget("close", close_button);
        if(close_button) close_button->signal_clicked().connect( sigc::ptr_fun(&close_about_screen) );

        Gtk::Button *license_button;
        builder->get_widget("license", license_button);
        if(license_button) license_button->signal_clicked().connect( sigc::ptr_fun(&show_license) );

        Gtk::Button *credits_button;
        builder->get_widget("credits", credits_button);
        if(credits_button) credits_button->signal_clicked().connect( sigc::ptr_fun(&show_credits) );

        Gtk::Label *version;
        builder->get_widget("version", version);
        if(version) version->set_text(Inkscape::version_string);

        // Render the about screen image via inkscape SPDocument
        auto filename = Resource::get_filename(Resource::SCREENS, "about.svg", true, false);
        SPDocument *doc = SPDocument::createNewDoc(filename.c_str(), TRUE);

        // Bind glade's container to our SVGViewWidget class
        if(doc) {
            //SVGViewWidget *viewer;
            //builder->get_widget_derived("image-container", viewer, doc);
            //Gtk::manage(viewer);
            auto viewer = Gtk::manage(new Inkscape::UI::View::SVGViewWidget(doc));
            double width = doc->getWidth().value("px");
            double height = doc->getHeight().value("px");
            viewer->setResize(width, height);

            Gtk::AspectFrame *splash_widget;
            builder->get_widget("aspect-frame", splash_widget);
            splash_widget->unset_label();
            splash_widget->set_shadow_type(Gtk::SHADOW_NONE);
            splash_widget->property_ratio() = width / height;
            splash_widget->add(*viewer);
            splash_widget->show_all();
        } else {
            g_error("Error loading about screen SVG.");
        }

        Gtk::TextView *credits;
        builder->get_widget("credits-text", credits);
        if(credits) {
            std::ifstream fn(Resource::get_filename(Resource::DOCS, "AUTHORS"));
            std::string str((std::istreambuf_iterator<char>(fn)),
                             std::istreambuf_iterator<char>());
            credits->get_buffer()->set_text(str.c_str());
        }

        Gtk::TextView *license;
        builder->get_widget("license-text", license);
        if(license) {
            std::ifstream fn(Resource::get_filename(Resource::DOCS, "LICENSE"));
            std::string str((std::istreambuf_iterator<char>(fn)),
                             std::istreambuf_iterator<char>());
            license->get_buffer()->set_text(str.c_str());
        }
    }
    if(window) {
        window->show();
        tabs->set_current_page(0);
    } else {
        g_error("About screen window couldn't be loaded. Missing window id in glade file.");
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
