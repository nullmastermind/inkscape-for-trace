// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */



#ifndef INKSCAPE_EXTENSION_OUTPUT_H__
#define INKSCAPE_EXTENSION_OUTPUT_H__

#include "extension.h"
class SPDocument;

namespace Inkscape {
namespace Extension {

class Output : public Extension {
    gchar *mimetype;             /**< What is the mime type this inputs? */
    gchar *extension;            /**< The extension of the input files */
    gchar *filetypename;         /**< A userfriendly name for the file type */
    gchar *filetypetooltip;      /**< A more detailed description of the filetype */
    bool   dataloss;             /**< The extension causes data loss on save */
    bool   raster;               /**< Is the extension expecting a png file */

public:
    class save_failed {};        /**< Generic failure for an undescribed reason */
    class save_cancelled {};     /**< Saving was cancelled */
    class no_extension_found {}; /**< Failed because we couldn't find an extension to match the filename */
    class file_read_only {};     /**< The existing file can not be opened for writing */
    class export_id_not_found {  /**< The object ID requested for export could not be found in the document */
        public:
            const gchar * const id;
            export_id_not_found(const gchar * const id = nullptr) : id{id} {};
    };

    Output(Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory);
    ~Output () override;

    bool check() override;

    void         save (SPDocument *doc,
                       gchar const *filename,
                       bool detachbase = false);
    void         export_raster (const SPDocument *doc,
                                std::string png_filename,
                                gchar const *filename,
                                bool detachbase);
    bool         prefs ();
    gchar *      get_mimetype();
    gchar *      get_extension();
    const char * get_filetypename(bool translated=false);
    const char * get_filetypetooltip(bool translated=false);
    bool         causes_dataloss() { return dataloss; };
    bool         is_raster() { return raster; };
};

} }  /* namespace Inkscape, Extension */
#endif /* INKSCAPE_EXTENSION_OUTPUT_H__ */

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
