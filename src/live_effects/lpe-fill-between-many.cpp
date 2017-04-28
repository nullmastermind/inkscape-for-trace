/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "live_effects/lpe-fill-between-many.h"

#include "display/curve.h"
#include "sp-shape.h"
#include "sp-text.h"
#include "svg/svg.h"
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEFillBetweenMany::LPEFillBetweenMany(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    linked_paths(_("Linked path:"), _("Paths from which to take the original path data"), "linkedpaths", &wr, this),
    fuse(_("Fuse coincident points"), _("Fuse coincident points"), "fuse", &wr, this, false),
    allow_transforms(_("Allow transforms"), _("Allow transforms"), "allow_transforms", &wr, this, false),
    join(_("Join subpaths"), _("Join subpaths"), "join", &wr, this, true),
    close(_("Close"), _("Close path"), "close", &wr, this, true)
{
    registerParameter(&linked_paths);
    registerParameter(&fuse);
    registerParameter(&allow_transforms);
    registerParameter(&join);
    registerParameter(&close);
    transformmultiply = false;
}

LPEFillBetweenMany::~LPEFillBetweenMany()
{

}

void LPEFillBetweenMany::doEffect (SPCurve * curve)
{
    Geom::PathVector res_pathv;
    SPItem * firstObj = NULL;
    for (std::vector<PathAndDirection*>::iterator iter = linked_paths._vector.begin(); iter != linked_paths._vector.end(); ++iter) {
        SPObject *obj;
        if ((*iter)->ref.isAttached() && (obj = (*iter)->ref.getObject()) && SP_IS_ITEM(obj) && !(*iter)->_pathvector.empty()) {
            Geom::Path linked_path;
            if ((*iter)->reversed) {
                linked_path = (*iter)->_pathvector.front().reversed();
            } else {
                linked_path = (*iter)->_pathvector.front();
            }
            
            if (!res_pathv.empty() && join) {
                linked_path = linked_path * SP_ITEM(obj)->getRelativeTransform(firstObj);
                if (!are_near(res_pathv.front().finalPoint(), linked_path.initialPoint(), 0.01) || !fuse) {
                    res_pathv.front().appendNew<Geom::LineSegment>(linked_path.initialPoint());
                } else {
                    linked_path.setInitial(res_pathv.front().finalPoint());
                }
                res_pathv.front().append(linked_path);
            } else {
                firstObj = SP_ITEM(obj);
                if (close && !join) {
                    linked_path.close();
                }
                res_pathv.push_back(linked_path);
            }
        }
    }
    if (!res_pathv.empty() && close) {
        res_pathv.front().close();
    }
    if (res_pathv.empty()) {
        res_pathv = curve->get_pathvector();
    }
    if(!allow_transforms && !transformmultiply) {
        Geom::Affine affine = Geom::identity();
        sp_svg_transform_read(SP_ITEM(sp_lpe_item)->getAttribute("transform"), &affine);
        res_pathv *= affine.inverse();
    }
    if(transformmultiply) {
        transformmultiply = false;
    }
    curve->set_pathvector(res_pathv);
}

void
LPEFillBetweenMany::transform_multiply(Geom::Affine const& postmul, bool set)
{
    if(!allow_transforms && sp_lpe_item) {
        SP_ITEM(sp_lpe_item)->transform *= postmul.inverse();
        transformmultiply = true;
        sp_lpe_item_update_patheffect(sp_lpe_item, false, false);
    }
}

} // namespace LivePathEffect
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
