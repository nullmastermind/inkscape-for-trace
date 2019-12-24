// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/satellitesarray.h"
#include "helper/geom.h"
#include "knotholder.h"
#include "live_effects/effect.h"
#include "live_effects/lpe-fillet-chamfer.h"
#include "ui/dialog/lpe-fillet-chamfer-properties.h"
#include "ui/shape-editor.h"
#include "ui/tools-switch.h"
#include "ui/tools/node-tool.h"

#include "inkscape.h"
#include <preferences.h>
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
    : ArrayParam<std::vector<Satellite> >(label, tip, key, wr, effect, 0), _knoth(nullptr)
{
    _knot_shape = SP_KNOT_SHAPE_DIAMOND;
    _knot_mode = SP_KNOT_MODE_XOR;
    _knot_color = 0xAAFF8800;
    _use_distance = false;
    _global_knot_hide = false;
    _current_zoom = 0;
    _effectType = FILLET_CHAMFER;
    _last_pathvector_satellites = nullptr;
    param_widget_is_visible(false);
}


void SatellitesArrayParam::set_oncanvas_looks(SPKnotShapeType shape,
        SPKnotModeType mode,
        guint32 color)
{
    _knot_shape = shape;
    _knot_mode = mode;
    _knot_color = color;
}

void SatellitesArrayParam::setPathVectorSatellites(PathVectorSatellites *pathVectorSatellites, bool write)
{
    _last_pathvector_satellites = pathVectorSatellites;
    if (write) {
        param_set_and_write_new_value(_last_pathvector_satellites->getSatellites());
    } else {
        param_setValue(_last_pathvector_satellites->getSatellites());
    }
}

void SatellitesArrayParam::reloadKnots()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop && tools_isactive(desktop, TOOLS_NODES)) {
        Inkscape::UI::Tools::NodeTool *nt = static_cast<Inkscape::UI::Tools::NodeTool *>(desktop->event_context);
        if (nt) {
            for (auto i = nt->_shape_editors.begin(); i != nt->_shape_editors.end(); ++i) {
                Inkscape::UI::ShapeEditor *shape_editor = i->second;
                if (shape_editor && shape_editor->lpeknotholder) {
                    SPItem *item = shape_editor->knotholder->item;
                    shape_editor->unset_item(true);
                    shape_editor->set_item(item);
                }
            }
        }
    }
}
void SatellitesArrayParam::setUseDistance(bool use_knot_distance)
{
    _use_distance = use_knot_distance;
}

void SatellitesArrayParam::setCurrentZoom(double current_zoom)
{
    _current_zoom = current_zoom;
}

void SatellitesArrayParam::setGlobalKnotHide(bool global_knot_hide)
{
    _global_knot_hide = global_knot_hide;
}

void SatellitesArrayParam::setEffectType(EffectType et)
{
    _effectType = et;
}

void SatellitesArrayParam::updateCanvasIndicators(bool mirror)
{
    if (!_last_pathvector_satellites) {
        return;
    }

    if (!_hp.empty()) {
        _hp.clear();
    }
    Geom::PathVector pathv = _last_pathvector_satellites->getPathVector();
    if (pathv.empty()) {
        return;
    }
    if (mirror == true) {
        _hp.clear();
    }
    if (_effectType == FILLET_CHAMFER) {
        for (size_t i = 0; i < _vector.size(); ++i) {
            for (size_t j = 0; j < _vector[i].size(); ++j) {
                if (_vector[i][j].hidden || //Ignore if hidden
                    (!_vector[i][j].has_mirror && mirror == true) || //Ignore if not have mirror and we are in mirror loop
                    _vector[i][j].amount == 0 || //no helper in 0 value
                    j >= count_path_nodes(pathv[i]) || //ignore last satellite in open paths with fillet chamfer effect
                    (!pathv[i].closed() && j == 0) ||  //ignore first satellites on open paths
                    count_path_nodes(pathv[i]) == 2)
                {
                    continue;
                }
                Geom::Curve *curve_in = pathv[i][j].duplicate();
                double pos = 0;
                bool overflow = false;
                double size_out = _vector[i][j].arcDistance(*curve_in);
                double length_out = curve_in->length();
                gint previous_index = j - 1; //Always are previous index because we skip first satellite on open paths
                if (j == 0 && pathv[i].closed()) {
                    previous_index = count_path_nodes(pathv[i]) - 1;
                }
                if ( previous_index < 0 ) {
                    return;
                }
                double length_in = pathv.curveAt(previous_index).length();
                if (mirror) {
                    curve_in = const_cast<Geom::Curve *>(&pathv.curveAt(previous_index));
                    pos = _vector[i][j].time(size_out, true, *curve_in);
                    if (length_out < size_out) {
                        overflow = true;
                    }
                } else {
                    pos = _vector[i][j].time(*curve_in);
                    if (length_in < size_out) {
                        overflow = true;
                    }
                }
                if (pos <= 0 || pos >= 1) {
                    continue;
                }
            }
        }
    }
    if (mirror) {
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

void SatellitesArrayParam::param_transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    if (prefs->getBool("/options/transform/rectcorners", true)) {
        for (auto & i : _vector) {
            for (auto & j : i) {
                if (!j.is_time && j.amount > 0) {
                    j.amount = j.amount * ((postmul.expansionX() + postmul.expansionY()) / 2);
                }
            }
        }
        param_set_and_write_new_value(_vector);
    }
}

void SatellitesArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
                                                 SPItem *item,
                                                 bool mirror)
{
    if (!_last_pathvector_satellites) {
        return;
    }
    size_t index = 0;
    for (size_t i = 0; i < _vector.size(); ++i) {
        for (size_t j = 0; j < _vector[i].size(); ++j) {
            if (!_vector[i][j].has_mirror && mirror) {
                continue;
            }
            SatelliteType type = _vector[i][j].satellite_type;
            if (mirror && i == 0 && j == 0) {
                index += _last_pathvector_satellites->getTotalSatellites();
            }
            using namespace Geom;
            //If is for filletChamfer effect...
            if (_effectType == FILLET_CHAMFER) {
                const gchar *tip;
                if (type == CHAMFER) {
                    tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else if (type == INVERSE_CHAMFER) {
                    tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else if (type == INVERSE_FILLET) {
                    tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                } else {
                    tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                            "<b>Shift+Click</b> open dialog, "
                            "<b>Ctrl+Alt+Click</b> reset");
                }
                FilletChamferKnotHolderEntity *e = new FilletChamferKnotHolderEntity(this, index);
                e->create(nullptr, item, knotholder, Inkscape::CTRL_TYPE_LPE, _(tip), _knot_shape, _knot_mode,
                          _knot_color);
                knotholder->add(e);
            }
            index++;
        }
    }
    if (mirror) {
        addKnotHolderEntities(knotholder, item, false);
    }
}

void SatellitesArrayParam::updateAmmount(double amount)
{ 
    Geom::PathVector const pathv = _last_pathvector_satellites->getPathVector();
    Satellites satellites = _last_pathvector_satellites->getSatellites();
    for (size_t i = 0; i < satellites.size(); ++i) {
        for (size_t j = 0; j < satellites[i].size(); ++j) {
            Geom::Curve const &curve_in = pathv[i][j];
            if (param_effect->isNodePointSelected(curve_in.initialPoint()) ){
                _vector[i][j].amount = amount;
                _vector[i][j].setSelected(true);
            } else {
                _vector[i][j].setSelected(false);
            }
        }
    }
}

void SatellitesArrayParam::addKnotHolderEntities(KnotHolder *knotholder,
        SPItem *item)
{
    _knoth = knotholder;
    addKnotHolderEntities(knotholder, item, true);
}

FilletChamferKnotHolderEntity::FilletChamferKnotHolderEntity(
    SatellitesArrayParam *p, size_t index)
    : _pparam(p), _index(index) {}


void FilletChamferKnotHolderEntity::knot_set(Geom::Point const &p,
        Geom::Point const &/*origin*/,
        guint state)
{
    if (!_pparam->_last_pathvector_satellites) {
        return;
    }
    size_t total_satellites = _pparam->_last_pathvector_satellites->getTotalSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_satellites) {
        index = _index - total_satellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_satellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    
    Geom::Point s = snap_knot_position(p, state);
    if (!valid_index(satelite_index, subsatelite_index)) {
        return;
    }
    Satellite satellite = _pparam->_vector[satelite_index][subsatelite_index];
    Geom::PathVector pathv = _pparam->_last_pathvector_satellites->getPathVector();
    if (satellite.hidden ||
       (!pathv[satelite_index].closed() && 
        (subsatelite_index == 0||//ignore first satellites on open paths
         count_path_nodes(pathv[satelite_index]) - 1 == subsatelite_index))) //ignore last satellite in open paths with fillet chamfer effect
    {
        return;
    }
    gint previous_index = subsatelite_index - 1;
    if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
        previous_index = count_path_nodes(pathv[satelite_index]) - 1;
    }
    if ( previous_index < 0 ) {
        return;
    }
    Geom::Curve const &curve_in = pathv[satelite_index][previous_index];
    double mirror_time = Geom::nearest_time(s, curve_in);
    Geom::Point mirror = curve_in.pointAt(mirror_time);
    double normal_time = Geom::nearest_time(s, pathv[satelite_index][subsatelite_index]);
    Geom::Point normal = pathv[satelite_index][subsatelite_index].pointAt(normal_time);
    double distance_mirror = Geom::distance(mirror,s);
    double distance_normal = Geom::distance(normal,s);
    if (Geom::are_near(s, pathv[satelite_index][subsatelite_index].initialPoint(), 1.5 / _pparam->_current_zoom)) {
        satellite.amount = 0;
    } else if (distance_mirror < distance_normal) {
        double time_start = 0;
        Satellites satellites = _pparam->_last_pathvector_satellites->getSatellites();
        time_start = satellites[satelite_index][previous_index].time(curve_in);
        if (time_start > mirror_time) {
            mirror_time = time_start;
        }
        double size = arcLengthAt(mirror_time, curve_in);
        double amount = curve_in.length() - size;
        if (satellite.is_time) {
            amount = timeAtArcLength(amount, pathv[satelite_index][subsatelite_index]);
        }
        satellite.amount = amount;
    } else {
        satellite.setPosition(s, pathv[satelite_index][subsatelite_index]);
    }
    Inkscape::LivePathEffect::LPEFilletChamfer *filletchamfer = dynamic_cast<Inkscape::LivePathEffect::LPEFilletChamfer *>(_pparam->param_effect);
    filletchamfer->helperpath = true;
    _pparam->updateAmmount(satellite.amount);
    _pparam->_vector[satelite_index][subsatelite_index] = satellite;
    sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
}

void
FilletChamferKnotHolderEntity::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    Inkscape::LivePathEffect::LPEFilletChamfer *filletchamfer = dynamic_cast<Inkscape::LivePathEffect::LPEFilletChamfer *>(_pparam->param_effect);
    if (filletchamfer) {
        filletchamfer->refresh_widgets = true;
        filletchamfer->helperpath = false;
        filletchamfer->writeParamsToSVG();
        sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
    }
}

Geom::Point FilletChamferKnotHolderEntity::knot_get() const
{
    if (!_pparam->_last_pathvector_satellites || _pparam->_global_knot_hide) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Geom::Point tmp_point;
    size_t total_satellites = _pparam->_last_pathvector_satellites->getTotalSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_satellites) {
        index = _index - total_satellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_satellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    if (!valid_index(satelite_index, subsatelite_index)) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Satellite satellite = _pparam->_vector[satelite_index][subsatelite_index];
    Geom::PathVector pathv = _pparam->_last_pathvector_satellites->getPathVector();
    if (satellite.hidden ||
        (!pathv[satelite_index].closed() && (subsatelite_index == 0 ||
        subsatelite_index == count_path_nodes(pathv[satelite_index]) - 1)))  //ignore first and last satellites on open paths
    {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    this->knot->show();
    if (is_mirror) {
        gint previous_index = subsatelite_index - 1;
        if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
            previous_index = count_path_nodes(pathv[satelite_index]) - 1;
        }
        if ( previous_index < 0 ) {
            return Geom::Point(Geom::infinity(), Geom::infinity());
        }
        Geom::Curve const &curve_in = pathv[satelite_index][previous_index];
        double s = satellite.arcDistance(pathv[satelite_index][subsatelite_index]);
        double t = satellite.time(s, true, curve_in);
        if (t > 1) {
            t = 1;
        }
        if (t < 0) {
            t = 0;
        }
        double time_start = 0;
        time_start = _pparam->_last_pathvector_satellites->getSatellites()[satelite_index][previous_index].time(curve_in);
        if (time_start > t) {
            t = time_start;
        }
        tmp_point = (curve_in).pointAt(t);
    } else {
        tmp_point = satellite.getPosition(pathv[satelite_index][subsatelite_index]);
    }
    Geom::Point const canvas_point = tmp_point;
    return canvas_point;
}

void FilletChamferKnotHolderEntity::knot_click(guint state)
{
    if (!_pparam->_last_pathvector_satellites) {
        return;
    }
    size_t total_satellites = _pparam->_last_pathvector_satellites->getTotalSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_satellites) {
        index = _index - total_satellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_satellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    if (!valid_index(satelite_index, subsatelite_index)) {
        return;
    }
    Geom::PathVector pathv = _pparam->_last_pathvector_satellites->getPathVector();
    if (!pathv[satelite_index].closed() && 
       (subsatelite_index == 0 ||
        count_path_nodes(pathv[satelite_index]) - 1 == subsatelite_index)) //ignore last satellite in open paths with fillet chamfer effect
    {
        return;
    }
    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_MOD1_MASK) {
            _pparam->_vector[satelite_index][subsatelite_index].amount = 0.0;
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
        } else {
            using namespace Geom;
            SatelliteType type = _pparam->_vector[satelite_index][subsatelite_index].satellite_type;
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
            _pparam->_vector[satelite_index][subsatelite_index].satellite_type = type;
            sp_lpe_item_update_patheffect(SP_LPE_ITEM(item), false, false);
            const gchar *tip;
            if (type == CHAMFER) {
                tip = _("<b>Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            } else if (type == INVERSE_CHAMFER) {
                tip = _("<b>Inverse Chamfer</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            } else if (type == INVERSE_FILLET) {
                tip = _("<b>Inverse Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            } else {
                tip = _("<b>Fillet</b>: <b>Ctrl+Click</b> toggles type, "
                        "<b>Shift+Click</b> open dialog, "
                        "<b>Ctrl+Alt+Click</b> resets");
            }
            this->knot->tip = g_strdup(tip);
            this->knot->show();
        }
    } else if (state & GDK_SHIFT_MASK) {
        double amount = _pparam->_vector[satelite_index][subsatelite_index].amount;
        gint previous_index = subsatelite_index - 1;
        if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
            previous_index = count_path_nodes(pathv[satelite_index]) - 1;
        }
        if ( previous_index < 0 ) {
            return;
        }
        if (!_pparam->_use_distance && !_pparam->_vector[satelite_index][subsatelite_index].is_time) {
            amount = _pparam->_vector[satelite_index][subsatelite_index].lenToRad(
                amount, pathv[satelite_index][previous_index], pathv[satelite_index][subsatelite_index],
                _pparam->_vector[satelite_index][previous_index]);
        }
        bool aprox = false;
        Geom::D2<Geom::SBasis> d2_out = pathv[satelite_index][subsatelite_index].toSBasis();
        Geom::D2<Geom::SBasis> d2_in = pathv[satelite_index][previous_index].toSBasis();
        aprox = ((d2_in)[0].degreesOfFreedom() != 2 ||
                 d2_out[0].degreesOfFreedom() != 2) &&
                !_pparam->_use_distance
                ? true
                : false;
        Inkscape::UI::Dialogs::FilletChamferPropertiesDialog::showDialog(
            this->desktop, amount, this, _pparam->_use_distance, aprox,
            _pparam->_vector[satelite_index][subsatelite_index]);
    }
}

void FilletChamferKnotHolderEntity::knot_set_offset(Satellite satellite)
{
    if (!_pparam->_last_pathvector_satellites) {
        return;
    }
    size_t total_satellites = _pparam->_last_pathvector_satellites->getTotalSatellites();
    bool is_mirror = false;
    size_t index = _index;
    if (_index >= total_satellites) {
        index = _index - total_satellites;
        is_mirror = true;
    }
    std::pair<size_t, size_t> index_data = _pparam->_last_pathvector_satellites->getIndexData(index);
    size_t satelite_index = index_data.first;
    size_t subsatelite_index = index_data.second;
    if (!valid_index(satelite_index, subsatelite_index)) {
        return;
    }
    Geom::PathVector pathv = _pparam->_last_pathvector_satellites->getPathVector();
    if (satellite.hidden ||
        (!pathv[satelite_index].closed() && 
        (subsatelite_index == 0 || count_path_nodes(pathv[satelite_index]) - 1 == subsatelite_index))) //ignore last satellite in open paths with fillet chamfer effect
    {
        return;
    }
    double amount = satellite.amount;
    double max_amount = amount;
    if (!_pparam->_use_distance && !satellite.is_time) {
        gint previous_index = subsatelite_index - 1;
        if (subsatelite_index == 0 && pathv[satelite_index].closed()) {
            previous_index = count_path_nodes(pathv[satelite_index]) - 1;
        }
        if ( previous_index < 0 ) {
            return;
        }
        amount = _pparam->_vector[satelite_index][subsatelite_index].radToLen(
            amount, pathv[satelite_index][previous_index], pathv[satelite_index][subsatelite_index]);
        if (max_amount > 0 && amount == 0) {
            amount = _pparam->_vector[satelite_index][subsatelite_index].amount;
        }
    }
    satellite.amount = amount;
    _pparam->_vector[satelite_index][subsatelite_index] = satellite;
    this->parent_holder->knot_ungrabbed_handler(this->knot, 0);
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
