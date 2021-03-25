// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * An internal raster export which passes the generated PNG output
 * to an external file. In the future this module could host more of
 * the PNG generation code that isn't needed for other raster export options.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "png-output.h"

#include <clocale>
#include <cstdio>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

#include "clear-n_.h"

// Replace with C++17, see notes in file-export-cmd.cpp
#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;

namespace Inkscape {
namespace Extension {
namespace Internal {

void PngOutput::export_raster(Inkscape::Extension::Output * /*module*/,
        const SPDocument * /*doc*/, std::string const png_file, gchar const *filename)
{
    // We want to move the png file to the new location
    auto input_fn = filesystem::path(png_file);
    auto output_fn = filesystem::path(filename);
    filesystem::copy_file(input_fn, output_fn, filesystem::copy_option::overwrite_if_exists);
    boost::filesystem::remove(input_fn);
}

void PngOutput::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Portable Network Graphic") "</name>\n"
            "<id>org.inkscape.output.png.inkscape</id>\n"
            "<output raster=\"true\">\n"
                "<extension>.png</extension>\n"
                "<mimetype>image/png</mimetype>\n"
                "<filetypename>" N_("Portable Network Graphic (*.png)") "</filetypename>\n"
                "<filetypetooltip>" N_("Default raster graphic export") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>",
        new PngOutput());
    // clang-format on
}

} // namespace Internal
} // namespace Extension
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
