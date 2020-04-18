// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "live_effects/lpe-fill-between-many.h"
#include "live_effects/lpeobject.h"
#include "xml/node.h"
#include "display/curve.h"
#include "inkscape.h"
#include "selection.h"

#include "object/sp-defs.h"
#include "object/sp-shape.h"
#include "svg/svg.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<Filllpemethod> FilllpemethodData[] = {
    { FLM_ORIGINALD, N_("Without LPEs"), "originald" }, 
    { FLM_BSPLINESPIRO, N_("With Spiro or BSpline"), "bsplinespiro" },
    { FLM_D, N_("With all LPEs"), "d" }
};
static const Util::EnumDataConverter<Filllpemethod> FLMConverter(FilllpemethodData, FLM_END);

LPEFillBetweenMany::LPEFillBetweenMany(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , linked_paths(_("Linked path:"), _("Paths from which to take the original path data"), "linkedpaths", &wr, this)
    , method(_("LPEs:"), _("Which LPEs of the linked paths should be considered"), "method", FLMConverter, &wr, this, FLM_BSPLINESPIRO)
    , join(_("Join subpaths"), _("Join subpaths"), "join", &wr, this, true)
    , close(_("Close"), _("Close path"), "close", &wr, this, true)
    , autoreverse(_("Autoreverse"), _("Autoreverse"), "autoreverse", &wr, this, true)
    , applied("Store the first apply", "", "applied", &wr, this, "false", false)
{
    registerParameter(&linked_paths);
    registerParameter(&method);
    registerParameter(&join);
    registerParameter(&close);
    registerParameter(&autoreverse);
    registerParameter(&applied);
    previous_method = FLM_END;
}

LPEFillBetweenMany::~LPEFillBetweenMany()
= default;

void LPEFillBetweenMany::doEffect (SPCurve * curve)
{
    if (previous_method != method) {
        if (method == FLM_BSPLINESPIRO) {
            linked_paths.allowOnlyBsplineSpiro(true);
            linked_paths.setFromOriginalD(false);
        } else if(method == FLM_ORIGINALD) {
            linked_paths.allowOnlyBsplineSpiro(false);
            linked_paths.setFromOriginalD(true);
        } else {
            linked_paths.allowOnlyBsplineSpiro(false);
            linked_paths.setFromOriginalD(false);
        }
        previous_method = method;
    }
    Geom::PathVector res_pathv;
    Geom::Affine transf = sp_item_transform_repr(sp_lpe_item);
    if (transf != Geom::identity()) {
        sp_lpe_item->doWriteTransform(Geom::identity());
    }
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    Inkscape::Selection *selection = nullptr;
    if (desktop) {
        selection = desktop->selection;
    }
    if (!autoreverse) {
        for (auto & iter : linked_paths._vector) {
            SPObject *obj;
            if (iter->ref.isAttached() && (obj = iter->ref.getObject()) && SP_IS_ITEM(obj) &&
                !iter->_pathvector.empty() && iter->visibled) {
                Geom::Path linked_path;
                if (iter->_pathvector.front().closed() && linked_paths._vector.size() > 1) {
                    continue;
                }
                if (obj && transf != Geom::identity() && selection && !selection->includes(obj->getRepr())) {
                    SP_ITEM(obj)->doWriteTransform(transf);
                }
                if (iter->reversed) {
                    linked_path = iter->_pathvector.front().reversed();
                } else {
                    linked_path = iter->_pathvector.front();
                }
                if (!res_pathv.empty() && join) {
                    if (!are_near(res_pathv.front().finalPoint(), linked_path.initialPoint(), 0.1)) {
                        res_pathv.front().appendNew<Geom::LineSegment>(linked_path.initialPoint());
                    } else {
                        linked_path.setInitial(res_pathv.front().finalPoint());
                    }
                    res_pathv.front().append(linked_path);
                } else {
                    if (close && !join) {
                        linked_path.close();
                    }
                    res_pathv.push_back(linked_path);
                }
            }
        }
    } else {
        unsigned int counter = 0;
        Geom::Point current = Geom::Point();
        std::vector<unsigned int> done;
        for (auto & iter : linked_paths._vector) {
            SPObject *obj;
            if (iter->ref.isAttached() && (obj = iter->ref.getObject()) && SP_IS_ITEM(obj) &&
                !iter->_pathvector.empty() && iter->visibled) {
                Geom::Path linked_path;
                if (iter->_pathvector.front().closed() && linked_paths._vector.size() > 1) {
                    counter++;
                    continue;
                }
                if (obj && transf != Geom::identity() && selection && !selection->includes(obj->getRepr())) {
                    SP_ITEM(obj)->doWriteTransform(transf);
                }
                if (counter == 0) {
                    current = iter->_pathvector.front().finalPoint();
                    Geom::Path initial_path = iter->_pathvector.front();
                    done.push_back(0);
                    if (close && !join) {
                        initial_path.close();
                    }
                    res_pathv.push_back(initial_path);
                }
                Geom::Coord distance = Geom::infinity();
                unsigned int counter2 = 0;
                unsigned int added = 0;
                PathAndDirectionAndVisible *nearest = nullptr;
                for (auto & iter2 : linked_paths._vector) {
                    SPObject *obj2;
                    if (iter2->ref.isAttached() && (obj2 = iter2->ref.getObject()) && SP_IS_ITEM(obj2) &&
                        !iter2->_pathvector.empty() && iter2->visibled) {
                        if (obj == obj2 || std::find(done.begin(), done.end(), counter2) != done.end()) {
                            counter2++;
                            continue;
                        }
                        if (iter2->_pathvector.front().closed() && linked_paths._vector.size() > 1) {
                            counter2++;
                            continue;
                        }
                        Geom::Point start = iter2->_pathvector.front().initialPoint();
                        Geom::Point end = iter2->_pathvector.front().finalPoint();
                        Geom::Coord distance_iter =
                            std::min(Geom::distance(current, end), Geom::distance(current, start));
                        if (distance > distance_iter) {
                            distance = distance_iter;
                            nearest = iter2;
                            added = counter2;
                        }
                        counter2++;
                    }
                }
                if (nearest != nullptr) {
                    done.push_back(added);
                    Geom::Point start = nearest->_pathvector.front().initialPoint();
                    Geom::Point end = nearest->_pathvector.front().finalPoint();
                    if (Geom::distance(current, end) > Geom::distance(current, start)) {
                        linked_path = nearest->_pathvector.front();
                    } else {
                        linked_path = nearest->_pathvector.front().reversed();
                    }
                    current = end;
                    if (!res_pathv.empty() && join) {
                        if (!are_near(res_pathv.front().finalPoint(), linked_path.initialPoint(), 0.1)) {
                            res_pathv.front().appendNew<Geom::LineSegment>(linked_path.initialPoint());
                        } else {
                            linked_path.setInitial(res_pathv.front().finalPoint());
                        }
                        res_pathv.front().append(linked_path);
                    } else {
                        if (close && !join) {
                            linked_path.close();
                        }
                        res_pathv.push_back(linked_path);
                    }
                }
                counter++;
            }
        }
    }
    if (!res_pathv.empty() && close) {
        res_pathv.front().close();
    }
    if (res_pathv.empty()) {
        res_pathv = curve->get_pathvector();
    }

    curve->set_pathvector(res_pathv);
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
