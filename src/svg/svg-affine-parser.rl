// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG data parser
 *
 * Authors:
 *   Marc Jeanmougin <marc.jeanmougin@telecom-paris.fr>
 *
 * Copyright (C) 2019 Marc Jeanmougin (.rl parser, CSS-transform spec)
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * THE CPP FILE IS GENERATED FROM THE RL FILE, DO NOT EDIT THE CPP
 *
 * To generate it, run
 *   ragel svg-affine-parser.rl -o svg-affine-parser.cpp
 *   sed -Ei 's/\(1\)/(true)/' svg-affine-parser.cpp
 */

#include <string>
#include <glib.h>
#include <2geom/transforms.h>
#include "svg.h"
#include "preferences.h"

%%{
    machine svg_transform;
    write data noerror;
}%%

// https://www.w3.org/TR/css-transforms-1/#svg-syntax
bool sp_svg_transform_read(gchar const *str, Geom::Affine *transform)
{
    if (str == nullptr)
        return false;

    std::vector<double> params;
    Geom::Affine final_transform (Geom::identity());
    *transform = final_transform;
    Geom::Affine tmp_transform (Geom::identity());
    int cs;
    const char *p = str;
    const char *pe = p + strlen(p) + 1;
    const char *eof = pe;
    char const *start_num = 0;
    char const *ts = p;
    char const *te = pe;
    int act = 0;
    if (pe == p+1) return true; // ""

%%{
    write init;

    action number {
      std::string buf(start_num, p); 
      params.push_back(g_ascii_strtod(buf.c_str(), NULL));
    }

    action translate { tmp_transform = Geom::Translate(params[0], params.size() == 1 ? 0 : params[1]); }
    action scale { tmp_transform = Geom::Scale(params[0], params.size() == 1 ? params[0] : params[1]); }
    action rotate {
        if (params.size() == 1) 
            tmp_transform = Geom::Rotate(Geom::rad_from_deg(params[0])); 
        else {
            tmp_transform = Geom::Translate(-params[1], -params[2]) * 
                            Geom::Rotate(Geom::rad_from_deg(params[0])) * 
                            Geom::Translate(params[1], params[2]); 
        } 
    }
    action skewX { tmp_transform = Geom::Affine(1, 0, tan(params[0] * M_PI / 180.0), 1, 0, 0); }
    action skewY { tmp_transform = Geom::Affine(1, tan(params[0] * M_PI / 180.0), 0, 1, 0, 0); }
    action matrix { tmp_transform = Geom::Affine(params[0], params[1], params[2], params[3], params[4], params[5]);}
    action transform {params.clear(); final_transform = tmp_transform * final_transform ;}
    action transformlist { *transform = final_transform; /*printf("%p %p %p %p
%d\n",p, pe, ts, te, cs);*/ return (te+1 == pe);}

    comma = ',';
    wsp = ( 10 | 13 | 9 | ' ');
    sign = ('+' | '-');
    digit_sequence = digit+;
    exponent = ('e' | 'E') sign? digit_sequence;
    fractional_constant = digit_sequence? '.' digit_sequence
            | digit_sequence '.';
    floating_point_constant = fractional_constant exponent?
            | digit_sequence exponent;
    integer_constant = digit_sequence;
    number = ( sign? integer_constant | sign? floating_point_constant )
           > {start_num = p;}
           %number;

    commawsp = (wsp+ comma? wsp*) | (comma wsp*);
    translate = "translate" wsp* "(" wsp* number <: ( commawsp? number )? wsp* ")"
      %translate;
    scale = "scale" wsp* "(" wsp* number <: ( commawsp? number )? wsp* ")"
      %scale;
    rotate = "rotate" wsp* "(" wsp* number <: ( commawsp? number <: commawsp? number )? wsp* ")"
      %rotate;
    skewX = "skewX" wsp* "(" wsp* number wsp* ")"
      %skewX;
    skewY = "skewY" wsp* "(" wsp* number wsp* ")"
      %skewY;
    matrix = "matrix" wsp* "(" wsp*
      number <: commawsp?
      number <: commawsp?
      number <: commawsp?
      number <: commawsp?
      number <: commawsp?
      number wsp* ")"
      %matrix;
    transform = (matrix | translate | scale | rotate | skewX | skewY)
      %transform;
    transforms = transform ( commawsp? transform )**;
    transformlist = wsp* transforms? wsp*;
    main := |* 
      transformlist => transformlist;
    *|;
    write exec;
}%%
    g_warning("could not parse transform attribute");

    return false;
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
// vim: filetype=ragel:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
