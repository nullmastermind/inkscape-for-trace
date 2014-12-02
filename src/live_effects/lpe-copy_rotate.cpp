/** \file
 * LPE <copy_rotate> implementation
 */
/*
 * Authors:
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Johan Engelen <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Copyright (C) Authors 2007-2012
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glibmm/i18n.h>
#include <gdk/gdk.h>

#include "live_effects/lpe-copy_rotate.h"
#include "sp-shape.h"
#include "display/curve.h"

#include <2geom/path.h>
#include <2geom/transforms.h>
#include <2geom/d2-sbasis.h>
#include <2geom/angle.h>

#include "knot-holder-entity.h"
#include "knotholder.h"

namespace Inkscape {
namespace LivePathEffect {

namespace CR {

class KnotHolderEntityStartingAngle : public LPEKnotHolderEntity {
public:
    KnotHolderEntityStartingAngle(LPECopyRotate *effect) : LPEKnotHolderEntity(effect) {};
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
    virtual Geom::Point knot_get() const;
};


} // namespace CR

LPECopyRotate::LPECopyRotate(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    starting_angle(_("Starting:"), _("Angle of the first copy"), "starting_angle", &wr, this, 0.0),
    rotation_angle(_("Rotation angle:"), _("Angle between two successive copies"), "rotation_angle", &wr, this, 30.0),
    num_copies(_("Number of copies:"), _("Number of copies of the original path"), "num_copies", &wr, this, 5),
    copiesTo360(_("360º Copies"), _("No rotation angle, fixed to 360º"), "copiesTo360", &wr, this, true),
    fusionPaths(_("Fusioned paths"), _("Fusion paths"), "fusionPaths", &wr, this, true),

    origin(_("Origin"), _("Origin of the rotation"), "origin", &wr, this, "Adjust the origin of the rotation"),
    dist_angle_handle(100)
{
    show_orig_path = true;
    _provides_knotholder_entities = true;

    // register all your parameters here, so Inkscape knows which parameters this effect has:
    registerParameter( dynamic_cast<Parameter *>(&copiesTo360) );
    registerParameter( dynamic_cast<Parameter *>(&fusionPaths) );
    registerParameter( dynamic_cast<Parameter *>(&starting_angle) );
    registerParameter( dynamic_cast<Parameter *>(&rotation_angle) );
    registerParameter( dynamic_cast<Parameter *>(&num_copies) );
    registerParameter( dynamic_cast<Parameter *>(&origin) );

    num_copies.param_make_integer(true);
    num_copies.param_set_range(0, 1000);
}

LPECopyRotate::~LPECopyRotate()
{

}

void
LPECopyRotate::doOnApply(SPLPEItem const* lpeitem)
{
    using namespace Geom;

    original_bbox(lpeitem);

    Point A(boundingbox_X.min(), boundingbox_Y.middle());
    Point B(boundingbox_X.max(), boundingbox_Y.middle());
    Point C(boundingbox_X.middle(), boundingbox_Y.middle());
    origin.param_setValue(A);

    dir = unit_vector(B - A);
    dist_angle_handle = L2(B - A);
}

Geom::Piecewise<Geom::D2<Geom::SBasis> >
LPECopyRotate::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in)
{
    using namespace Geom;

    // I first suspected the minus sign to be a bug in 2geom but it is
    // likely due to SVG's choice of coordinate system orientation (max)
    start_pos = origin + dir * Rotate(-deg_to_rad(starting_angle)) * dist_angle_handle;
    double rotation_angle_end = rotation_angle;
    if(copiesTo360){
        rotation_angle_end = 360.0/(double)num_copies;
    }
    rot_pos = origin + dir * Rotate(-deg_to_rad(starting_angle + rotation_angle_end)) * dist_angle_handle;

    A = pwd2_in.firstValue();
    B = pwd2_in.lastValue();
    dir = unit_vector(B - A);

    Piecewise<D2<SBasis> > output;

    Affine pre = Translate(-origin) * Rotate(-deg_to_rad(starting_angle));
    if(fusionPaths){
            // I first suspected the minus sign to be a bug in 2geom but it is
            // likely due to SVG's choice of coordinate system orientation (max)
            Rotate rot(-deg_to_rad(-starting_angle));
            Rotate rot2(-deg_to_rad(rotation_angle_end-starting_angle));
            //Affine t = pre * rot * Translate(origin);
            Geom::Path mlineExpanded;
            Geom::Point lineStart(0,0);
            Geom::Point lineStart[Geom::X] = cos(rot) * 100000.0;
            Geom::Point lineStart[Geom::Y] = sin(rot) * 100000.0;
            Geom::Point lineEnd(0,0);
            Geom::Point lineEnd[Geom::X] = cos(rot2) * 100000.0;
            Geom::Point lineEnd[Geom::Y] = sin(rot2) * 100000.0;
            mlineExpanded.start( lineStart);
            mlineExpanded.appendNew<Geom::LineSegment>( origin);
            mlineExpanded.appendNew<Geom::LineSegment>( lineEnd);
            PathVector const original_pathv = path_from_piecewise(remove_short_cuts(pwd2_in, 0.1), 0.001);
            for (Geom::PathVector::const_iterator path_it = original_pathv.begin(); path_it != original_pathv.end(); ++path_it) {
                if (path_it->empty()){
                    continue;
                }
                std::vector<Geom::Path> temp_path;
                double timeStart = 0.0;
                int position = 0;
                bool end_open = false;
                if (path_it->closed()) {
                    const Geom::Curve &closingline = path_it->back_closed();
                    if (!are_near(closingline.initialPoint(), closingline.finalPoint())) {
                        end_open = true;
                    }
                }
                Geom::Path original = (Geom::Path)(*path_it);
                if(end_open && path_it->closed()){
                    original.close(false);
                    original.appendNew<Geom::LineSegment>( original.initialPoint() );
                    original.close(true);
                }
                Geom::Crossings cs = crossings(original, mlineExpanded);
                for(unsigned int i = 0; i < cs.size(); i++) {
                    double timeEnd = cs[i].ta;
                    Geom::Path portion = original.portion(timeStart, timeEnd);
                    Geom::Point middle = portion.pointAt((double)portion.size()/2.0);
                    position = pointSideOfLine(lineStart, lineEnd, middle);
                    Geom::line middleLine;
                    middleLine->setPoints(origin,middle);
                    if(middleLine.angle > rot && middleLine < rot2){
                        Geom::Path kaleidoscope;
                        for (int j = 0; j < num_copies; ++j) {
                            // I first suspected the minus sign to be a bug in 2geom but it is
                            // likely due to SVG's choice of coordinate system orientation (max)
                            Rotate rot(-deg_to_rad(rotation_angle_end * j));
                            Affine t = pre * rot * Translate(origin);
                            kaleidoscope = portion.reverse() *  t);
                            kaleidoscope.setInitial(portion.finalPoint());
                            portion.append(kaleidoscope);
                        }
                        if(i!=0){
                            portion.setFinal(portion.initialPoint());
                            portion.close();
                        }
                        temp_path.push_back(portion);
                    }
                    portion.clear();
                    timeStart = timeEnd;
                }
                Geom::line middleLine;
                middleLine->setPoints(origin,original.finalPoint());
                if(cs.size()!=0 && middleLine.angle > rot && middleLine < rot2){
                    Geom::Path portion = original.portion(timeStart, original.size());
                    portion = portion.reverse();
                    
                    Geom::Path kaleidoscope;
                    for (int i = 0; i < num_copies; ++i) {
                        // I first suspected the minus sign to be a bug in 2geom but it is
                        // likely due to SVG's choice of coordinate system orientation (max)
                        Rotate rot(-deg_to_rad(rotation_angle_end * i));
                        Affine t = pre * rot * Translate(origin);
                        kaleidoscope = portion.reverse() *  t);
                        kaleidoscope.setInitial(portion.finalPoint());
                        portion.append(kaleidoscope);
                    }
                    portion = portion.reverse();
                    if (!original.closed()){
                        temp_path.push_back(portion);
                    } else {
                        if(cs.size() >1 ){
                            portion.setFinal(temp_path[0].initialPoint());
                            portion.setInitial(temp_path[0].finalPoint());
                            temp_path[0].append(portion);
                        } else {
                            temp_path.push_back(portion);
                        }
                        temp_path[0].close();
                    }
                    portion.clear();
                }
                if(cs.size() == 0 && position == 1){
                    temp_path.push_back(original);
                    temp_path.push_back(original * m);
                }
                path_out.insert(path_out.end(), temp_path.begin(), temp_path.end());
                temp_path.clear();
            }
            output = path_out.toPwSb();
        } else {
            for (int i = 0; i < num_copies; ++i) {
                // I first suspected the minus sign to be a bug in 2geom but it is
                // likely due to SVG's choice of coordinate system orientation (max)
                Rotate rot(-deg_to_rad(rotation_angle_end * i));
                Affine t = pre * rot * Translate(origin);
                output.concat(pwd2_in * t);
            }
        }
    }
    return output;
}

void
LPECopyRotate::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    using namespace Geom;

    Path path(start_pos);
    path.appendNew<LineSegment>((Geom::Point) origin);
    path.appendNew<LineSegment>(rot_pos);
    PathVector pathv;
    pathv.push_back(path);
    hp_vec.push_back(pathv);
}


void LPECopyRotate::addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop, SPItem *item) {
    {
        KnotHolderEntity *e = new CR::KnotHolderEntityStartingAngle(this);
        e->create( desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN,
                   _("Adjust the starting angle") );
        knotholder->add(e);
    }
};

namespace CR {

using namespace Geom;

void
KnotHolderEntityStartingAngle::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, guint state)
{
    LPECopyRotate* lpe = dynamic_cast<LPECopyRotate *>(_effect);

    Geom::Point const s = snap_knot_position(p, state);

    // I first suspected the minus sign to be a bug in 2geom but it is
    // likely due to SVG's choice of coordinate system orientation (max)
    lpe->starting_angle.param_set_value(rad_to_deg(-angle_between(lpe->dir, s - lpe->origin)));
    if (state & GDK_SHIFT_MASK) {
        lpe->dist_angle_handle = L2(lpe->B - lpe->A);
    } else {
        lpe->dist_angle_handle = L2(p - lpe->origin);
    }

    // FIXME: this should not directly ask for updating the item. It should write to SVG, which triggers updating.
    sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, true);
}

Geom::Point
KnotHolderEntityStartingAngle::knot_get() const
{
    LPECopyRotate const *lpe = dynamic_cast<LPECopyRotate const*>(_effect);
    return lpe->start_pos;
}

} // namespace CR



/* ######################## */

} //namespace LivePathEffect
} /* namespace Inkscape */

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
