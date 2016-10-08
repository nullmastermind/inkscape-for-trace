/*
 * viewBox helper class, common code used by root, symbol, marker, pattern, image, view
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com> (code extracted from symbol.cpp)
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Johan Engelen
 *
 * Copyright (C) 2013-2014 Tavmjong Bah, authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 *
 */

#include <2geom/transforms.h>

#include "viewbox.h"
#include "enums.h"
#include "sp-item.h"
#include "display/sp-canvas-group.h"
#include "display/canvas-bpath.h"
#include "display/curve.h"
#include "inkscape.h"
#include "desktop.h"

#include "display/sodipodi-ctrl.h"
#include "ui/dialog/knot-properties.h"

SPViewBox::SPViewBox()
    : viewBox_set(false)
    , viewBox()
    , aspect_set(false)
    , aspect_align(SP_ASPECT_XMID_YMID) // Default per spec
    , aspect_clip(SP_ASPECT_MEET)
    , c2p(Geom::identity())
    , rotation(Geom::identity())
    , angle(0)
    , previous_angle(0)
    , rotated(false)
{
}

void SPViewBox::set_viewBox(const gchar* value) {

  if (value) {
    gchar *eptr = const_cast<gchar*>(value); // const-cast necessary because of const-incorrect interface definition of g_ascii_strtod

    double x = g_ascii_strtod (eptr, &eptr);

    while (*eptr && ((*eptr == ',') || (*eptr == ' '))) {
      eptr++;
    }

    double y = g_ascii_strtod (eptr, &eptr);

    while (*eptr && ((*eptr == ',') || (*eptr == ' '))) {
      eptr++;
    }

    double width = g_ascii_strtod (eptr, &eptr);

    while (*eptr && ((*eptr == ',') || (*eptr == ' '))) {
      eptr++;
    }

    double height = g_ascii_strtod (eptr, &eptr);

    while (*eptr && ((*eptr == ',') || (*eptr == ' '))) {
      eptr++;
    }

    if ((width > 0) && (height > 0)) {
      /* Set viewbox */
      this->viewBox = Geom::Rect::from_xywh(x, y, width, height);
      this->viewBox_set = true;
    } else {
      this->viewBox_set = false;
    }
  } else {
    this->viewBox_set = false;
  }

  // The C++ way?  -- not necessarily using iostreams
  // std::string sv( value );
  // std::replace( sv.begin(), sv.end(), ',', ' ');
  // std::stringstream ss( sv );
  // double x, y, width, height;
  // ss >> x >> y >> width >> height;
}

void SPViewBox::set_preserveAspectRatio(const gchar* value) {

  /* Do setup before, so we can use break to escape */
  this->aspect_set = false;
  this->aspect_align = SP_ASPECT_XMID_YMID; // Default per spec
  this->aspect_clip = SP_ASPECT_MEET;

  if (value) {
    const gchar *p = value;

    while (*p && (*p == 32)) {
      p += 1;
    }

    if (!*p) {
      return;
    }

    const gchar *e = p;

    while (*e && (*e != 32)) {
      e += 1;
    }

    int len = e - p;

    if (len > 8) {  // Can't have buffer overflow as 8 < 256
      return;
    }

    gchar c[256];
    memcpy (c, value, len);

    c[len] = 0;

    /* Now the actual part */
    unsigned int align = SP_ASPECT_NONE;
    if (!strcmp (c, "none")) {
      align = SP_ASPECT_NONE;
    } else if (!strcmp (c, "xMinYMin")) {
      align = SP_ASPECT_XMIN_YMIN;
    } else if (!strcmp (c, "xMidYMin")) {
      align = SP_ASPECT_XMID_YMIN;
    } else if (!strcmp (c, "xMaxYMin")) {
      align = SP_ASPECT_XMAX_YMIN;
    } else if (!strcmp (c, "xMinYMid")) {
      align = SP_ASPECT_XMIN_YMID;
    } else if (!strcmp (c, "xMidYMid")) {
      align = SP_ASPECT_XMID_YMID;
    } else if (!strcmp (c, "xMaxYMid")) {
      align = SP_ASPECT_XMAX_YMID;
    } else if (!strcmp (c, "xMinYMax")) {
      align = SP_ASPECT_XMIN_YMAX;
    } else if (!strcmp (c, "xMidYMax")) {
      align = SP_ASPECT_XMID_YMAX;
    } else if (!strcmp (c, "xMaxYMax")) {
      align = SP_ASPECT_XMAX_YMAX;
    } else {
      return;
    }

    unsigned int clip = SP_ASPECT_MEET;

    while (*e && (*e == 32)) {
      e += 1;
    }

    if (*e) {
      if (!strcmp (e, "meet")) {
        clip = SP_ASPECT_MEET;
      } else if (!strcmp (e, "slice")) {
        clip = SP_ASPECT_SLICE;
      } else {
        return;
      }
    }

    this->aspect_set = true;
    this->aspect_align = align;
    this->aspect_clip = clip;
  }
}

double SPViewBox::get_rotation() {
    return this->angle;
}

void SPViewBox::set_rotation(double angle_val) {
    this->previous_angle = this->angle;
    this->rotated = true;
    this->angle = angle_val;
}

// Apply scaling from viewbox
void SPViewBox::apply_viewbox(const Geom::Rect& in, double scale_none) {

    /* Determine actual viewbox in viewport coordinates */
    // scale_none is the scale that would apply if the viewbox and page size are same size
    // it is passed here because it is a double-precision variable, while 'in' is originally float
    double x = 0.0;
    double y = 0.0;
    double scale_x = in.width() / this->viewBox.width();
    double scale_y = in.height() / this->viewBox.height();
    double scale_uniform = 1.0; // used only if scaling is uniform

    if (Geom::are_near(scale_x / scale_y, 1.0, Geom::EPSILON)) {
      // scaling is already uniform, reduce numerical error
      scale_uniform = (scale_x + scale_y)/2.0;
      if (Geom::are_near(scale_uniform / scale_none, 1.0, Geom::EPSILON))
          scale_uniform = scale_none; // objects are same size, reduce numerical error
      scale_x = scale_uniform;
      scale_y = scale_uniform;
    } else if (this->aspect_align != SP_ASPECT_NONE) {
      // scaling is not uniform, but force it to be
      scale_uniform = (this->aspect_clip == SP_ASPECT_MEET) ? MIN (scale_x, scale_y) : MAX (scale_x, scale_y);
      scale_x = scale_uniform;
      scale_y = scale_uniform;
      double width  = this->viewBox.width()  * scale_uniform;
      double height = this->viewBox.height() * scale_uniform;

      /* Now place viewbox to requested position */
      switch (this->aspect_align) {
        case SP_ASPECT_XMIN_YMIN:
          break;
        case SP_ASPECT_XMID_YMIN:
          x = 0.5 * (in.width() - width);
          break;
        case SP_ASPECT_XMAX_YMIN:
          x = 1.0 * (in.width() - width);
          break;
        case SP_ASPECT_XMIN_YMID:
          y = 0.5 * (in.height() - height);
          break;
        case SP_ASPECT_XMID_YMID:
          x = 0.5 * (in.width() - width);
          y = 0.5 * (in.height() - height);
          break;
        case SP_ASPECT_XMAX_YMID:
          x = 1.0 * (in.width() - width);
          y = 0.5 * (in.height() - height);
          break;
        case SP_ASPECT_XMIN_YMAX:
          y = 1.0 * (in.height() - height);
          break;
        case SP_ASPECT_XMID_YMAX:
          x = 0.5 * (in.width() - width);
          y = 1.0 * (in.height() - height);
          break;
        case SP_ASPECT_XMAX_YMAX:
          x = 1.0 * (in.width() - width);
          y = 1.0 * (in.height() - height);
          break;
        default:
          break;
      }
    }

    /* Viewbox transform from scale and position */
    Geom::Affine vbt = Geom::identity();
    vbt[0] = scale_x;
    vbt[1] = 0.0;
    vbt[2] = 0.0;
    vbt[3] = scale_y;
    vbt[4] = x - scale_x * this->viewBox.left();
    vbt[5] = y - scale_y * this->viewBox.top();
    // std::cout << "  q\n" << q << std::endl
    /* Append viewbox and turn transformation */
    if (this->angle > 0.0 || this->angle < 0.0 ) { //!0
        Geom::Point r0 = this->viewBox.min();
        Geom::Point r1 = this->viewBox.max();
        Geom::Point p = (r0 + r1) / 2;
        SPDesktop * desktop = SP_ACTIVE_DESKTOP;
        SPCanvasItem *vw_rot = NULL;
        Geom::Point rot_center = Geom::Point();
        if (desktop) {
            if (this->page_border_rotated) {
                desktop->remove_temporary_canvasitem(this->page_border_rotated);
                this->page_border_rotated = NULL;
            }
            SPCurve *c = new SPCurve();
            c->moveto(this->viewBox.min());
            c->lineto(Geom::Point(this->viewBox.max()[Geom::X],this->viewBox.min()[Geom::Y]));
            c->lineto(Geom::Point(this->viewBox.max()[Geom::X],this->viewBox.max()[Geom::Y]));
            c->lineto(Geom::Point(this->viewBox.min()[Geom::X],this->viewBox.max()[Geom::Y]));
            c->closepath();
            vw_rot = sp_canvas_bpath_new(desktop->getTempGroup(), c, true);
            sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(vw_rot), 0xFF00009A, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
            sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(vw_rot), 0, SP_WIND_RULE_NONZERO);
            this->page_border_rotated = desktop->add_temporary_canvasitem(vw_rot, 0);
            Geom::PathVector const box = c->get_pathvector();
            Geom::OptRect bbox = box.boundsFast();
            if (bbox){
                p = (*bbox).midpoint();
            }
            Geom::Rect view = desktop->get_display_area();
            rot_center =  desktop->doc2dt(view.midpoint());
            Geom::Affine rot = Geom::identity();
            rot *= Geom::Translate(p).inverse() * Geom::Rotate(Geom::rad_from_deg(-angle)) *  Geom::Translate(p);
            sp_canvas_item_affine_absolute(vw_rot, rot * vbt);
            this->page_border_rotated = desktop->add_temporary_canvasitem(vw_rot, 0);
            Geom::Affine rotcenter = Geom::identity();
            rotcenter *= Geom::Translate(p * vbt).inverse() * Geom::Rotate(Geom::rad_from_deg(this->angle - this->previous_angle)) *  Geom::Translate(p * vbt);
            rot_center = rot_center * rotcenter;
        }
        rotation = Geom::Translate(p).inverse() * Geom::Rotate(Geom::rad_from_deg(angle)) * Geom::Translate(p);
        this->c2p = rotation * vbt * this->c2p;
        rot_center =  desktop->dt2doc(rot_center);
        if (desktop && this->rotated) {
            desktop->zoom_relative(rot_center[Geom::X], rot_center[Geom::Y], 1.0);
            this->rotated = false;
        }
    } else {
        SPDesktop * desktop = SP_ACTIVE_DESKTOP;
        if (desktop) {
            if (this->page_border_rotated) {
                desktop->remove_temporary_canvasitem(this->page_border_rotated);
                this->page_border_rotated = NULL;
            }
        }
        this->c2p = vbt * this->c2p;
    }
}


SPItemCtx SPViewBox::get_rctx(const SPItemCtx* ictx, double scale_none) {

  /* Create copy of item context */
  SPItemCtx rctx = *ictx;

  /* Calculate child to parent transformation */
  /* Apply parent translation (set up as viewport) */
  this->c2p = Geom::Translate(rctx.viewport.min());

  if (this->viewBox_set) {
    // Adjusts c2p for viewbox
    apply_viewbox( rctx.viewport, scale_none );
  }

  rctx.i2doc = this->c2p * rctx.i2doc;

  /* If viewBox is set initialize child viewport */
  /* Otherwise it is already correct */
  if (this->viewBox_set) {
    rctx.viewport = this->viewBox;
    rctx.i2vp = Geom::identity();
  }

  return rctx;
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
