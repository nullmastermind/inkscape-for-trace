// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the code that moves all of the SVG loading and saving into
 * the module format.  Really Sodipodi is built to handle these formats
 * internally, so this is just calling those internal functions.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2003 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef __SVG_H__
#define __SVG_H__

#include "../implementation/implementation.h"

#define SVG_COMMON_INPUT_PARAMS \
            "<param name='import_mode_svg' type='optiongroup' gui-text='" N_("SVG Image Import Type:") "' >\n" \
                    "<option value='include' >" N_("Include SVG image as editable object(s) in the current file") "</option>\n" \
                    "<option value='embed' >" N_("Embed the SVG file in a image tag (not editable in this document)") "</option>\n" \
                    "<option value='link' >" N_("Link the SVG file in a image tag (not editable in this document).") "</option>\n" \
                  "</param>\n" \
            "<param name='svgdpi' type='float' precision='2' min='1' max='999999' gui-text='DPI for rendered SVG'>96.00</param>\n" \
            "<param name='scale' appearance='combo' type='optiongroup' gui-text='" N_("Image Rendering Mode:") "' gui-description='" N_("When an image is upscaled, apply smoothing or keep blocky (pixelated). (Will not work in all browsers.)") "' >\n" \
                    "<option value='auto' >" N_("None (auto)") "</option>\n" \
                    "<option value='optimizeQuality' >" N_("Smooth (optimizeQuality)") "</option>\n" \
                    "<option value='optimizeSpeed' >" N_("Blocky (optimizeSpeed)") "</option>\n" \
                  "</param>\n" \
            "<param name=\"do_not_ask\" gui-description='" N_("Hide the dialog next time and always apply the same actions.") "' gui-text=\"" N_("Don't ask again") "\" type=\"bool\" >false</param>\n"


namespace Inkscape {
namespace Extension {
namespace Internal {

class Svg : public Inkscape::Extension::Implementation::Implementation {
    bool m_detachbase = false;

public:
    void        setDetachBase(bool detach) override { m_detachbase = detach; }

    void        save( Inkscape::Extension::Output *mod,
                               SPDocument *doc,
                               gchar const *filename ) override;
    SPDocument *open( Inkscape::Extension::Input *mod,
                                const gchar *uri ) override;
    static void         init( );

};

} } }  /* namespace Inkscape, Extension, Implementation */
#endif /* __SVG_H__ */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
