/** \file
 * LPE <mirror_symmetry> implementation: mirrors a path with respect to a given line.
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *   Abhishek Sharma
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilin Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "live_effects/lpe-mirror_symmetry.h"
#include <sp-path.h>
#include <display/curve.h>
#include <svg/path-string.h>
#include "helper/geom.h"
#include <2geom/path.h>
#include <2geom/path-intersection.h>
#include <2geom/transforms.h>
#include <2geom/affine.h>
#include "knot-holder-entity.h"
#include "knotholder.h"

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<ModeType> ModeTypeData[MT_END] = {
    { MT_FREE, N_("Free from reflection line"), "Free from path" },
    { MT_X, N_("X from middle knot"), "X from middle knot" },
    { MT_Y, N_("Y from middle knot"), "Y from middle knot" }
};
static const Util::EnumDataConverter<ModeType>
MTConverter(ModeTypeData, MT_END);

namespace MS {

class KnotHolderEntityCenterMirrorSymmetry : public LPEKnotHolderEntity {
public:
    KnotHolderEntityCenterMirrorSymmetry(LPEMirrorSymmetry *effect) : LPEKnotHolderEntity(effect) {};
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
    virtual Geom::Point knot_get() const;
};

} // namespace MS

LPEMirrorSymmetry::LPEMirrorSymmetry(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    mode(_("Mode"), _("Symetry move mode"), "mode", MTConverter, &wr, this, MT_FREE),
    discard_orig_path(_("Discard original path?"), _("Check this to only keep the mirrored part of the path"), "discard_orig_path", &wr, this, false),
    fusionPaths(_("Fusioned symetry"), _("Fusion right side whith symm"), "fusionPaths", &wr, this, true),
    reverseFusion(_("Reverse fusion"), _("Reverse fusion"), "reverseFusion", &wr, this, false),
    reflection_line(_("Reflection line:"), _("Line which serves as 'mirror' for the reflection"), "reflection_line", &wr, this, "M0,0 L1,0"),
    center(_("Center of mirroring (X or Y)"), _("Center of the mirror"), "center", &wr, this, "Adjust the center of mirroring")
{
    show_orig_path = true;

    registerParameter(&mode);
    registerParameter( &discard_orig_path);
    registerParameter( &fusionPaths);
    registerParameter( &reverseFusion);
    registerParameter( &reflection_line);
    registerParameter( &center);

}

LPEMirrorSymmetry::~LPEMirrorSymmetry()
{
}

void
LPEMirrorSymmetry::doBeforeEffect (SPLPEItem const* lpeitem)
{
    using namespace Geom;

    SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
    Point A(boundingbox_X.max(), boundingbox_Y.min());
    Point B(boundingbox_X.max(), boundingbox_Y.max());
    Point C(boundingbox_X.max(), boundingbox_Y.middle());
    if(mode == MT_Y){
        A = Geom::Point(boundingbox_X.min(),center[Y]);
        B = Geom::Point(boundingbox_X.max(),center[Y]);
    }
    if(mode == MT_X){
        A = Geom::Point(center[X],boundingbox_Y.min());
        B = Geom::Point(center[X],boundingbox_Y.max());
    }
    if( mode == MT_X || mode == MT_Y ){
        Geom::Path path;
        path.start( A );
        path.appendNew<Geom::LineSegment>( B );
        reflection_line.set_new_value(path.toPwSb(), true);
        lineSeparation.setPoints(A,B);
        center.param_setValue(path.pointAt(0.5));
        if(knot_holder){
            knot_holder->update_knots();
        }
    } else if( mode == MT_FREE) {
        std::vector<Geom::Path> mline(reflection_line.get_pathvector());
        if(!are_near(previousCenter,center, 0.01)){
            Geom::Point trans = center - mline[0].pointAt(0.5);
            mline[0] *= Geom::Affine(1,0,0,1,trans[X],trans[Y]);
            A = mline[0].initialPoint();
            B = mline[0].finalPoint();
            reflection_line.set_new_value(mline[0].toPwSb(), true);
            lineSeparation.setPoints(A,B);
        } else {
            center.param_setValue(mline[0].pointAt(0.5));
            A = mline[0].initialPoint();
            B = mline[0].finalPoint();
            lineSeparation.setPoints(A,B);
            if(knot_holder){
                knot_holder->update_knots();
            }
        }
        previousCenter = center;
    }

    item->apply_to_clippath(item);
    item->apply_to_mask(item);
}

void
LPEMirrorSymmetry::doOnApply (SPLPEItem const* lpeitem)
{
    using namespace Geom;

    original_bbox(lpeitem);

    Point A(boundingbox_X.max(), boundingbox_Y.min());
    Point B(boundingbox_X.max(), boundingbox_Y.max());
    Point C(boundingbox_X.max(), boundingbox_Y.middle());
    Geom::Path path;
    path.start( A );
    path.appendNew<Geom::LineSegment>( B );
    reflection_line.set_new_value(path.toPwSb(), true);
    center.param_setValue(C);
    previousCenter = center;
}

int 
LPEMirrorSymmetry::pointSideOfLine(Geom::Point A, Geom::Point B, Geom::Point X)
{
    //http://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
    double pos =  (B[Geom::X]-A[Geom::X])*(X[Geom::Y]-A[Geom::Y]) - (B[Geom::Y]-A[Geom::Y])*(X[Geom::X]-A[Geom::X]);
    return (pos < 0) ? -1 : (pos > 0);
}

std::vector<Geom::Path>
LPEMirrorSymmetry::doEffect_path (std::vector<Geom::Path> const & path_in)
{
    // Don't allow empty path parameter:
    if ( reflection_line.get_pathvector().empty() ) {
        return path_in;
    }
    Geom::PathVector const original_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    std::vector<Geom::Path> path_out;
    Geom::Path mlineExpanded;
    Geom::Point lineStart = lineSeparation.pointAt(-100000.0);
    Geom::Point lineEnd = lineSeparation.pointAt(100000.0);
    mlineExpanded.start( lineStart);
    mlineExpanded.appendNew<Geom::LineSegment>( lineEnd);

    if (!discard_orig_path && !fusionPaths) {
        path_out = path_in;
    }

    Geom::Point A(lineStart);
    Geom::Point B(lineEnd);

    Geom::Affine m1(1.0, 0.0, 0.0, 1.0, A[0], A[1]);
    double hyp = Geom::distance(A, B);
    double c = (B[0] - A[0]) / hyp; // cos(alpha)
    double s = (B[1] - A[1]) / hyp; // sin(alpha)

    Geom::Affine m2(c, -s, s, c, 0.0, 0.0);
    Geom::Affine sca(1.0, 0.0, 0.0, -1.0, 0.0, 0.0);

    Geom::Affine m = m1.inverse() * m2;
    m = m * sca;
    m = m * m2.inverse();
    m = m * m1;
    
    if(fusionPaths && !discard_orig_path){
        for (Geom::PathVector::const_iterator path_it = original_pathv.begin();
            path_it != original_pathv.end(); ++path_it) {
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
                if(reverseFusion){
                    position *= -1;
                }
                if(position == 1){
                    Geom::Path mirror = portion.reverse() * m;
                    mirror.setInitial(portion.finalPoint());
                    portion.append(mirror);
                    if(i!=0){
                        portion.setFinal(portion.initialPoint());
                        portion.close();
                    }
                    temp_path.push_back(portion);
                }
                portion.clear();
                timeStart = timeEnd;
            }
            position = pointSideOfLine(lineStart, lineEnd, original.finalPoint());
            if(reverseFusion){
                position *= -1;
            }
            if(cs.size()!=0 && position == 1){
                Geom::Path portion = original.portion(timeStart, original.size());
                portion = portion.reverse();
                Geom::Path mirror = portion.reverse() * m;
                mirror.setInitial(portion.finalPoint());
                portion.append(mirror);
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
    }
    
    if (!fusionPaths || discard_orig_path) {
        for (int i = 0; i < static_cast<int>(path_in.size()); ++i) {
            path_out.push_back(path_in[i] * m);
        }
    }

    return path_out;
}

void
LPEMirrorSymmetry::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    using namespace Geom;

    PathVector pathv;
    Geom::Path mlineExpanded;
    Geom::Point lineStart = lineSeparation.pointAt(-100000.0);
    Geom::Point lineEnd = lineSeparation.pointAt(100000.0);
    mlineExpanded.start( lineStart);
    mlineExpanded.appendNew<Geom::LineSegment>( lineEnd);
    pathv.push_back(mlineExpanded);
    hp_vec.push_back(pathv);
}

void 
LPEMirrorSymmetry::addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop, SPItem *item) {
    {
        KnotHolderEntity *e = new MS::KnotHolderEntityCenterMirrorSymmetry(this);
        e->create( desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN,
                   _("Adjust the center") );
        knotholder->add(e);
    }

};

namespace MS {

using namespace Geom;

void
KnotHolderEntityCenterMirrorSymmetry::knot_set(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    LPEMirrorSymmetry* lpe = dynamic_cast<LPEMirrorSymmetry *>(_effect);
    Geom::Point const s = snap_knot_position(p, state);
    lpe->center.param_setValue(s);

    // FIXME: this should not directly ask for updating the item. It should write to SVG, which triggers updating.
    sp_lpe_item_update_patheffect (SP_LPE_ITEM(item), false, true);
}

Geom::Point
KnotHolderEntityCenterMirrorSymmetry::knot_get() const
{
    LPEMirrorSymmetry const *lpe = dynamic_cast<LPEMirrorSymmetry const*>(_effect);
    return lpe->center;
}

} // namespace CR

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
