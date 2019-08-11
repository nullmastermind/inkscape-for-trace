// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Image widget for extensions
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "widget-image.h"

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <gtkmm/image.h>

#include "xml/node.h"
#include "extension/extension.h"

namespace Inkscape {
namespace Extension {


WidgetImage::WidgetImage(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext)
    : InxWidget(xml, ext)
{
    std::string image_path;

    // get path to image
    const char *content = nullptr;
    if (xml->firstChild()) {
        content = xml->firstChild()->content();
    }
    if (content) {
        image_path = content;
    } else {
        g_warning("Missing path for image widget in extension '%s'.", _extension->get_id());
        return;
    }

    // make sure path is absolute (relative paths are relative to .inx file's location)
    if (!Glib::path_is_absolute(image_path)) {
        image_path = Glib::build_filename(_extension->get_base_directory(), image_path);
    }

    // check if image exists
    if (Glib::file_test(image_path, Glib::FILE_TEST_IS_REGULAR)) {
        _image_path = image_path;
    } else {
        g_warning("Image file ('%s') not found for image widget in extension '%s'.",
                  image_path.c_str(), _extension->get_id());
    }

    // parse width/height attributes
    const char *width = xml->attribute("width");
    const char *height = xml->attribute("height");
    if (width && height) {
        _width = strtoul(width, nullptr, 0);
        _height = strtoul(height, nullptr, 0);
    }
}

/** \brief  Create a label for the description */
Gtk::Widget *WidgetImage::get_widget(sigc::signal<void> * /*changeSignal*/)
{
    if (_hidden || _image_path.empty()) {
        return nullptr;
    }

    Gtk::Image *image = Gtk::manage(new Gtk::Image(_image_path));

    // resize if requested
    if (_width && _height) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = image->get_pixbuf();
        pixbuf = pixbuf->scale_simple(_width, _height, Gdk::INTERP_BILINEAR);
        image->set(pixbuf);
    }

    image->show();

    return dynamic_cast<Gtk::Widget *>(image);
}

}  /* namespace Extension */
}  /* namespace Inkscape */
