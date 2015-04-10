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
    KnotHolderEntityCenterMirrorSymmetry(LPEMirrorSymmetry *effect) : LPEKnotHolderEntity(effect)
    {
        this->knoth = effect->knoth;
    };
    virtual ~KnotHolderEntityCenterMirrorSymmetry()
    {
        this->knoth = NULL;
    }
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state);
    virtual Geom::Point knot_get() const;
private:
    KnotHolder *knoth;
};

} // namespace MS

LPEMirrorSymmetry::LPEMirrorSymmetry(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    mode(_("Mode"), _("Symetry move mode"), "mode", MTConverter, &wr, this, MT_FREE),
    discard_orig_path(_("Discard original path?"), _("Check this to only keep the mirrored part of the path"), "discard_orig_path", &wr, this, false),
    fusion_paths(_("Fusioned symetry"), _("Fusion right side whith symm"), "fusion_paths", &wr, this, true),
    reverse_fusion(_("Reverse fusion"), _("Reverse fusion"), "reverse_fusion", &wr, this, false),
    reflection_line(_("Reflection line:"), _("Line which serves as 'mirror' for the reflection"), "reflection_line", &wr, this, "M0,0 L1,0"),
    center(_("Center of mirroring (X or Y)"), _("Center of the mirror"), "center", &wr, this, "Adjust the center of mirroring"),
    knoth(NULL)
{
    show_orig_path = true;

    registerParameter(&mode);
    registerParameter( &discard_orig_path);
    registerParameter( &fusion_paths);
    registerParameter( &reverse_fusion);
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
    Point point_a(boundingbox_X.max(), boundingbox_Y.min());
    Point point_b(boundingbox_X.max(), boundingbox_Y.max());
    Point point_c(boundingbox_X.max(), boundingbox_Y.middle());
    if(mode == MT_Y) {
        point_a = Geom::Point(boundingbox_X.min(),center[Y]);
        point_b = Geom::Point(boundingbox_X.max(),center[Y]);
    }
    if(mode == MT_X) {
        point_a = Geom::Point(center[X],boundingbox_Y.min());
        point_b = Geom::Point(center[X],boundingbox_Y.max());
    }
    if( mode == MT_X || mode == MT_Y ) {
        Geom::Path path;
        path.start( point_a );
        path.appendNew<Geom::LineSegment>( point_b );
        reflection_line.set_new_value(path.toPwSb(), true);
        line_separation.setPoints(point_a, point_b);
        center.param_setValue(path.pointAt(0.5));
        if(knoth) {
            knoth->update_knots();
        }
    } else if( mode == MT_FREE) {
        std::vector<Geom::Path> line_m(reflection_line.get_pathvector());
        if(!are_near(previous_center,center, 0.01)) {
            Geom::Point trans = center - line_m[0].pointAt(0.5);
            line_m[0] *= Geom::Affine(1,0,0,1,trans[X],trans[Y]);
            point_a = line_m[0].initialPoint();
            point_b = line_m[0].finalPoint();
            reflection_line.set_new_value(line_m[0].toPwSb(), true);
            line_separation.setPoints(point_a, point_b);
        } else {
            center.param_setValue(line_m[0].pointAt(0.5));
            point_a = line_m[0].initialPoint();
            point_b = line_m[0].finalPoint();
            line_separation.setPoints(point_a, point_b);
            if(knoth) {
                knoth->update_knots();
            }
        }
        previous_center = center;
    }

    item->apply_to_clippath(item);
    item->apply_to_mask(item);
}

void
LPEMirrorSymmetry::doOnApply (SPLPEItem const* lpeitem)
{
    using namespace Geom;

    original_bbox(lpeitem);

    Point point_a(boundingbox_X.max(), boundingbox_Y.min());
    Point point_b(boundingbox_X.max(), boundingbox_Y.max());
    Point point_c(boundingbox_X.max(), boundingbox_Y.middle());
    Geom::Path path;
    path.start( point_a );
    path.appendNew<Geom::LineSegment>( point_b );
    reflection_line.set_new_value(path.toPwSb(), true);
    center.param_setValue(point_c);
    previous_center = center;
}

int
LPEMirrorSymmetry::pointSideOfLine(Geom::Point point_a, Geom::Point point_b, Geom::Point point_x)
{
    //http://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
    double pos =  (point_b[Geom::X]-point_a[Geom::X])*(point_x[Geom::Y]-point_a[Geom::Y]) - (point_b[Geom::Y]-point_a[Geom::Y])*(point_x[Geom::X]-point_a[Geom::X]);
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
    Geom::Path line_m_expanded;
    Geom::Point line_start = line_separation.pointAt(-100000.0);
    Geom::Point line_end = line_separation.pointAt(100000.0);
    line_m_expanded.start( line_start);
    line_m_expanded.appendNew<Geom::LineSegment>( line_end);

    if (!discard_orig_path && !fusion_paths) {
        path_out = path_in;
    }

    Geom::Point point_a(line_start);
    Geom::Point point_b(line_end);

    Geom::Affine m1(1.0, 0.0, 0.0, 1.0, point_a[0], point_a[1]);
    double hyp = Geom::distance(point_a, point_b);
    double c = (point_b[0] - point_a[0]) / hyp; // cos(alpha)
    double s = (point_b[1] - point_a[1]) / hyp; // sin(alpha)

    Geom::Affine m2(c, -s, s, c, 0.0, 0.0);
    Geom::Affine sca(1.0, 0.0, 0.0, -1.0, 0.0, 0.0);

    Geom::Affine m = m1.inverse() * m2;
    m = m * sca;
    m = m * m2.inverse();
    m = m * m1;

    if(fusion_paths && !discard_orig_path) {
        for (Geom::PathVector::const_iterator path_it = original_pathv.begin();
                path_it != original_pathv.end(); ++path_it) {
            if (path_it->empty()) {
                continue;
            }
            std::vector<Geom::Path> temp_path;
            double time_start = 0.0;
            int position = 0;
            bool end_open = false;
            if (path_it->closed()) {
                const Geom::Curve &closingline = path_it->back_closed();
                if (!are_near(closingline.initialPoint(), closingline.finalPoint())) {
                    end_open = true;
                }
            }
            Geom::Path original = (Geom::Path)(*path_it);
            if(end_open && path_it->closed()) {
                original.close(false);
                original.appendNew<Geom::LineSegment>( original.initialPoint() );
                original.close(true);
            }
            Geom::Crossings cs = crossings(original, line_m_expanded);
            for(unsigned int i = 0; i < cs.size(); i++) {
                double timeEnd = cs[i].ta;
                Geom::Path portion = original.portion(time_start, timeEnd);
                Geom::Point middle = portion.pointAt((double)portion.size()/2.0);
                position = pointSideOfLine(line_start, line_end, middle);
                if(reverse_fusion) {
                    position *= -1;
                }
                if(position == 1) {
                    Geom::Path mirror = portion.reverse() * m;
                    mirror.setInitial(portion.finalPoint());
                    portion.append(mirror);
                    if(i!=0) {
                        portion.setFinal(portion.initialPoint());
                        portion.close();
                    }
                    temp_path.push_back(portion);
                }
                portion.clear();
                time_start = timeEnd;
            }
            position = pointSideOfLine(line_start, line_end, original.finalPoint());
            if(reverse_fusion) {
                position *= -1;
            }
            if(cs.size()!=0 && position == 1) {
                Geom::Path portion = original.portion(time_start, original.size());
                portion = portion.reverse();
                Geom::Path mirror = portion.reverse() * m;
                mirror.setInitial(portion.finalPoint());
                portion.append(mirror);
                portion = portion.reverse();
                if (!original.closed()) {
                    temp_path.push_back(portion);
                } else {
                    if(cs.size() >1 ) {
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
            if(cs.size() == 0 && position == 1) {
                temp_path.push_back(original);
                temp_path.push_back(original * m);
            }
            path_out.insert(path_out.end(), temp_path.begin(), temp_path.end());
            temp_path.clear();
        }
    }

    if (!fusion_paths || discard_orig_path) {
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
    Geom::Path line_m_expanded;
    Geom::Point line_start = line_separation.pointAt(-100000.0);
    Geom::Point line_end = line_separation.pointAt(100000.0);
    line_m_expanded.start( line_start);
    line_m_expanded.appendNew<Geom::LineSegment>( line_end);
    pathv.push_back(line_m_expanded);
    hp_vec.push_back(pathv);
}

void
LPEMirrorSymmetry::addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop, SPItem *item)
{
    {
        knoth = knotholder;
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
