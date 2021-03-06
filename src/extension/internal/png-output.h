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

#ifndef EXTENSION_INTERNAL_PNG_OUTPUT_H
#define EXTENSION_INTERNAL_PNG_OUTPUT_H

#include <glib.h>

#include "extension/extension.h"
#include "extension/implementation/implementation.h"
#include "extension/output.h"
#include "extension/system.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class PngOutput : public Inkscape::Extension::Implementation::Implementation
{
public:
    PngOutput(){};

    bool check(Inkscape::Extension::Extension *module) override { return true; };

    void export_raster(Inkscape::Extension::Output *module, std::string const png_file, gchar const *filename) override;

    static void init();

private:
};

} // namespace Internal
} // namespace Extension
} // namespace Inkscape

#endif /* EXTENSION_INTERNAL_PNG_OUTPUT_H */
