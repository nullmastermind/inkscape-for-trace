// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG data parser
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Raph Levien <raph@acm.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 1999 Raph Levien
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <glib.h>
#include <2geom/transforms.h>
#include "svg.h"
#include "preferences.h"

std::string
sp_svg_transform_write(Geom::Affine const &transform)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // this must be a bit grater than EPSILON
    double e = 1e-5 * transform.descrim();
    int prec = prefs->getInt("/options/svgoutput/numericprecision", 8);
    int min_exp = prefs->getInt("/options/svgoutput/minimumexponent", -8);

    // Special case: when all fields of the affine are zero,
    // the optimized transformation is scale(0)
    if (transform[0] == 0 && transform[1] == 0 && transform[2] == 0 &&
        transform[3] == 0 && transform[4] == 0 && transform[5] == 0)
    {
        return "scale(0)";
    }

    // FIXME legacy C code!
    // the function sp_svg_number_write_de is stopping me from using a proper C++ string

    gchar c[256]; // string buffer
    unsigned p = 0; // position in the buffer

    if (transform.isIdentity()) {
        // We are more or less identity, so no transform attribute needed:
        return {};
    } else if (transform.isScale()) {
        // We are more or less a uniform scale
        strcpy (c + p, "scale(");
        p += 6;
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[0], prec, min_exp );
        if (Geom::are_near(transform[0], transform[3], e)) {
            c[p++] = ')';
            c[p] = '\000';
        } else {
            c[p++] = ',';
            p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[3], prec, min_exp );
            c[p++] = ')';
            c[p] = '\000';
        }
    } else if (transform.isTranslation()) {
        // We are more or less a pure translation
        strcpy (c + p, "translate(");
        p += 10;
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[4], prec, min_exp );
        if (Geom::are_near(transform[5], 0.0, e)) {
            c[p++] = ')';
            c[p] = '\000';
        } else {
            c[p++] = ',';
            p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[5], prec, min_exp );
            c[p++] = ')';
            c[p] = '\000';
        }
    } else if (transform.isRotation()) {
        // We are more or less a pure rotation
        strcpy(c + p, "rotate(");
        p += 7;

        double angle = std::atan2(transform[1], transform[0]) * (180 / M_PI);
        p += sp_svg_number_write_de(c + p, sizeof(c) - p, angle, prec, min_exp);

        c[p++] = ')';
        c[p] = '\000';
    } else if (transform.withoutTranslation().isRotation()) {
        // Solution found by Johan Engelen
        // Refer to the matrix in svg-affine-test.h

        // We are a rotation about a special axis
        strcpy(c + p, "rotate(");
        p += 7;

        double angle = std::atan2(transform[1], transform[0]) * (180 / M_PI);
        p += sp_svg_number_write_de(c + p, sizeof(c) - p, angle, prec, min_exp);
        c[p++] = ',';

        Geom::Affine const& m = transform;
        double tx = (m[2]*m[5]+m[4]-m[4]*m[3]) / (1-m[3]-m[0]+m[0]*m[3]-m[2]*m[1]);
        p += sp_svg_number_write_de(c + p, sizeof(c) - p, tx, prec, min_exp);

        c[p++] = ',';

        double ty = (m[1]*tx + m[5]) / (1 - m[3]);
        p += sp_svg_number_write_de(c + p, sizeof(c) - p, ty, prec, min_exp);

        c[p++] = ')';
        c[p] = '\000';
    } else if (transform.isHShear()) {
        // We are more or less a pure skewX
        strcpy(c + p, "skewX(");
        p += 6;

        double angle = atan(transform[2]) * (180 / M_PI);
        p += sp_svg_number_write_de(c + p, sizeof(c) - p, angle, prec, min_exp);

        c[p++] = ')';
        c[p] = '\000';
    } else if (transform.isVShear()) {
        // We are more or less a pure skewY
        strcpy(c + p, "skewY(");
        p += 6;

        double angle = atan(transform[1]) * (180 / M_PI);
        p += sp_svg_number_write_de(c + p, sizeof(c) - p, angle, prec, min_exp);

        c[p++] = ')';
        c[p] = '\000';
    } else {
        strcpy (c + p, "matrix(");
        p += 7;
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[0], prec, min_exp );
        c[p++] = ',';
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[1], prec, min_exp );
        c[p++] = ',';
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[2], prec, min_exp );
        c[p++] = ',';
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[3], prec, min_exp );
        c[p++] = ',';
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[4], prec, min_exp );
        c[p++] = ',';
        p += sp_svg_number_write_de( c + p, sizeof(c) - p, transform[5], prec, min_exp );
        c[p++] = ')';
        c[p] = '\000';
    }

    assert(p <= sizeof(c));
    return c;
}

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
