/*
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for point me to the idea
 * Also su_v for his construvtive feedback and time
 * and finaly to Liam P. White for his big help on coding, that save me a lot of
 * hours
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "knotholder.h"
#include "ui/dialog/lpe-fillet-chamfer-properties.h"
#include "live_effects/parameter/satellitearray.h"
#include "live_effects/effect.h"
#include "sp-lpe-item.h"
#include <preferences.h>
#include <boost/optional.hpp>
// TODO due to internal breakage in glibmm headers,
// this has to be included last.
#include <glibmm/i18n.h>

using namespace Geom;

namespace Inkscape {

namespace LivePathEffect {

SatelliteArrayParam::SatelliteArrayParam(const Glib::ustring &label,
        const Glib::ustring &tip,
        const Glib::ustring &key,
        Inkscape::UI::Widget::Registry *wr,
        Effect *effect)
    : ArrayParam<Geom::Satellite>(label, tip, key, wr, effect, 0), knoth(NULL)
{
    _knot_shape = SP_KNOT_SHAPE_DIAMOND;
    _knot_mode = SP_KNOT_MODE_XOR;
    _knot_color = 0x00ff0000;
    _helper_size = 0;
    _use_distance = false;
    _effectType = FILLET_CHAMFER;
    _last_pointwise = NULL;
}

SatelliteArrayParam::~SatelliteArrayParam() {}

void SatelliteArrayParam::set_oncanvas_looks(SPKnotShapeType shape,
        SPKnotModeType mode,
        guint32 color)
{
    _knot_shape = shape;
    _knot_mode = mode;
    _knot_color = color;
}

void SatelliteArrayParam::setPointwise(Geom::Pointwise *pointwise)
{
    _last_pointwise = pointwise;
    param_set_and_write_new_value(_last_pointwise->getSatellites());
}

void SatelliteArrayParam::setDocumentUnit(Glib::ustring value_document_unit)
{
    _documentUnit = value_document_unit;
}

void SatelliteArrayParam::setUseDistance(bool use_knot_distance)
{
    _use_distance = use_knot_distance;
}

void SatelliteArrayParam::setUnit(const gchar *abbr)
{
    _unit = abbr;
}

void SatelliteArrayParam::setEffectType(EffectType et)
{
    _effectType = et;
}

void SatelliteArrayParam::setHelperSize(int hs)
{
    _helper_size = hs;
    updateCanvasIndicators();
}

void SatelliteArrayParam::updateCanvasIndicators(bool mirror)
{
    if (!_last_pointwise) {
        return;
    }
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = _last_pointwise->getPwd2();
    Pathinfo path_info(pwd2);
    if (mirror == true) {
        _hp.clear();
    }
    for (size_t i = 0; i < _vector.size(); ++i) {
        if (!_vector[i].active || _vector[i].hidden) {
            continue;
        }
        if ((!_vector[i].hasMirror && mirror == true) || _vector[i].amount == 0) {
            continue;
        }
        double pos = 0;
        if (pwd2.size() <= i) {
            break;
        }
        Geom::D2<Geom::SBasis> d2 = pwd2[i];
        bool overflow = false;
        double size_out = _vector[i].size(pwd2[i]);
        double lenght_out = Geom::length(pwd2[i], Geom::EPSILON);
        double lenght_in = 0;
        boost::optional<size_t> d2_prev_index = path_info.previous(i);
        if (d2_prev_index) {
            lenght_in = Geom::length(pwd2[*d2_prev_index], Geom::EPSILON);
        }
        if (mirror == true) {
            if (d2_prev_index) {
                d2 = pwd2[*d2_prev_index];
                pos = _vector[i].time(size_out, true, d2);
                if (lenght_out < size_out) {
                    overflow = true;
                }
            }
        } else {
            pos = _vector[i].time(d2);
            if (lenght_in < size_out) {
                overflow = true;
            }
        }
        if (pos <= 0 || pos >= 1) {
            continue;
        }
        Geom::Point point_a = d2.valueAt(pos);
        Geom::Point deriv_a = unit_vector(derivative(d2).valueAt(pos));
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
            aff *= Geom::Rotate(ray_1.angle() - deg_to_rad(90));
        } else {
            aff *= Geom::Rotate(ray_1.angle() - deg_to_rad(270));
        }
        pathv *= aff;
        pathv += d2.valueAt(pos);
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
                pathv *= Geom::Scale(diameter);
                pathv += point_a - Geom::Point(diameter * 0.35, diameter * 0.35);
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
                    aff *= Geom::Rotate(ray_1.angle() - deg_to_rad(90));
                } else {
                    aff *= Geom::Rotate(ray_1.angle() - deg_to_rad(270));
                }
                pathv *= aff;
                pathv += d2.valueAt(pos);
                _hp.push_back(pathv[0]);
            }
        }
    }
    if (mirror == true) {
        updateCanvasIndicators(false);
    }
}
void SatelliteArrayParam::updateCanvasIndicators()
{
    updateCanvasIndicators(true);
}

void SatelliteArrayParam::addCanvasIndicators(
    SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

void SatelliteArrayParam::param_transform_multiply(Geom::Affine const &postmul,
        bool /*set*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/options/transform/rectcorners", true)) {
        for (size_t i = 0; i < _vector.size(); ++i) {
            if (!_vector[i].isTime && _vector[i].amount > 0) {
                _vector[i].amount = _vector[i].amount *
                                    ((postmul.expansionX() + postmul.expansionY()) / 2);
            }
        }
        param_set_and_write_new_value(_vector);
    }
}

void SatelliteArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item, bool mirror)
{
    for (size_t i = 0; i < _vector.size(); ++i) {
        size_t iPlus = i;
        if (mirror == true) {
            iPlus = i + _vector.size();
        }
        if (!_vector[i].active) {
            continue;
        }
        if (!_vector[i].hasMirror && mirror == true) {
            continue;
        }
        using namespace Geom;
        SatelliteType type = _vector[i].satelliteType;
        //IF is for filletChamfer effect...
        if (_effectType == FILLET_CHAMFER) {
            const gchar *tip;
            if (type == C) {
                tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IC) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IF) {
                tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else {
                tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            }
            FilletChamferKnotHolderEntity *e =
                new FilletChamferKnotHolderEntity(this, iPlus);
            e->create(desktop, item, knotholder, Inkscape::CTRL_TYPE_UNKNOWN, _(tip),
                      _knot_shape, _knot_mode, _knot_color);
            knotholder->add(e);
        }
    }
    if (mirror == true) {
        addKnotHolderEntities(knotholder, desktop, item, false);
    }
}

void SatelliteArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPDesktop *desktop,
        SPItem *item)
{
    knoth = knotholder;
    addKnotHolderEntities(knotholder, desktop, item, true);
}

FilletChamferKnotHolderEntity::FilletChamferKnotHolderEntity(
    SatelliteArrayParam *p, size_t index)
    : _pparam(p), _index(index) {}

void FilletChamferKnotHolderEntity::knot_set(Point const &p,
        Point const &/*origin*/,
        guint state)
{
    Geom::Point s = snap_knot_position(p, state);
    size_t index = _index;
    if (_index >= _pparam->_vector.size()) {
        index = _index - _pparam->_vector.size();
    }
    if (!valid_index(index)) {
        return;
    }

    if (!_pparam->_last_pointwise) {
        return;
    }

    Geom::Satellite satellite = _pparam->_vector.at(index);
    if (!satellite.active || satellite.hidden) {
        return;
    }
    Geom::Pointwise *pointwise = _pparam->_last_pointwise;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = pointwise->getPwd2();
    Pathinfo path_info(pwd2);
    if (_pparam->_vector.size() <= _index) {
        boost::optional<size_t> d2_prev_index = path_info.previous(index);
        if (d2_prev_index) {
            Geom::D2<Geom::SBasis> d2_in = pwd2[*d2_prev_index];
            double mirror_time = Geom::nearest_point(s, d2_in);
            double time_start = 0;
            std::vector<Geom::Satellite> sats = pointwise->getSatellites();
            time_start = sats[*d2_prev_index].time(d2_in);
            if (time_start > mirror_time) {
                mirror_time = time_start;
            }
            double size = satellite.toSize(mirror_time, d2_in);
            double amount = Geom::length(d2_in, Geom::EPSILON) - size;
            if (satellite.isTime) {
                amount = satellite.toTime(amount, pwd2[index]);
            }
            satellite.amount = amount;
        }
    } else {
        satellite.setPosition(s, pwd2[index]);
    }
    _pparam->_vector.at(index) = satellite;
    SPLPEItem *splpeitem = dynamic_cast<SPLPEItem *>(item);
    if (splpeitem) {
        sp_lpe_item_update_patheffect(splpeitem, false, false);
    }
}

Geom::Point FilletChamferKnotHolderEntity::knot_get() const
{
    Geom::Point tmp_point;
    size_t index = _index;
    if (_index >= _pparam->_vector.size()) {
        index = _index - _pparam->_vector.size();
    }
    if (!valid_index(index)) {
        return Point(infinity(), infinity());
    }
    Geom::Satellite satellite = _pparam->_vector.at(index);
    if (!_pparam->_last_pointwise) {
        return Point(infinity(), infinity());
    }
    if (!satellite.active || satellite.hidden) {
        return Point(infinity(), infinity());
    }
    Geom::Pointwise *pointwise = _pparam->_last_pointwise;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = pointwise->getPwd2();
    Pathinfo path_info(pwd2);
    if (pwd2.size() <= index) {
        return Point(infinity(), infinity());
    }
    this->knot->show();
    if (_index >= _pparam->_vector.size()) {
        tmp_point = satellite.getPosition(pwd2[index]);
        boost::optional<size_t> d2_prev_index = path_info.previous(index);
        if (d2_prev_index) {
            Geom::D2<Geom::SBasis> d2_in = pwd2[*d2_prev_index];
            double s = satellite.size(pwd2[index]);
            double t = satellite.time(s, true, d2_in);
            if (t > 1) {
                t = 1;
            }
            if (t < 0) {
                t = 0;
            }
            double time_start = 0;
            time_start = pointwise->getSatellites()[*d2_prev_index].time(d2_in);
            if (time_start > t) {
                t = time_start;
            }
            tmp_point = (d2_in).valueAt(t);
        }
    } else {
        tmp_point = satellite.getPosition(pwd2[index]);
    }
    Geom::Point const canvas_point = tmp_point;
    return canvas_point;
}

void FilletChamferKnotHolderEntity::knot_click(guint state)
{
    if (!_pparam->_last_pointwise) {
        return;
    }

    size_t index = _index;
    if (_index >= _pparam->_vector.size()) {
        index = _index - _pparam->_vector.size();
    }
    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_MOD1_MASK) {
            _pparam->_vector.at(index).amount = 0.0;
            _pparam->param_set_and_write_new_value(_pparam->_vector);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
        } else {
            using namespace Geom;
            SatelliteType type = _pparam->_vector.at(index).satelliteType;
            switch (type) {
            case F:
                type = IF;
                break;
            case IF:
                type = C;
                break;
            case C:
                type = IC;
                break;
            default:
                type = F;
                break;
            }
            _pparam->_vector.at(index).satelliteType = type;
            _pparam->param_set_and_write_new_value(_pparam->_vector);
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
            const gchar *tip;
            if (type == C) {
                tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IC) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggle type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> reset");
            } else if (type == IF) {
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
        Piecewise<D2<SBasis> > pwd2 = _pparam->_last_pointwise->getPwd2();
        Pathinfo path_info(pwd2);
        double amount = _pparam->_vector.at(index).amount;
        if (!_pparam->_use_distance && !_pparam->_vector.at(index).isTime) {
            boost::optional<size_t> prev = path_info.previous(index);
            boost::optional<Geom::D2<Geom::SBasis> > prevPwd2 = boost::none;
            boost::optional<Geom::Satellite> prevSat = boost::none;
            if (prev) {
                prevPwd2 = pwd2[*prev];
                prevSat = _pparam->_vector.at(*prev);
            }
            amount = _pparam->_vector.at(index)
                     .lenToRad(amount, prevPwd2, pwd2[index], prevSat);
        }
        bool aprox = false;
        D2<SBasis> d2_out = _pparam->_last_pointwise->getPwd2()[index];
        boost::optional<size_t> d2_prev_index = path_info.previous(index);
        if (d2_prev_index) {
            Geom::D2<Geom::SBasis> d2_in =
                _pparam->_last_pointwise->getPwd2()[*d2_prev_index];
            aprox = ((d2_in)[0].degreesOfFreedom() != 2 ||
                     d2_out[0].degreesOfFreedom() != 2) &&
                    !_pparam->_use_distance
                    ? true
                    : false;
        }
        Inkscape::UI::Dialogs::FilletChamferPropertiesDialog::showDialog(
            this->desktop, amount, this, _pparam->_unit, _pparam->_use_distance,
            aprox, _pparam->_documentUnit, _pparam->_vector.at(index));

    }
}

void FilletChamferKnotHolderEntity::knot_set_offset(Geom::Satellite satellite)
{
    if (!_pparam->_last_pointwise) {
        return;
    }
    size_t index = _index;
    if (_index >= _pparam->_vector.size()) {
        index = _index - _pparam->_vector.size();
    }
    double amount = satellite.amount;
    double max_amount = amount;
    if (!_pparam->_use_distance && !satellite.isTime) {
        Piecewise<D2<SBasis> > pwd2 = _pparam->_last_pointwise->getPwd2();
        Pathinfo path_info(pwd2);
        boost::optional<size_t> prev = path_info.previous(index);
        boost::optional<Geom::D2<Geom::SBasis> > prevPwd2 = boost::none;
        boost::optional<Geom::Satellite> prevSat = boost::none;
        if (prev) {
            prevPwd2 = pwd2[*prev];
            prevSat = _pparam->_vector.at(*prev);
        }
        amount = _pparam->_vector.at(index)
                 .radToLen(amount, prevPwd2, pwd2[index], prevSat);
        if (max_amount > 0 && amount == 0) {
            amount = _pparam->_vector.at(index).amount;
        }
    }
    satellite.amount = amount;
    _pparam->_vector.at(index) = satellite;
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
