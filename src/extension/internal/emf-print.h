/** @file
 * @brief Enhanced Metafile printing - implementation
 */
/* Author:
 *   Ulf Erikson <ulferikson@users.sf.net>
 *
 * Copyright (C) 2006-2008 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef __INKSCAPE_EXTENSION_INTERNAL_PRINT_EMF_H__
#define __INKSCAPE_EXTENSION_INTERNAL_PRINT_EMF_H__


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "uemf.h"

#include "extension/implementation/implementation.h"
//#include "extension/extension.h"

#include <2geom/pathvector.h>

#include <stack>

namespace Inkscape {
namespace Extension {
namespace Internal {

class PrintEmf : public Inkscape::Extension::Implementation::Implementation
{
    double  _width;
    double  _height;
    U_RECTL  rc;

    uint32_t hbrush, hbrushOld, hpen, hpenOld;

    std::stack<Geom::Affine> m_tr_stack;
    Geom::PathVector fill_pathv;
    Geom::Affine fill_transform;
    bool use_stroke;
    bool use_fill;
    bool simple_shape;

    unsigned int print_pathv (Geom::PathVector const &pathv, const Geom::Affine &transform);
    bool print_simple_shape (Geom::PathVector const &pathv, const Geom::Affine &transform);

public:
    PrintEmf (void);
    virtual ~PrintEmf (void);

    /* Print functions */
    virtual unsigned int setup (Inkscape::Extension::Print * module);

    virtual unsigned int begin (Inkscape::Extension::Print * module, SPDocument *doc);
    virtual unsigned int finish (Inkscape::Extension::Print * module);

    /* Rendering methods */
    virtual unsigned int bind(Inkscape::Extension::Print *module, Geom::Affine const &transform, float opacity);
    virtual unsigned int release(Inkscape::Extension::Print *module);
    virtual unsigned int fill (Inkscape::Extension::Print *module,
                               Geom::PathVector const &pathv,
                               Geom::Affine const &ctm, SPStyle const *style,
                               Geom::OptRect const &pbox, Geom::OptRect const &dbox,
                               Geom::OptRect const &bbox);
    virtual unsigned int stroke (Inkscape::Extension::Print * module,
                                 Geom::PathVector const &pathv,
                                 Geom::Affine const &ctm, SPStyle const *style,
                                 Geom::OptRect const &pbox, Geom::OptRect const &dbox,
                                 Geom::OptRect const &bbox);
    virtual unsigned int image(Inkscape::Extension::Print *module,
                           unsigned char *px,
                           unsigned int w,
                           unsigned int h,
                           unsigned int rs,
                           Geom::Affine const &transform,
                           SPStyle const *style);
    virtual unsigned int comment(Inkscape::Extension::Print *module, const char * comment);
    virtual unsigned int text(Inkscape::Extension::Print *module, char const *text,
                              Geom::Point const &p, SPStyle const *style);
    bool textToPath (Inkscape::Extension::Print * ext);

    static void init (void);
protected:
    int create_brush(SPStyle const *style, PU_COLORREF fcolor);

    void destroy_brush();

    int create_pen(SPStyle const *style, const Geom::Affine &transform);

    void destroy_pen();

};

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */


#endif /* __INKSCAPE_EXTENSION_INTERNAL_PRINT_EMF_H__ */

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
