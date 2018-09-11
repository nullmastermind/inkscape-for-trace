// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_VONKOCH_H
#define INKSCAPE_LPE_VONKOCH_H

/*
 * Inkscape::LPEVonKoch
 *
 * Copyright (C) JF Barraud 2007 <jf.barraud@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/lpegroupbbox.h"
#include "live_effects/parameter/path.h"
#include "live_effects/parameter/point.h"
#include "live_effects/parameter/bool.h"

namespace Inkscape {
namespace LivePathEffect {

class VonKochPathParam : public PathParam{
public:
    VonKochPathParam ( const Glib::ustring& label,
		       const Glib::ustring& tip,
		       const Glib::ustring& key,
		       Inkscape::UI::Widget::Registry* wr,
		       Effect* effect,
		       const gchar * default_value = "M0,0 L1,1"):PathParam(label,tip,key,wr,effect,default_value){}
    ~VonKochPathParam() override= default;
    void param_setup_nodepath(Inkscape::NodePath::Path *np) override;  
  };

  //FIXME: a path is used here instead of 2 points to work around path/point param incompatibility bug.
class VonKochRefPathParam : public PathParam{
public:
    VonKochRefPathParam ( const Glib::ustring& label,
		       const Glib::ustring& tip,
		       const Glib::ustring& key,
		       Inkscape::UI::Widget::Registry* wr,
		       Effect* effect,
		       const gchar * default_value = "M0,0 L1,1"):PathParam(label,tip,key,wr,effect,default_value){}
    ~VonKochRefPathParam() override= default;
    void param_setup_nodepath(Inkscape::NodePath::Path *np) override;  
    bool param_readSVGValue(const gchar * strvalue) override;  
  };
 
class LPEVonKoch : public Effect, GroupBBoxEffect {
public:
    LPEVonKoch(LivePathEffectObject *lpeobject);
    ~LPEVonKoch() override;

    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;

    void resetDefaults(SPItem const* item) override;

    void doBeforeEffect(SPLPEItem const* item) override;

    //Useful??
    //    protected: 
    //virtual void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec); 

private:
    ScalarParam  nbgenerations;
    VonKochPathParam    generator;
    BoolParam    similar_only;
    BoolParam    drawall;
    //BoolParam    draw_boxes;
    //FIXME: a path is used here instead of 2 points to work around path/point param incompatibility bug.
    VonKochRefPathParam    ref_path;
    //    PointParam   refA;
    //    PointParam   refB;
    ScalarParam  maxComplexity;

    LPEVonKoch(const LPEVonKoch&) = delete;
    LPEVonKoch& operator=(const LPEVonKoch&) = delete;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
