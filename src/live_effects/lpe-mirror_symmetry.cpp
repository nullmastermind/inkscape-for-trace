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

#include <glibmm/i18n.h>

#include "live_effects/lpe-mirror_symmetry.h"
#include <sp-path.h>
#include <display/curve.h>
#include <svg/path-string.h>

#include <2geom/path.h>
#include <2geom/path-intersection.h>
#include <2geom/transforms.h>
#include <2geom/affine.h>

namespace Inkscape {
namespace LivePathEffect {

LPEMirrorSymmetry::LPEMirrorSymmetry(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    discard_orig_path(_("Discard original path?"), _("Check this to only keep the mirrored part of the path"), "discard_orig_path", &wr, this, false),
    joinPaths(_("Join the paths"), _("Join the resulting paths"), "joinPaths", &wr, this, true),
    reflection_line(_("Reflection line:"), _("Line which serves as 'mirror' for the reflection"), "reflection_line", &wr, this, "M0,0 L1,0")
{
    show_orig_path = true;

    registerParameter( dynamic_cast<Parameter *>(&discard_orig_path) );
    registerParameter( dynamic_cast<Parameter *>(&joinPaths) );
    registerParameter( dynamic_cast<Parameter *>(&reflection_line) );
}

LPEMirrorSymmetry::~LPEMirrorSymmetry()
{
}

void
LPEMirrorSymmetry::doBeforeEffect (SPLPEItem const* lpeitem)
{
    SPLPEItem * item = const_cast<SPLPEItem*>(lpeitem);
    item->apply_to_clippath(item);
    item->apply_to_mask(item);
}

void
LPEMirrorSymmetry::doOnApply (SPLPEItem const* lpeitem)
{
    using namespace Geom;

    original_bbox(lpeitem);

    Point A(boundingbox_X.max(), boundingbox_Y.max());
    Point B(boundingbox_X.max(), boundingbox_Y.min());
    Piecewise<D2<SBasis> > rline = Piecewise<D2<SBasis> >(D2<SBasis>(Linear(A[X], B[X]), Linear(A[Y], B[Y])));
    reflection_line.set_new_value(rline, true);
}

std::vector<Geom::Path>
LPEMirrorSymmetry::doEffect_path (std::vector<Geom::Path> const & path_in)
{
    // Don't allow empty path parameter:
    if ( reflection_line.get_pathvector().empty() ) {
        return path_in;
    }

    std::vector<Geom::Path> path_out;
    std::vector<Geom::Path> mline(reflection_line.get_pathvector());

    if (!discard_orig_path && !joinPaths) {
        path_out = path_in;
    }

    Geom::Point A(mline.front().initialPoint());
    Geom::Point B(mline.back().finalPoint());

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
    
    if(joinPaths && !discard_orig_path){
        for (Geom::PathVector::const_iterator path_it = path_in.begin();
            path_it != path_in.end(); ++path_it) {
            if (path_it->empty()){
                continue;
            }
            Geom::Path original;
            Geom::Path mlineExpanded;
            Geom::Line lineSeparation;
            lineSeparation.setPoints(mline[0].initialPoint(),mline[0].finalPoint());
            Geom::Point lineStart = lineSeparation.pointAt(-100000.0);
            Geom::Point lineEnd = lineSeparation.pointAt(100000.0);
            mlineExpanded.start( lineStart);
            mlineExpanded.appendNew<Geom::LineSegment>( lineEnd);
            Geom::Crossings cs = crossings(*path_it, mlineExpanded);
            double timeStart = 0.0;
            //http://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
            double pos =  (lineEnd[Geom::X]-lineStart[Geom::X])*(path_it->initialPoint()[Geom::Y]-lineStart[Geom::Y]) - (lineEnd[Geom::Y]-lineStart[Geom::Y])*(path_it->initialPoint()[Geom::X]-lineStart[Geom::X]);
            int position = (pos < 0) ? -1 : (pos > 0);
            unsigned int counter = 0;

            for(unsigned int i = 0; i < cs.size(); i++) {
                double timeEnd = cs[i].ta;
                Geom::Path portion = path_it->portion(timeStart, timeEnd);
                if(position == -1 && i==0){
                    counter++;
                }
                if(counter%2!=0){
                    original = portion;
                    original.append(portion.reverse() * m, (Geom::Path::Stitching)1);
                    if(i!=0){
                        original.close();
                    }
                    path_out.push_back(original);
                    original.clear();
                }
                timeStart = timeEnd;
                counter++;
            }
            if(cs.size()!=0 && ((cs.size()%2 == 0 && position == -1)||(cs.size()%2 != 0 && position == 1))){
                Geom::Path portion = path_it->portion(timeStart, nearest_point(path_it->finalPoint(), *path_it));
                original = portion.reverse();
                original.append(portion * m, (Geom::Path::Stitching)1);
                if (!path_it->closed()){
                    path_out.push_back(original);
                } else {
                    path_out[0].append(original.reverse(), (Geom::Path::Stitching)1);
                    path_out[0].close();
                }
                original.clear();
            }
            if(cs.size() == 0 && position == -1){
                path_out.push_back(*path_it);
                path_out.push_back(*path_it * m);
            }
        }
    }
    
    if (!joinPaths) {
        for (int i = 0; i < static_cast<int>(path_in.size()); ++i) {
            path_out.push_back(path_in[i] * m);
        }
    }

    return path_out;
}

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
