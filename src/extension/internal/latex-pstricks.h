// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __INKSCAPE_EXTENSION_INTERNAL_PRINT_LATEX_H__
#define __INKSCAPE_EXTENSION_INTERNAL_PRINT_LATEX_H__

/*
 * LaTeX Printing
 *
 * Author:
 *  Michael Forbes <miforbes@mbhs.edu>
 *
 * Copyright (C) 2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <stack>

#include "extension/implementation/implementation.h"
#include "extension/extension.h"

#include "svg/stringstream.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

class PrintLatex : public Inkscape::Extension::Implementation::Implementation {

    float  _width;
    float  _height;
    FILE * _stream;
    
    std::stack<Geom::Affine> m_tr_stack;

    void print_pathvector(SVGOStringStream &os, Geom::PathVector const &pathv_in, const Geom::Affine & /*transform*/);
    void print_2geomcurve(SVGOStringStream &os, Geom::Curve const & c );

public:
        PrintLatex ();
        ~PrintLatex () override;

        /* Print functions */
        unsigned int setup (Inkscape::Extension::Print * module) override;

        unsigned int begin (Inkscape::Extension::Print * module, SPDocument *doc) override;
        unsigned int finish (Inkscape::Extension::Print * module) override;

        /* Rendering methods */
        unsigned int bind(Inkscape::Extension::Print *module, Geom::Affine const &transform, float opacity) override;
        unsigned int release(Inkscape::Extension::Print *module) override;

        unsigned int fill (Inkscape::Extension::Print *module, Geom::PathVector const &pathv,
                                   Geom::Affine const &ctm, SPStyle const *style,
                                   Geom::OptRect const &pbox, Geom::OptRect const &dbox,
                                   Geom::OptRect const &bbox) override;
        unsigned int stroke (Inkscape::Extension::Print *module, Geom::PathVector const &pathv,
                                     Geom::Affine const &ctm, SPStyle const *style,
                                     Geom::OptRect const &pbox, Geom::OptRect const &dbox,
                                     Geom::OptRect const &bbox) override;
        bool textToPath (Inkscape::Extension::Print * ext) override;

        static void init ();
};

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* __INKSCAPE_EXTENSION_INTERNAL_PRINT_LATEX */

/*
  Local Variables:
  mode:cpp
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
