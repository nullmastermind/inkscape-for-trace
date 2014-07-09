#ifndef INKSCAPE_LPE_ENVELOPE_PERSPECTIVE_H
#define INKSCAPE_LPE_ENVELOPE_PERSPECTIVE_H

/** \file
 * LPE <envelope-perspective> implementation , see lpe-envelope-perspective.cpp.
 
 */
/*
 * Authors:
 *   Jabiertxof Code migration from python extensions envelope and perspective
 *   Aaron Spike, aaron@ekips.org from envelope and perspective phyton code
 *   Dmitry Platonov, shadowjack@mail.ru, 2006 perspective approach & math
 *   Jose Hevia (freon) Transform algorithm from envelope
 *   
 * Copyright (C) 2007-2014 Authors 
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/parameter/enum.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/pointreseteable.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEEnvelopePerspective : public Effect, GroupBBoxEffect {
public:

    LPEEnvelopePerspective(LivePathEffectObject *lpeobject);
    virtual ~LPEEnvelopePerspective();

    virtual void doEffect(SPCurve *curve);

    virtual Geom::Point project_point(Geom::Point p);

    virtual Geom::Point project_point(Geom::Point p,  double m[][3]);

    Geom::Point pointAtRatio(Geom::Coord ratio,Geom::Point A, Geom::Point B);

    virtual void resetDefaults(SPItem const* item);

    virtual void doBeforeEffect(SPLPEItem const* lpeitem);

    virtual Gtk::Widget * newWidget();

    virtual void calculateCurve(Geom::Point a,Geom::Point b, SPCurve *c, bool horizontal, bool move);

    virtual void setDefaults();

    virtual void resetGrid();

    //virtual void original_bbox(SPLPEItem const* lpeitem, bool absolute = false);

    //virtual void addCanvasIndicators(SPLPEItem const*/*lpeitem*/, std::vector<Geom::PathVector> &/*hp_vec*/);

    //virtual std::vector<Geom::PathVector> getHelperPaths(SPLPEItem const* lpeitem);
protected:
    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec);
private:

    EnumParam<unsigned> deform_type;
    PointReseteableParam Up_Left_Point;
    PointReseteableParam Up_Right_Point;
    PointReseteableParam Down_Left_Point;
    PointReseteableParam Down_Right_Point;

    LPEEnvelopePerspective(const LPEEnvelopePerspective&);
    LPEEnvelopePerspective& operator=(const LPEEnvelopePerspective&);
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif
