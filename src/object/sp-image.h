// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG <image> implementation
 *//*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Edward Flick (EAF)
 *
 * Copyright (C) 1999-2005 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_SP_IMAGE_H
#define SEEN_INKSCAPE_SP_IMAGE_H

#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#include <glibmm/ustring.h>
#include "svg/svg-length.h"
#include "display/curve.h"
#include "sp-item.h"
#include "viewbox.h"
#include "sp-dimensions.h"

#define SP_IMAGE(obj) (dynamic_cast<SPImage*>((SPObject*)obj))
#define SP_IS_IMAGE(obj) (dynamic_cast<const SPImage*>((SPObject*)obj) != NULL)

#define SP_IMAGE_HREF_MODIFIED_FLAG SP_OBJECT_USER_MODIFIED_FLAG_A

namespace Inkscape { class Pixbuf; }
class SPImage : public SPItem, public SPViewBox, public SPDimensions {
public:
    SPImage();
    ~SPImage() override;

    Geom::Rect clipbox;
    double sx, sy;
    double ox, oy;
    double dpi;
    double prev_width, prev_height;

    SPCurve *curve; // This curve is at the image's boundary for snapping

    char *href;
#if defined(HAVE_LIBLCMS2)
    char *color_profile;
#endif // defined(HAVE_LIBLCMS2)

    Inkscape::Pixbuf *pixbuf;

    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void release() override;
    void set(SPAttributeEnum key, char const* value) override;
    void update(SPCtx *ctx, unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
    void modified(unsigned int flags) override;

    Geom::OptRect bbox(Geom::Affine const &transform, SPItem::BBoxType type) const override;
    void print(SPPrintContext *ctx) override;
    const char* displayName() const override;
    char* description() const override;
    Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
    void snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const override;
    Geom::Affine set_transform(Geom::Affine const &transform) override;

#if defined(HAVE_LIBLCMS2)
    void apply_profile(Inkscape::Pixbuf *pixbuf);
#endif // defined(HAVE_LIBLCMS2)

    SPCurve *get_curve () const;
    void refresh_if_outdated();
};

/* Return duplicate of curve or NULL */
void sp_embed_image(Inkscape::XML::Node *imgnode, Inkscape::Pixbuf *pb);
void sp_embed_svg(Inkscape::XML::Node *image_node, std::string const &fn);

#endif
