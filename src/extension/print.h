// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Abhishek Sharma
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_PRINT_H__
#define INKSCAPE_EXTENSION_PRINT_H__

#include <2geom/affine.h>
#include <2geom/generic-rect.h>
#include <2geom/pathvector.h>
#include <2geom/point.h>
#include "extension.h"

class SPItem;
class SPStyle;

namespace Inkscape {

class Drawing;
class DrawingItem;

namespace Extension {

class Print : public Extension {

public: /* TODO: These are public for the short term, but this should be fixed */
    SPItem *base;
    Inkscape::Drawing *drawing;
    Inkscape::DrawingItem *root;
    unsigned int dkey;

public:
    Print(Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory);
    ~Print() override;

    bool check() override;

    /* FALSE means user hit cancel */
    unsigned int  setup       ();
    unsigned int  set_preview ();

    unsigned int  begin       (SPDocument *doc);
    unsigned int  finish      ();

    /* Rendering methods */
    unsigned int  bind        (Geom::Affine const &transform,
                               float opacity);
    unsigned int  release     ();
    unsigned int  comment     (const char * comment);
    unsigned int  fill        (Geom::PathVector const &pathv,
                               Geom::Affine const &ctm,
                               SPStyle const *style,
                               Geom::OptRect const &pbox,
                               Geom::OptRect const &dbox,
                               Geom::OptRect const &bbox);
    unsigned int  stroke      (Geom::PathVector const &pathv,
                               Geom::Affine const &transform,
                               SPStyle const *style,
                               Geom::OptRect const &pbox,
                               Geom::OptRect const &dbox,
                               Geom::OptRect const &bbox);
    unsigned int  image       (unsigned char *px,
                               unsigned int w,
                               unsigned int h,
                               unsigned int rs,
                               Geom::Affine const &transform,
                               SPStyle const *style);
    unsigned int  text        (char const *text,
                               Geom::Point const &p,
                               SPStyle const *style);
    bool          textToPath  ();
    bool          fontEmbedded  ();
};

} }  /* namespace Inkscape, Extension */
#endif /* INKSCAPE_EXTENSION_PRINT_H__ */

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
