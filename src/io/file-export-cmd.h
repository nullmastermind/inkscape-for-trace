// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File export from the command line. This code use to be in main.cpp. It should be
 * replaced by shared code (Gio::Actions?) for export from the file dialog.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_FILE_EXPORT_CMD_H
#define INK_FILE_EXPORT_CMD_H

#include <iostream>
#include <glibmm.h>

class SPDocument;
namespace Inkscape {
namespace Extension {
class Output;
}
} // namespace Inkscape

class InkFileExportCmd {

public:
    InkFileExportCmd();

    void do_export(SPDocument* doc, std::string filename_in="");

private:
    guint32 get_bgcolor(SPDocument *doc);
    std::string get_filename_out(std::string filename_in = "", std::string object_id = "");
    int do_export_svg(SPDocument *doc, std::string const &filename_in);
    int do_export_svg(SPDocument *doc, std::string const &filename_in, Inkscape::Extension::Output &extension);
    int do_export_png(SPDocument *doc, std::string const &filename_in);
    int do_export_ps_pdf(SPDocument *doc, std::string const &filename_in, std::string mime_type);
    int do_export_ps_pdf(SPDocument *doc, std::string const &filename_in, std::string mime_type,
                         Inkscape::Extension::Output &extension);
    int do_export_extension(SPDocument *doc, std::string const &filename_in, Inkscape::Extension::Output *extension);
    Glib::ustring export_type_current;

public:
    // Should be private, but this is just temporary code (I hope!).

    // One-to-one correspondence with command line options
    std::string   export_filename; // Only if one file is processed!

    Glib::ustring export_type;
    Glib::ustring export_extension;
    bool          export_overwrite;

    Glib::ustring export_area;
    bool          export_area_drawing;
    bool          export_area_page;
    int           export_margin;
    bool          export_area_snap;
    int           export_width;
    int           export_height;

    double        export_dpi;
    bool          export_ignore_filters;
    bool          export_text_to_path;
    int           export_ps_level;
    Glib::ustring export_pdf_level;
    bool          export_latex;
    Glib::ustring export_id;
    bool          export_id_only;
    bool          export_use_hints;
    Glib::ustring export_background;
    double        export_background_opacity;
    Glib::ustring export_png_color_mode;
    bool          export_plain_svg;
};

#endif // INK_FILE_EXPORT_CMD_H

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
