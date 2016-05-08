/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "knotholder.h"
#include "ui/dialog/lpe-fillet-chamfer-properties.h"
#include "live_effects/parameter/satellitesarray.h"
#include "live_effects/effect.h"
#include "sp-lpe-item.h"
#include <preferences.h>
#include <boost/optional.hpp>
// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>

namespace Inkscape {

namespace LivePathEffect {

SatellitesArrayParam::SatellitesArrayParam(const Glib::ustring &label,
        const Glib::ustring &tip,
        const Glib::ustring &key,
        Inkscape::UI::Widget::Registry *wr,
        Effect *effect)
    : ArrayArrayParam<ArrayArrayParam<Satellite> >(label, tip, key, wr, effect, 0), knoth(NULL)
{
    _knot_shape = SP_KNOT_SHAPE_DIAMOND;
    _knot_mode = SP_KNOT_MODE_XOR;
    _knot_color = 0xAAFF8800;
    _helper_size = 0;
    _use_distance = false;
    _effectType = FILLET_CHAMFER;
    _last_pointwise = NULL;
}

void SatellitesArrayParam::set_oncanvas_looks(SPKnotShapeType shape,
        SPKnotModeType mode,
        guint32 color)
{
    _knot_shape = shape;
    _knot_mode = mode;
    _knot_color = color;
}

void SatellitesArrayParam::setPointwise(Pointwise *pointwise)
{
    _last_pointwise = pointwise;
    param_set_and_write_new_value(_last_pointwise->getSatellites());
}

void SatellitesArrayParam::setUseDistance(bool use_knot_distance)
{
    _use_distance = use_knot_distance;
}

void SatellitesArrayParam::setEffectType(EffectType et)
{
    _effectType = et;
}

void SatellitesArrayParam::setHelperSize(int hs)
{
    _helper_size = hs;
    updateCanvasIndicators();
}

void SatellitesArrayParam::updateCanvasIndicators(bool mirror)
{
    if (!_last_pointwise) {
        return;
    }
    Geom::PathVector pathv = _last_pointwise->getPV();
    if (mirror == true) {
        _hp.clear();
    }
    for (size_t i = 0; i < _vector.size(); ++i) {
        for (size_t j = 0; j < _vector[i].size(); ++j) {
            if (!_vector[i][j].active || _vector[i][j].hidden) {
                continue;
            }
            if ((!_vector[i][j].has_mirror && mirror == true) || _vector[i][j].amount == 0) {
                continue;
            }
            double pos = 0;
            if (pathv.size() <= i || pathv[i].size() <= j) {
                break;
            }
            Geom::Curve *curve_in = pathv[i][j].duplicate();
            bool overflow = false;
            double size_out = _vector[i][j].arcDistance(*curve_in);
            double lenght_out = curve_in->length();
            double lenght_in = 0;
            
            boost::optional<size_t> curve_prev_index = boost::none;
            if(j == 0 && pathv[i].closed()){
                curve_prev_index = pathv[pathv[i].size() - 1];
            } else if(!pathv[i].closed() || j != 0) {
                curve_prev_index = j - 1;
            }
            if (curve_prev_index) {
                lenght_in = pathv.curveAt(*curve_prev_index).length();
            }
            if (mirror == true) {
                if (curve_prev_index) {
                    curve_in = const_cast<Geom::Curve *>(&pathv.curveAt(*curve_prev_index));
                    pos = _vector[i][j].time(size_out, true, *curve_in);
                    if (lenght_out < size_out) {
                        overflow = true;
                    }
                }
            } else {
                pos = _vector[i][j].time(*curve_in);
                if (lenght_in < size_out) {
                    overflow = true;
                }
            }
            if (pos <= 0 || pos >= 1) {
                continue;
            }
            Geom::Point point_a = curve_in->pointAt(pos);
            Geom::Point deriv_a = unit_vector(derivative(curve_in->toSBasis()).pointAt(pos));
            Geom::Rotate rot(Geom::Rotate::from_degrees(-90));
            deriv_a = deriv_a * rot;
            Geom::Point point_c = point_a - deriv_a * _helper_size;
            Geom::Point point_d = point_a + deriv_a * _helper_size;
            Geom::Ray ray_1(point_c, point_d);
            char const *svgd = "M 1,0.25 0.5,0 1,-0.25 M 1,0.5 0,0 1,-0.5";
            Geom::PathVector pathv = sp_svg_read_pathv(svgd);
            Geom::Affine aff = Geom::Affine();
            aff *= Geom::Scale(_helper_size);
            if (mirror == true) {
                aff *= Geom::Rotate(ray_1.angle() - Geom::rad_from_deg(90));
            } else {
                aff *= Geom::Rotate(ray_1.angle() - Geom::rad_from_deg(270));
            }
            aff *= Geom::Translate(curve_in->pointAt(pos));
            pathv *= aff;
            _hp.push_back(pathv[0]);
            _hp.push_back(pathv[1]);
            if (overflow) {
                double diameter = _helper_size;
                if (_helper_size == 0) {
                    diameter = 15;
                    char const *svgd;
                    svgd = "M 0.7,0.35 A 0.35,0.35 0 0 1 0.35,0.7 0.35,0.35 0 0 1 0,0.35 "
                           "0.35,0.35 0 0 1 0.35,0 0.35,0.35 0 0 1 0.7,0.35 Z";
                    Geom::PathVector pathv = sp_svg_read_pathv(svgd);
                    aff = Geom::Affine();
                    aff *= Geom::Scale(diameter);
                    aff *= Geom::Translate(point_a - Geom::Point(diameter * 0.35, diameter * 0.35));
                    pathv *= aff;
                    _hp.push_back(pathv[0]);
                } else {
                    char const *svgd;
                    svgd = "M 0 -1.32 A 1.32 1.32 0 0 0 -1.32 0 A 1.32 1.32 0 0 0 0 1.32 A "
                           "1.32 1.32 0 0 0 1.18 0.59 L 0 0 L 1.18 -0.59 A 1.32 1.32 0 0 0 "
                           "0 -1.32 z";
                    Geom::PathVector pathv = sp_svg_read_pathv(svgd);
                    aff = Geom::Affine();
                    aff *= Geom::Scale(_helper_size / 2.0);
                    if (mirror == true) {
                        aff *= Geom::Rotate(ray_1.angle() - Geom::rad_from_deg(90));
                    } else {
                        aff *= Geom::Rotate(ray_1.angle() - Geom::rad_from_deg(270));
                    }
                    aff *= Geom::Translate(curve_in->pointAt(pos));
                    pathv *= aff;
                    _hp.push_back(pathv[0]);
                }
            }
        }
    }
    if (mirror == true) {
        updateCanvasIndicators(false);
    }
}
void SatellitesArrayParam::updateCanvasIndicators()
{
    updateCanvasIndicators(true);
}

void SatellitesArrayParam::addCanvasIndicators(
    SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

void SatellitesArrayParam::param_transform_multiply(Geom::Affine const &postmul,
        bool /*set*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/options/transform/rectcorners", true)) {
        for (size_t i = 0; i < _vector.size(); ++i) {
            for (size_t j = 0; j < _vector[i].size(); ++j) {
                if (!_vector[i][j].is_time && _vector[i][j].amount > 0) {
                    _vector[i][j].amount = _vector[i][j].amount *
                                        ((postmul.expansionX() + postmul.expansionY()) / 2);
                }
            }
        }
        param_set_and_write_new_value(_vector);
    }
}

void SatellitesArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item, bool mirror)
{
    for (size_t i = 0; i < _vector.size(); ++i) {
        for (size_t j = 0; j < _vector[i].size(); ++j) {
            size_t iPlus = j;
            if (mirror == true) {
                iPlus = j + _vector[i].size();
            }
            if (!_vector[i][j].active) {
                continue;
            }
            if (!_vector[i][j].has_mirror && mirror == true) {
                continue;
            }
            using namespace Geom;
            SatelliteType type = _vector[i][j].satellite_type;
            //IF is for filletChamfer effect...
            if (_effectType == FILLET_CHAMFER) {
                const gchar *tip;
                if (type == CHAMFER) {
                    tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else if (type == INVERSE_CHAMFER) {
                    tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else if (type == INVERSE_FILLET) {
                    tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else {
                    tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                }
                FilletChamferKnotHolderEntity *e =
                    new FilletChamferKnotHolderEntity(this, i, iPlus);
                e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                          _knot_shape, _knot_mode, _knot_color);
                knotholder->add(e);
            }
        }
    }
    if (mirror == true) {
        addKnotHolderEntities(knotholder, desktop, item, false);
    }
}

void SatellitesArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item)
{
    knoth = knotholder;
    addKnotHolderEntities(knotholder, desktop, item, true);
}

FilletChamferKnotHolderEntity::FilletChamferKnotHolderEntity(
    SatellitesArrayParam *p, size_t index, size_t subindex)
    : _pparam(p), _index(index), _subindex(subindex) {}

void FilletChamferKnotHolderEntity::knot_set(Geom::Point const &p,
        Geom::Point const &/*origin*/,
        guint state)
{
    Geom::Point s = snap_knot_position(p, state);
    size_t subindex = _subindex;
    if (_subindex >= _pparam->_vector[_index].size()) {
        subindex = _subindex - _pparam->_vector[_index].size();
    }
    if (!valid_index(subindex)) {
        return;
    }

    if (!_pparam->_last_pointwise) {
        return;
    }

    Satellite satellite = _pparam->_vector[_index].at(subindex);
    if (!satellite.active || satellite.hidden) {
        return;
    }
    Pointwise *pointwise = _pparam->_last_pointwise;
    Geom::PathVector pathv = pparam->_last_pointwise->getPV();
    if (_subindex >= _pparam->_vector[_index].size() ) {
        boost::optional<size_t> curve_prev_index = boost::none;
        if(_subindex == 0 && pathv[_index].closed()){
            curve_prev_index = pathv[pathv[_index].size() - 1];
        } else if(!pathv[_index].closed() || _subindex != 0) {
            curve_prev_index = _subindex - 1;
        }
        if (curve_prev_index) {
            Geom::Curve const &curve_in = pathv[_index][*curve_prev_index];
            double mirror_time = Geom::nearest_time(s, curve_in);
            double time_start = 0;
            Satellites sats = pointwise->getSatellites();
            time_start = sats[_index][*curve_prev_index].time(curve_in);
            if (time_start > mirror_time) {
                mirror_time = time_start;
            }
            double size = arcLengthAt(mirror_time, curve_in);
            double amount = curve_in.length() - size;
            if (satellite.is_time) {
                amount = timeAtArcLength(amount, pathv[_index][);
            }
            satellite.amount = amount;
        }
    } else {
        satellite.setPosition(s, pathv[_index]);
    }
    _pparam->_vector[_index][_subindex] = satellite;
    SPLPEItem *splpeitem = dynamic_cast<SPLPEItem *>(item);
    if (splpeitem) {
        sp_lpe_item_update_patheffect(splpeitem, false, false);
    }
}

Geom::Point FilletChamferKnotHolderEntity::knot_get() const
{
    Geom::Point tmp_point;
    size_t subindex = _subindex;
    if (_subindex >= _pparam->_vector[_index].size()) {
        subindex = _subindex - _pparam->_vector[_index].size();
    }
    if (!valid_index(_subindex)) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Satellite satellite = _pparam->_vector[_index][_subindex];
    if (!_pparam->_last_pointwise) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    if (!satellite.active || satellite.hidden) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Pointwise *pointwise = _pparam->_last_pointwise;
    Geom::PathVector pathv = pparam->_last_pointwise->getPV();
    if (pathv[_index].size() <= subindex) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    this->knot->show();
    if (_subindex >= _pparam->_vector[index].size()) {
        tmp_point = satellite.getPosition(pathv[_index][_subindex]);
        boost::optional<size_t> curve_prev_index = boost::none;
        if(_subindex == 0 && pathv[_index].closed()){
            curve_prev_index = pathv[pathv[_index].size() - 1];
        } else if(!pathv[_index].closed() || _subindex != 0) {
            curve_prev_index = _subindex - 1;
        }
        if (curve_prev_index) {
            Geom::Curve const &curve_in = pathv[_index][*curve_prev_index];
            double s = satellite.arcDistance(pathv[_index][_subindex]);
            double t = satellite.time(s, true, curve_in);
            if (t > 1) {
                t = 1;
            }
            if (t < 0) {
                t = 0;
            }
            double time_start = 0;
            time_start = pointwise->getSatellites()[_index][*curve_prev_index].time(curve_in);
            if (time_start > t) {
                t = time_start;
            }
            tmp_point = (curve_in).pointAt(t);
        }
    } else {
        tmp_point = satellite.getPosition(pathv[_index][_subindex]);
    }
    Geom::Point const canvas_point = tmp_point;
    return canvas_point;
}

void FilletChamferKnotHolderEntity::knot_click(guint state)
{
    if (!_pparam->_last_pointwise) {
        return;
    }

    size_t subindex = _subindex;
    if (_subindex >= _pparam->_vector[_index].size()) {
        subindex = _subindex - _pparam->_vector[_index].size();
    }
    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_MOD1_MASK) {
            _pparam->_vector[_index][_subindex].amount = 0.0;
            _pparam->param_set_and_write_new_value(_pparam->_vector);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
        } else {
            using namespace Geom;
            SatelliteType type = _pparam->_vector[_index][_subindex].satellite_type;
            switch (type) {
            case FILLET:
                type = INVERSE_FILLET;
                break;
            case INVERSE_FILLET:
                type = CHAMFER;
                break;
            case CHAMFER:
                type = INVERSE_CHAMFER;
                break;
            default:
                type = FILLET;
                break;
            }
            _pparam->_vector[_index][subindex].satellite_type = type;
            _pparam->param_set_and_write_new_value(_pparam->_vector);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
            const gchar *tip;
            if (type == CHAMFER) {
                tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == INVERSE_CHAMFER) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == INVERSE_FILLET) {
                tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else {
                tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            }
            this->knot->tip = g_strdup(tip);
            this->knot->show();
        }
    } else if (state & GDK_SHIFT_MASK) {
        Geom::PathVector pathv = pparam->_last_pointwise->getPV();
        double amount = _pparam->_vector[_index][_subindex].amount;
        boost::optional<size_t> curve_prev_index = boost::none;
        if(_subindex == 0 && pathv[_index].closed()){
            curve_prev_index = pathv[pathv[_index].size() - 1];
        } else if(!pathv[_index].closed() || _subindex != 0) {
            curve_prev_index = _subindex - 1;
        }
        if (!_pparam->_use_distance && !_pparam->_vector[_index][subindex].is_time) {
            if (curve_prev_index) {
                amount = _pparam->_vector[_index][subindex].lenToRad(amount, pathv[_index][*curve_prev_index], pathv[_index][subindex], _pparam->_vector[_index][*curve_prev_index]);
            } else {
                amount = 0.0;
            }
        }
        bool aprox = false;
        Geom::D2<Geom::SBasis> d2_out = _pparam->_last_pointwise->getPwd2()[_index][subindex];
        if (curve_prev_index) {
            Geom::D2<Geom::SBasis> d2_in =
                _pparam->_last_pointwise->getPwd2()[_index][*curve_prev_index];
            aprox = ((d2_in)[0].degreesOfFreedom() != 2 ||
                     d2_out[0].degreesOfFreedom() != 2) &&
                    !_pparam->_use_distance
                    ? true
                    : false;
        }
        Inkscape::UI::Dialogs::FilletChamferPropertiesDialog::showDialog(
            this->desktop, amount, this, _pparam->_use_distance,
            aprox, _pparam->_vector[_index][subindex]);

    }
}

void FilletChamferKnotHolderEntity::knot_set_offset(Satellite satellite)
{
    if (!_pparam->_last_pointwise) {
        return;
    }
    size_t subindex = _subindex;
    if (_subindex >= _pparam->_vector[_index].size()) {
        subindex = _subindex - _pparam->_vector[_index].size();
    }
    double amount = satellite.amount;
    double max_amount = amount;
    if (!_pparam->_use_distance && !satellite.is_time) {
        Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = _pparam->_last_pointwise->getPwd2();
        Geom::PathVector pathv = path_from_piecewise(Geom::remove_short_cuts(pwd2,0.01),0.01);
        Geom::Path sat_path = pathv.pathAt(index);
        boost::optional<size_t> curve_prev_index = boost::none;
        if(_subindex == 0 && pathv[_index].closed()){
            curve_prev_index = pathv[pathv[_index].size() - 1];
        } else if(!pathv[_index].closed() || _subindex != 0) {
            curve_prev_index = _subindex - 1;
        }
        if (curve_prev_index) {
            amount = _pparam->_vector[_index][subindex].radToLen(amount, pathv[_index][*prev], pathv[_index][subindex]);
        } else {
            amount = 0.0;
        }
        if (max_amount > 0 && amount == 0) {
            amount = _pparam->_vector[_index][subindex].amount;
        }
    }
    satellite.amount = amount;
    _pparam->_vector[_index][subindex] = satellite;
    this->parent_holder->knot_ungrabbed_handler(this->knot, 0);
    _pparam->param_set_and_write_new_value(_pparam->_vector);
    SPLPEItem *splpeitem = dynamic_cast<SPLPEItem *>(item);
    if (splpeitem) {
        sp_lpe_item_update_patheffect(splpeitem, false, false);
    }
}

} /* namespace LivePathEffect */

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
