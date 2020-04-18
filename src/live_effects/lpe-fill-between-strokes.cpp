// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/lpe-fill-between-strokes.h"

#include "display/curve.h"
#include "svg/svg.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEFillBetweenStrokes::LPEFillBetweenStrokes(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    linked_path(_("Linked path:"), _("Path from which to take the original path data"), "linkedpath", &wr, this),
    second_path(_("Second path:"), _("Second path from which to take the original path data"), "secondpath", &wr, this),
    reverse_second(_("Reverse Second"), _("Reverses the second path order"), "reversesecond", &wr, this),
    join(_("Join subpaths"), _("Join subpaths"), "join", &wr, this, true),
    close(_("Close"), _("Close path"), "close", &wr, this, true)
{
    registerParameter(&linked_path);
    registerParameter(&second_path);
    registerParameter(&reverse_second);
    registerParameter(&join);
    registerParameter(&close);
}

LPEFillBetweenStrokes::~LPEFillBetweenStrokes()
= default;

void LPEFillBetweenStrokes::doEffect (SPCurve * curve)
{
    if (curve) {
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        Inkscape::Selection *selection = nullptr;
        if (desktop) {
            selection = desktop->selection;
        }
        Geom::Affine transf = sp_item_transform_repr(sp_lpe_item);
        if (transf != Geom::identity()) {
            sp_lpe_item->doWriteTransform(Geom::identity());
        }
        if ( linked_path.linksToPath() && second_path.linksToPath() && linked_path.getObject() && second_path.getObject() ) {
            SPItem * linked1 = linked_path.getObject();
            if (linked1 && transf != Geom::identity() && selection && !selection->includes(linked1->getRepr())) {
                SP_ITEM(linked1)->doWriteTransform(transf);
            }
            Geom::PathVector linked_pathv = linked_path.get_pathvector();
            SPItem * linked2 = second_path.getObject();
            if (linked2 && transf != Geom::identity() && selection && !selection->includes(linked2->getRepr())) {
                SP_ITEM(linked2)->doWriteTransform(transf);
            }
            Geom::PathVector second_pathv = second_path.get_pathvector();
            Geom::PathVector result_linked_pathv;
            Geom::PathVector result_second_pathv;
            for (auto & iter : linked_pathv)
            {
                result_linked_pathv.push_back(iter);
            }
            for (auto & iter : second_pathv)
            {
                result_second_pathv.push_back(iter);
            }

            if ( !result_linked_pathv.empty() && !result_second_pathv.empty() && !result_linked_pathv.front().closed() ) {
                if (reverse_second.get_value()) {
                    result_second_pathv.front() = result_second_pathv.front().reversed();
                }
                if (join) {
                    if (!are_near(result_linked_pathv.front().finalPoint(), result_second_pathv.front().initialPoint(),0.1)) {
                        result_linked_pathv.front().appendNew<Geom::LineSegment>(result_second_pathv.front().initialPoint());
                    } else {
                        result_second_pathv.front().setInitial(result_linked_pathv.front().finalPoint());
                    }
                    result_linked_pathv.front().append(result_second_pathv.front());
                    if (close) {
                        result_linked_pathv.front().close();
                    }
                } else {
                    if (close) {
                        result_linked_pathv.front().close();
                        result_second_pathv.front().close();
                    }
                    result_linked_pathv.push_back(result_second_pathv.front());
                }
                curve->set_pathvector(result_linked_pathv);
            } else if ( !result_linked_pathv.empty() ) {
                curve->set_pathvector(result_linked_pathv);
            } else if ( !result_second_pathv.empty() ) {
                curve->set_pathvector(result_second_pathv);
            }
        }
        else if ( linked_path.linksToPath() && linked_path.getObject() ) {
            SPItem *linked1 = linked_path.getObject();
            if (linked1 && transf != Geom::identity() && selection && !selection->includes(linked1->getRepr())) {
                SP_ITEM(linked1)->doWriteTransform(transf);
            }
            Geom::PathVector linked_pathv = linked_path.get_pathvector();
            Geom::PathVector result_pathv;
            for (auto & iter : linked_pathv)
            {
                result_pathv.push_back(iter);
            }
            if ( !result_pathv.empty() ) {
                if (close) {
                    result_pathv.front().close();
                }
                curve->set_pathvector(result_pathv);
            }
        }
        else if ( second_path.linksToPath() && second_path.getObject() ) {
            SPItem *linked2 = second_path.getObject();
            if (linked2 && transf != Geom::identity() && selection && !selection->includes(linked2->getRepr())) {
                SP_ITEM(linked2)->doWriteTransform(transf);
            }
            Geom::PathVector second_pathv = second_path.get_pathvector();
            Geom::PathVector result_pathv;
            for (auto & iter : second_pathv)
            {
                result_pathv.push_back(iter);
            }
            if ( !result_pathv.empty() ) {
                if (close) {
                    result_pathv.front().close();
                }
                curve->set_pathvector(result_pathv);
            }
        }
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
